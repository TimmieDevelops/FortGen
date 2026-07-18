using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace FortRuntime
{
    public static class CrashReporter
    {
        // P/Invoke Win32 APIs
        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern bool DebugActiveProcess(int dwProcessId);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern bool DebugActiveProcessStop(int dwProcessId);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern bool WaitForDebugEvent(ref DEBUG_EVENT32 lpDebugEvent, int dwMilliseconds);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern bool ContinueDebugEvent(uint dwProcessId, uint dwThreadId, uint dwContinueStatus);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern IntPtr OpenProcess(uint dwDesiredAccess, bool bInheritHandle, int dwProcessId);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern IntPtr OpenThread(uint dwDesiredAccess, bool bInheritHandle, uint dwThreadId);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern bool GetThreadContext(IntPtr hThread, ref CONTEXT32 lpContext);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern bool CloseHandle(IntPtr hObject);

        [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Ansi)]
        private static extern IntPtr CreateToolhelp32Snapshot(uint dwFlags, uint th32ProcessID);

        [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Ansi)]
        private static extern bool Module32First(IntPtr hSnapshot, ref MODULEENTRY32 lpme);

        [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Ansi)]
        private static extern bool Module32Next(IntPtr hSnapshot, ref MODULEENTRY32 lpme);

        [DllImport("user32.dll", CharSet = CharSet.Auto)]
        private static extern int MessageBox(IntPtr hWnd, string text, string caption, uint type);

        [DllImport("dbghelp.dll", SetLastError = true, CharSet = CharSet.Unicode)]
        private static extern bool MiniDumpWriteDump(
            IntPtr hProcess,
            int ProcessId,
            IntPtr hFile,
            int DumpType,
            IntPtr ExceptionParam,
            IntPtr UserStreamParam,
            IntPtr CallbackParam
        );

        [DllImport("dbghelp.dll", SetLastError = true, CharSet = CharSet.Ansi)]
        private static extern bool SymInitialize(IntPtr hProcess, string? UserSearchPath, bool fInvadeProcess);

        [DllImport("dbghelp.dll", SetLastError = true)]
        private static extern bool SymCleanup(IntPtr hProcess);

        [DllImport("dbghelp.dll", SetLastError = true, CharSet = CharSet.Ansi)]
        private static extern bool SymFromAddr(IntPtr hProcess, ulong Address, ref ulong Displacement, ref SYMBOL_INFO Symbol);

        [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Ansi)]
        private static extern IntPtr GetModuleHandle(string lpModuleName);

        [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Ansi)]
        private static extern IntPtr GetProcAddress(IntPtr hModule, string procName);

        [DllImport("dbghelp.dll", SetLastError = true)]
        private static extern bool StackWalk64(uint MachineType, IntPtr hProcess, IntPtr hThread, ref STACKFRAME64 StackFrame, ref CONTEXT32 ContextRecord, IntPtr ReadMemoryRoutine, IntPtr FunctionTableAccessRoutine, IntPtr GetModuleBaseRoutine, IntPtr TranslateAddress);

        // Constants
        private const int INFINITE = -1;
        private const uint DBG_CONTINUE = 0x00010002;
        private const uint DBG_EXCEPTION_NOT_HANDLED = 0x80010001;

        private const uint EXCEPTION_DEBUG_EVENT = 1;
        private const uint EXIT_PROCESS_DEBUG_EVENT = 5;

        private const uint PROCESS_ALL_ACCESS = 0x001F0FFF;
        private const uint THREAD_GET_CONTEXT = 0x0008;

        private const uint TH22CS_SNAPMODULE = 0x00000008;
        private const uint TH32CS_SNAPMODULE32 = 0x00000010;

        private const uint MB_OK = 0x00000000;
        private const uint MB_ICONERROR = 0x00000010;

        private const uint IMAGE_FILE_MACHINE_AMD64 = 0x8664;

        private const uint IMAGE_FILE_MACHINE_I386 = 0x014C;

        // Structure definitions matching x64 layout
        [StructLayout(LayoutKind.Explicit)]
        private struct DEBUG_EVENT32
        {
            [FieldOffset(0)] public uint dwDebugEventCode;
            [FieldOffset(4)] public uint dwProcessId;
            [FieldOffset(8)] public uint dwThreadId;
            [FieldOffset(12)] public EXCEPTION_DEBUG_INFO32 Exception;
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct EXCEPTION_DEBUG_INFO32
        {
            public EXCEPTION_RECORD32 ExceptionRecord;
            public uint dwFirstChance;
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct EXCEPTION_RECORD32
        {
            public uint ExceptionCode;
            public uint ExceptionFlags;
            public IntPtr ExceptionRecordPtr;
            public IntPtr ExceptionAddress;
            public uint NumberParameters;
            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 15)]
            public uint[] ExceptionInformation;
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct FLOATING_SAVE_AREA
        {
            public uint ControlWord;
            public uint StatusWord;
            public uint TagWord;
            public uint ErrorOffset;
            public uint ErrorSelector;
            public uint DataOffset;
            public uint DataSelector;
            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 80)]
            public byte[] RegisterArea;
            public uint Cr0NpxState;
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct CONTEXT32
        {
            public uint ContextFlags;

            public uint Dr0;
            public uint Dr1;
            public uint Dr2;
            public uint Dr3;
            public uint Dr6;
            public uint Dr7;

            public FLOATING_SAVE_AREA FloatSave;

            public uint SegGs;
            public uint SegFs;
            public uint SegEs;
            public uint SegDs;

            public uint Edi;
            public uint Esi;
            public uint Ebx;
            public uint Edx;
            public uint Ecx;
            public uint Eax;

            public uint Ebp;
            public uint Eip;
            public uint SegCs;
            public uint EFlags;
            public uint Esp;
            public uint SegSs;

            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 512)]
            public byte[] ExtendedRegisters;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        private struct MODULEENTRY32
        {
            public uint dwSize;
            public uint th32ModuleID;
            public uint th32ProcessID;
            public uint GlblcntUsage;
            public uint ProccntUsage;
            public IntPtr modBaseAddr;
            public uint modBaseSize;
            public IntPtr hModule;
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 256)]
            public string szModule;
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 260)]
            public string szExePath;
        }

        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Ansi)]
        private struct SYMBOL_INFO
        {
            public uint SizeOfStruct;
            public uint TypeIndex;
            public ulong Reserved1;
            public ulong Reserved2;
            public uint Index;
            public uint Size;
            public ulong ModBase;
            public uint Flags;
            public ulong Value;
            public ulong Address;
            public uint Register;
            public uint Scope;
            public uint Tag;
            public uint NameLen;
            public uint MaxNameLen;
            [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 256)]
            public string Name;
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct STACKFRAME64
        {
            public ADDRESS64 AddrPC;
            public ADDRESS64 AddrReturn;
            public ADDRESS64 AddrFrame;
            public ADDRESS64 AddrStack;
            public ADDRESS64 AddrBSTR;
            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 4)]
            public ulong[] Params;
            public bool Far;
            public bool Virtual;
            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 3)]
            public ulong[] Reserved;
            public ADDRESS64 AddrKDHelp;
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct ADDRESS64
        {
            public ulong Offset;
            public ushort Segment;
            public ushort Reserved;
            public int Mode;
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct MINIDUMP_EXCEPTION_INFORMATION
        {
            public uint ThreadId;
            public IntPtr ExceptionPointers;
            public int ClientPointers; // BOOL
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct EXCEPTION_POINTERS
        {
            public IntPtr ExceptionRecord; // -> EXCEPTION_RECORD32
            public IntPtr ContextRecord;   // -> CONTEXT32
        }

        public static void StartMonitoring(int processId)
        {
            Thread monitorThread = new Thread(() => MonitorLoop(processId));
            monitorThread.IsBackground = true;
            monitorThread.Start();
        }

        private static void MonitorLoop(int processId)
        {
            if (!DebugActiveProcess(processId))
            {
                Console.WriteLine($"[CrashReporter] Failed to attach to process {processId}. Error: {Marshal.GetLastWin32Error()}");
                return;
            }

            Console.WriteLine($"[CrashReporter] Successfully attached and initialized crash reporter for PID {processId}.");

            bool running = true;
            while (running)
            {
                DEBUG_EVENT32 debugEvent = new DEBUG_EVENT32();
                if (WaitForDebugEvent(ref debugEvent, INFINITE))
                {
                    uint continueStatus = DBG_CONTINUE;

                    if (debugEvent.dwDebugEventCode == EXCEPTION_DEBUG_EVENT)
                    {
                        var exception = debugEvent.Exception;
                        bool isFirstChance = exception.dwFirstChance != 0;

                        if (!isFirstChance)
                        {
                            // This is an unhandled exception (second chance)! Generate crash report.
                            try
                            {
                                GenerateCrashReport(processId, debugEvent);
                            }
                            catch (Exception ex)
                            {
                                Console.WriteLine($"[CrashReporter] Error generating crash report: {ex.Message}");
                            }

                            try
                            {
                                using (var proc = Process.GetProcessById(processId))
                                    proc.Kill();
                            }
                            catch (Exception ex)
                            {
                                Console.WriteLine($"[CrashReporter] Failed to kill crashed process: {ex.Message}");
                            }

                            continueStatus = DBG_EXCEPTION_NOT_HANDLED;
                            running = false; // Stop monitoring and detach/terminate
                        }
                        else
                        {
                            // First chance exception - let target handle it
                            continueStatus = DBG_EXCEPTION_NOT_HANDLED;
                        }
                    }
                    else if (debugEvent.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT)
                    {
                        running = false;
                    }

                    ContinueDebugEvent(debugEvent.dwProcessId, debugEvent.dwThreadId, continueStatus);
                }
            }

            DebugActiveProcessStop(processId);
            Console.WriteLine($"[CrashReporter] Stopped monitoring process {processId}.");
        }

        private struct ModuleInfo
        {
            public string Name;
            public string Path;
            public IntPtr BaseAddr;
            public uint Size;
        }

        private static System.Collections.Generic.List<ModuleInfo> GetLoadedModules(int processId)
        {
            var list = new System.Collections.Generic.List<ModuleInfo>();
            IntPtr hSnapshot = CreateToolhelp32Snapshot(TH22CS_SNAPMODULE | TH32CS_SNAPMODULE32, (uint)processId);
            if (hSnapshot == (IntPtr)(-1))
                return list;

            try
            {
                MODULEENTRY32 modEntry = new MODULEENTRY32();
                modEntry.dwSize = (uint)Marshal.SizeOf(typeof(MODULEENTRY32));

                if (Module32First(hSnapshot, ref modEntry))
                {
                    do
                    {
                        list.Add(new ModuleInfo
                        {
                            Name = modEntry.szModule,
                            Path = modEntry.szExePath,
                            BaseAddr = modEntry.modBaseAddr,
                            Size = modEntry.modBaseSize
                        });
                    } while (Module32Next(hSnapshot, ref modEntry));
                }
            }
            finally
            {
                CloseHandle(hSnapshot);
            }

            return list;
        }

        private static string GetExceptionCodeString(uint code)
        {
            return code switch
            {
                0xC0000005 => "EXCEPTION_ACCESS_VIOLATION",
                0xC000008C => "EXCEPTION_ARRAY_BOUNDS_EXCEEDED",
                0x80000003 => "EXCEPTION_BREAKPOINT",
                0xC0000094 => "EXCEPTION_INT_DIVIDE_BY_ZERO",
                0xC00000FD => "EXCEPTION_STACK_OVERFLOW",
                0xC000001D => "EXCEPTION_ILLEGAL_INSTRUCTION",
                0xC0000006 => "EXCEPTION_IN_PAGE_ERROR",
                0xC0000025 => "EXCEPTION_NONCONTINUABLE_EXCEPTION",
                0xC0000026 => "EXCEPTION_INVALID_DISPOSITION",
                0xC0000374 => "EXCEPTION_HEAP_CORRUPTION",
                _ => "UNKNOWN_EXCEPTION"
            };
        }

        private static void GenerateCrashReport(int processId, DEBUG_EVENT32 debugEvent)
        {
            string crashId = Guid.NewGuid().ToString().ToUpper();
            string crashDir = Path.Combine(AppContext.BaseDirectory, "Crashes", crashId);

            Directory.CreateDirectory(crashDir);

            string txtPath = Path.Combine(crashDir, "FortniteGame.txt");
            string dmpPath = Path.Combine(crashDir, "UE4Minidump.dmp");

            IntPtr hProcess = OpenProcess(PROCESS_ALL_ACCESS, false, processId);
            if (hProcess == IntPtr.Zero)
            {
                Console.WriteLine($"[CrashReporter] Failed to open process for diagnostics. Error: {Marshal.GetLastWin32Error()}");
                return;
            }

            try
            {
                // Initialize dbghelp symbols for the remote process
                SymInitialize(hProcess, null, true);

                // Generate FortniteGame.txt
                using (StreamWriter writer = new StreamWriter(txtPath, false, Encoding.UTF8))
                {
                    var exception = debugEvent.Exception.ExceptionRecord;
                    string exceptionName = GetExceptionCodeString(exception.ExceptionCode);
                    writer.WriteLine($"[CrashReporter] Caught unhandled exception (Code: {exceptionName})");

                    if (exception.ExceptionCode == 0xC0000005) // EXCEPTION_ACCESS_VIOLATION
                    {
                        string op = "read";
                        if (exception.NumberParameters >= 1)
                        {
                            ulong opCode = exception.ExceptionInformation[0];
                            if (opCode == 1) op = "write";
                            else if (opCode == 8) op = "execute";
                        }

                        long faultingAddress = 0;
                        if (exception.NumberParameters >= 2)
                        {
                            faultingAddress = (long)exception.ExceptionInformation[1];
                        }
                        writer.WriteLine($"- Trying to {op} 0x{faultingAddress:X16}");
                    }

                    writer.WriteLine(); // Blank line before the stack trace

                    var modules = GetLoadedModules(processId);

                    // Stack Trace
                    IntPtr hThread = OpenThread(THREAD_GET_CONTEXT, false, debugEvent.dwThreadId);
                    if (hThread != IntPtr.Zero)
                    {
                        try
                        {
                            CONTEXT32 ctx = new CONTEXT32();
                            ctx.ContextFlags = 0x10007; // CONTEXT_FULL (x64)
                            if (GetThreadContext(hThread, ref ctx))
                            {
                                STACKFRAME64 frame = new STACKFRAME64();
                                frame.AddrPC.Offset = ctx.Eip;
                                frame.AddrPC.Mode = 3; // AddrModeFlat
                                frame.AddrFrame.Offset = ctx.Ebp;
                                frame.AddrFrame.Mode = 3;
                                frame.AddrStack.Offset = ctx.Esp;
                                frame.AddrStack.Mode = 3;

                                IntPtr hDbgHelp = GetModuleHandle("dbghelp.dll");
                                IntPtr pSymFunctionTableAccess64 = GetProcAddress(hDbgHelp, "SymFunctionTableAccess64");
                                IntPtr pSymGetModuleBase64 = GetProcAddress(hDbgHelp, "SymGetModuleBase64");

                                int depth = 0;
                                while (StackWalk64(
                                    IMAGE_FILE_MACHINE_I386,
                                    hProcess,
                                    hThread,
                                    ref frame,
                                    ref ctx,
                                    IntPtr.Zero,
                                    pSymFunctionTableAccess64,
                                    pSymGetModuleBase64,
                                    IntPtr.Zero))
                                {
                                    if (frame.AddrPC.Offset == 0)
                                        break;

                                    ulong currentEip = frame.AddrPC.Offset;
                                    string modName = "Unknown";
                                    long offset = 0;

                                    foreach (var mod in modules)
                                    {
                                        long ipVal = (long)currentEip;
                                        long baseVal = mod.BaseAddr.ToInt64();
                                        if (ipVal >= baseVal && ipVal < baseVal + mod.Size)
                                        {
                                            modName = mod.Name;
                                            offset = ipVal - baseVal;
                                            break;
                                        }
                                    }

                                    string symbolName = "[unknown]";
                                    ulong displacement = 0;
                                    SYMBOL_INFO symbol = new SYMBOL_INFO();
                                    symbol.SizeOfStruct = 88;
                                    symbol.MaxNameLen = 256;

                                    if (SymFromAddr(hProcess, currentEip, ref displacement, ref symbol))
                                    {
                                        symbolName = $"[{symbol.Name}]";
                                    }

                                    if (modName != "Unknown")
                                    {
                                        writer.WriteLine($"0x{currentEip:X16} ({modName}+0x{offset:X}): {symbolName}");
                                    }
                                    else
                                    {
                                        writer.WriteLine($"0x{currentEip:X16} (Unknown+0x0): {symbolName}");
                                    }

                                    depth++;
                                    if (depth > 128)
                                        break;
                                }
                            }
                            else
                            {
                                writer.WriteLine("Failed to retrieve thread context.");
                            }

                            IntPtr pExceptionRecord = Marshal.AllocHGlobal(Marshal.SizeOf<EXCEPTION_RECORD32>());
                            Marshal.StructureToPtr(exception, pExceptionRecord, false); // exception = debugEvent.Exception.ExceptionRecord

                            IntPtr pContext = Marshal.AllocHGlobal(Marshal.SizeOf<CONTEXT32>());
                            Marshal.StructureToPtr(ctx, pContext, false); // ctx = the CONTEXT32 you got from GetThreadContext

                            EXCEPTION_POINTERS exPtrs = new EXCEPTION_POINTERS
                            {
                                ExceptionRecord = pExceptionRecord,
                                ContextRecord = pContext
                            };
                            IntPtr pExPtrs = Marshal.AllocHGlobal(Marshal.SizeOf<EXCEPTION_POINTERS>());
                            Marshal.StructureToPtr(exPtrs, pExPtrs, false);

                            MINIDUMP_EXCEPTION_INFORMATION exInfo = new MINIDUMP_EXCEPTION_INFORMATION
                            {
                                ThreadId = debugEvent.dwThreadId,
                                ExceptionPointers = pExPtrs,
                                ClientPointers = 0 // false — these pointers are in OUR (debugger) address space, not the target's
                            };
                            IntPtr pExInfo = Marshal.AllocHGlobal(Marshal.SizeOf<MINIDUMP_EXCEPTION_INFORMATION>());
                            Marshal.StructureToPtr(exInfo, pExInfo, false);

                            // Generate UE4Minidump.dmp
                            using (FileStream fs = new FileStream(dmpPath, FileMode.Create, FileAccess.Write, FileShare.None))
                            {
                                bool success = MiniDumpWriteDump(
                                    hProcess,
                                    processId,
                                    fs.SafeFileHandle.DangerousGetHandle(),
                                    0,
                                    pExInfo,      // was IntPtr.Zero
                                    IntPtr.Zero,
                                    IntPtr.Zero
                                );
                            }

                            Marshal.FreeHGlobal(pExceptionRecord);
                            Marshal.FreeHGlobal(pContext);
                            Marshal.FreeHGlobal(pExPtrs);
                            Marshal.FreeHGlobal(pExInfo);
                        }
                        finally
                        {
                            CloseHandle(hThread);
                        }
                    }
                    else
                    {
                        writer.WriteLine("Failed to open crashing thread.");
                    }
                }

                // Notify user
                string message = $"A crash has occurred! A crash report has been automatically generated and saved to:\nCrashes/{crashId}";
                MessageBox(IntPtr.Zero, message, "FortRuntime Crash Reporter", MB_OK | MB_ICONERROR);
            }
            finally
            {
                SymCleanup(hProcess);
                CloseHandle(hProcess);
            }
        }
    }
}
