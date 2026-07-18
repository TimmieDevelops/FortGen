using System;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;

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
        private static extern bool WaitForDebugEvent([Out] byte[] lpDebugEvent, int dwMilliseconds);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern bool ContinueDebugEvent(uint dwProcessId, uint dwThreadId, uint dwContinueStatus);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern IntPtr OpenProcess(uint dwDesiredAccess, bool bInheritHandle, int dwProcessId);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern IntPtr OpenThread(uint dwDesiredAccess, bool bInheritHandle, uint dwThreadId);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern bool GetThreadContext(IntPtr hThread, ref CONTEXT lpContext);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern bool Wow64GetThreadContext(IntPtr hThread, ref CONTEXT lpContext);

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
            ref MINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
            IntPtr UserStreamParam,
            IntPtr CallbackParam
        );

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

        [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Auto)]
        private static extern IntPtr VirtualAllocEx(IntPtr hProcess, IntPtr lpAddress, uint dwSize, uint flAllocationType, uint flProtect);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern bool WriteProcessMemory(IntPtr hProcess, IntPtr lpBaseAddress, byte[] lpBuffer, uint nSize, out IntPtr lpNumberOfBytesWritten);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern bool VirtualFreeEx(IntPtr hProcess, IntPtr lpAddress, uint dwSize, uint dwFreeType);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern bool ReadProcessMemory(IntPtr hProcess, IntPtr lpBaseAddress, [Out] byte[] lpBuffer, int dwSize, out IntPtr lpNumberOfBytesRead);

        // Constants
        private const int INFINITE = -1;
        private const uint DBG_CONTINUE = 0x00010002;
        private const uint DBG_EXCEPTION_NOT_HANDLED = 0x80010001;

        private const uint EXCEPTION_DEBUG_EVENT = 1;
        private const uint CREATE_PROCESS_DEBUG_EVENT = 3;
        private const uint EXIT_PROCESS_DEBUG_EVENT = 5;
        private const uint LOAD_DLL_DEBUG_EVENT = 6;

        private const uint PROCESS_ALL_ACCESS = 0x001F0FFF;
        private const uint THREAD_GET_CONTEXT = 0x0008;

        private const uint TH22CS_SNAPMODULE = 0x00000008;
        private const uint TH32CS_SNAPMODULE32 = 0x00000010;

        private const uint MB_OK = 0x00000000;
        private const uint MB_ICONERROR = 0x00000010;

        // Structured x86 layout for EXCEPTION_RECORD to write to target process
        [StructLayout(LayoutKind.Sequential)]
        private struct EXCEPTION_RECORD32
        {
            public uint ExceptionCode;
            public uint ExceptionFlags;
            public uint ExceptionRecordPtr;
            public uint ExceptionAddress;
            public uint NumberParameters;
            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 15)]
            public uint[] ExceptionInformation;
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct CONTEXT
        {
            public uint ContextFlags;

            // Debug registers
            public uint Dr0;
            public uint Dr1;
            public uint Dr2;
            public uint Dr3;
            public uint Dr6;
            public uint Dr7;

            // Floating point state
            [MarshalAs(UnmanagedType.ByValArray, SizeConst = 112)]
            public byte[] FloatSave;

            // Segment registers
            public uint SegGs;
            public uint SegFs;
            public uint SegEs;
            public uint SegDs;

            // Integer registers
            public uint Edi;
            public uint Esi;
            public uint Ebx;
            public uint Edx;
            public uint Ecx;
            public uint Eax;

            // Control registers
            public uint Ebp;
            public uint Eip;
            public uint SegCs;
            public uint EFlags;
            public uint Esp;
            public uint SegSs;

            // Extended registers
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
        private struct MINIDUMP_EXCEPTION_INFORMATION
        {
            public uint ThreadId;
            public IntPtr ExceptionPointers;
            public int ClientPointers;
        }

        // Parsed information from dynamic DEBUG_EVENT buffer
        private struct ParsedDebugEvent
        {
            public uint dwDebugEventCode;
            public uint dwProcessId;
            public uint dwThreadId;
            public uint ExceptionCode;
            public uint ExceptionFlags;
            public IntPtr ExceptionRecordPtr;
            public IntPtr ExceptionAddress;
            public uint NumberParameters;
            public ulong ExceptionInformation0;
            public ulong ExceptionInformation1;
            public uint dwFirstChance;
            public IntPtr hFile;
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
                byte[] debugEventBuffer = new byte[256];
                if (WaitForDebugEvent(debugEventBuffer, INFINITE))
                {
                    uint continueStatus = DBG_CONTINUE;
                    bool is64Bit = IntPtr.Size == 8;

                    ParsedDebugEvent ev = new ParsedDebugEvent();
                    ev.dwDebugEventCode = BitConverter.ToUInt32(debugEventBuffer, 0);
                    ev.dwProcessId = BitConverter.ToUInt32(debugEventBuffer, 4);
                    ev.dwThreadId = BitConverter.ToUInt32(debugEventBuffer, 8);

                    if (ev.dwDebugEventCode == EXCEPTION_DEBUG_EVENT)
                    {
                        if (is64Bit)
                        {
                            ev.ExceptionCode = BitConverter.ToUInt32(debugEventBuffer, 16);
                            ev.ExceptionFlags = BitConverter.ToUInt32(debugEventBuffer, 20);
                            ev.ExceptionRecordPtr = (IntPtr)BitConverter.ToInt64(debugEventBuffer, 24);
                            ev.ExceptionAddress = (IntPtr)BitConverter.ToInt64(debugEventBuffer, 32);
                            ev.NumberParameters = BitConverter.ToUInt32(debugEventBuffer, 40);
                            ev.ExceptionInformation0 = BitConverter.ToUInt64(debugEventBuffer, 48);
                            ev.ExceptionInformation1 = BitConverter.ToUInt64(debugEventBuffer, 56);
                            ev.dwFirstChance = BitConverter.ToUInt32(debugEventBuffer, 168);
                        }
                        else
                        {
                            ev.ExceptionCode = BitConverter.ToUInt32(debugEventBuffer, 12);
                            ev.ExceptionFlags = BitConverter.ToUInt32(debugEventBuffer, 16);
                            ev.ExceptionRecordPtr = (IntPtr)BitConverter.ToInt32(debugEventBuffer, 20);
                            ev.ExceptionAddress = (IntPtr)BitConverter.ToInt32(debugEventBuffer, 24);
                            ev.NumberParameters = BitConverter.ToUInt32(debugEventBuffer, 28);
                            ev.ExceptionInformation0 = BitConverter.ToUInt32(debugEventBuffer, 32);
                            ev.ExceptionInformation1 = BitConverter.ToUInt32(debugEventBuffer, 36);
                            ev.dwFirstChance = BitConverter.ToUInt32(debugEventBuffer, 92);
                        }

                        // Debugger control events including WoW64 exceptions:
                        // 0x80000003: STATUS_BREAKPOINT
                        // 0x80000004: STATUS_SINGLE_STEP
                        // 0x4000001F: STATUS_WX86_BREAKPOINT
                        // 0x4000001E: STATUS_WX86_SINGLE_STEP
                        if (ev.ExceptionCode == 0x80000003 || ev.ExceptionCode == 0x80000004 ||
                            ev.ExceptionCode == 0x4000001F || ev.ExceptionCode == 0x4000001E)
                        {
                            continueStatus = DBG_CONTINUE;
                        }
                        else
                        {
                            bool isFirstChance = ev.dwFirstChance != 0;

                            if (!isFirstChance)
                            {
                                // This is an unhandled exception (second chance)! Generate crash report.
                                try
                                {
                                    GenerateCrashReport(processId, ev);
                                }
                                catch (Exception ex)
                                {
                                    Console.WriteLine($"[CrashReporter] Error generating crash report: {ex.Message}");
                                }

                                // Terminate the target process so it exits completely, unloading the DLL and releasing the file lock
                                try
                                {
                                    using (var proc = Process.GetProcessById(processId))
                                    {
                                        proc.Kill();
                                    }
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
                    }
                    else if (ev.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT || ev.dwDebugEventCode == LOAD_DLL_DEBUG_EVENT)
                    {
                        ev.hFile = is64Bit ? (IntPtr)BitConverter.ToInt64(debugEventBuffer, 16) : (IntPtr)BitConverter.ToInt32(debugEventBuffer, 12);

                        // Clean up the file handles sent by Windows to avoid handle leaks and file locking
                        if (ev.hFile != IntPtr.Zero && ev.hFile != (IntPtr)(-1))
                        {
                            CloseHandle(ev.hFile);
                        }
                    }
                    else if (ev.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT)
                    {
                        running = false;
                    }

                    ContinueDebugEvent(ev.dwProcessId, ev.dwThreadId, continueStatus);
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
                0x4000001F => "EXCEPTION_WX86_BREAKPOINT",
                0x4000001E => "EXCEPTION_WX86_SINGLE_STEP",
                _ => "UNKNOWN_EXCEPTION"
            };
        }

        private static byte[] StructureToBytes<T>(T str) where T : struct
        {
            int size = Marshal.SizeOf(str);
            byte[] arr = new byte[size];
            IntPtr ptr = Marshal.AllocHGlobal(size);
            try
            {
                Marshal.StructureToPtr(str, ptr, true);
                Marshal.Copy(ptr, arr, 0, size);
            }
            finally
            {
                Marshal.FreeHGlobal(ptr);
            }
            return arr;
        }

        private static void GenerateCrashReport(int processId, ParsedDebugEvent debugEvent)
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
                    string exceptionName = GetExceptionCodeString(debugEvent.ExceptionCode);
                    writer.WriteLine($"[CrashReporter] Caught unhandled exception (Code: {exceptionName})");

                    if (debugEvent.ExceptionCode == 0xC0000005) // EXCEPTION_ACCESS_VIOLATION
                    {
                        string op = "read";
                        if (debugEvent.NumberParameters >= 1)
                        {
                            ulong opCode = debugEvent.ExceptionInformation0;
                            if (opCode == 1) op = "write";
                            else if (opCode == 8) op = "execute";
                        }

                        long faultingAddress = 0;
                        if (debugEvent.NumberParameters >= 2)
                        {
                            faultingAddress = (long)debugEvent.ExceptionInformation1;
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
                            CONTEXT ctx = new CONTEXT();
                            ctx.ContextFlags = 0x00010007; // CONTEXT_FULL (x86)

                            bool contextRetrieved = false;
                            if (IntPtr.Size == 8)
                            {
                                contextRetrieved = Wow64GetThreadContext(hThread, ref ctx);
                            }
                            else
                            {
                                contextRetrieved = GetThreadContext(hThread, ref ctx);
                            }

                            if (contextRetrieved)
                            {
                                IntPtr currentEbp = (IntPtr)ctx.Ebp;
                                IntPtr currentEip = (IntPtr)ctx.Eip;

                                for (int i = 0; i < 64; i++)
                                {
                                    if (currentEip == IntPtr.Zero)
                                        break;

                                    ulong currentEipVal = (ulong)currentEip.ToInt64();
                                    string modName = "Unknown";
                                    long offset = 0;

                                    foreach (var mod in modules)
                                    {
                                        long ipVal = (long)currentEipVal;
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

                                    if (SymFromAddr(hProcess, currentEipVal, ref displacement, ref symbol))
                                    {
                                        symbolName = $"[{symbol.Name}]";
                                    }

                                    writer.WriteLine($"0x{currentEipVal:X16} ({modName}+0x{offset:X}): {symbolName}");

                                    // Read next frame
                                    byte[] frameBuf = new byte[8];
                                    if (ReadProcessMemory(hProcess, currentEbp, frameBuf, 8, out IntPtr bytesRead) && bytesRead.ToInt32() == 8)
                                    {
                                        currentEbp = (IntPtr)BitConverter.ToUInt32(frameBuf, 0);
                                        currentEip = (IntPtr)BitConverter.ToUInt32(frameBuf, 4);
                                    }
                                    else
                                    {
                                        break;
                                    }
                                }

                                // Allocate remote EXCEPTION_RECORD, CONTEXT, and EXCEPTION_POINTERS
                                IntPtr remoteContext = VirtualAllocEx(hProcess, IntPtr.Zero, (uint)Marshal.SizeOf(typeof(CONTEXT)), 0x1000 | 0x2000, 0x04);
                                IntPtr remoteExceptionRecord = VirtualAllocEx(hProcess, IntPtr.Zero, 80, 0x1000 | 0x2000, 0x04);
                                IntPtr remoteExceptionPointers = VirtualAllocEx(hProcess, IntPtr.Zero, 8, 0x1000 | 0x2000, 0x04);

                                if (remoteContext != IntPtr.Zero && remoteExceptionRecord != IntPtr.Zero && remoteExceptionPointers != IntPtr.Zero)
                                {
                                    // 1. Write CONTEXT
                                    byte[] contextBytes = StructureToBytes(ctx);
                                    WriteProcessMemory(hProcess, remoteContext, contextBytes, (uint)contextBytes.Length, out _);

                                    // 2. Build and write EXCEPTION_RECORD32 with strict 32-bit layout
                                    EXCEPTION_RECORD32 rec = new EXCEPTION_RECORD32();
                                    rec.ExceptionCode = debugEvent.ExceptionCode;
                                    rec.ExceptionFlags = debugEvent.ExceptionFlags;
                                    rec.ExceptionRecordPtr = (uint)debugEvent.ExceptionRecordPtr.ToInt64();
                                    rec.ExceptionAddress = (uint)debugEvent.ExceptionAddress.ToInt64();
                                    rec.NumberParameters = debugEvent.NumberParameters;
                                    rec.ExceptionInformation = new uint[15];
                                    rec.ExceptionInformation[0] = (uint)debugEvent.ExceptionInformation0;
                                    rec.ExceptionInformation[1] = (uint)debugEvent.ExceptionInformation1;

                                    byte[] exceptionRecordBytes = StructureToBytes(rec);
                                    WriteProcessMemory(hProcess, remoteExceptionRecord, exceptionRecordBytes, (uint)exceptionRecordBytes.Length, out _);

                                    // 3. Write EXCEPTION_POINTERS (ExceptionRecord, ContextRecord)
                                    byte[] pointersBytes = new byte[8];
                                    Array.Copy(BitConverter.GetBytes(remoteExceptionRecord.ToInt32()), 0, pointersBytes, 0, 4);
                                    Array.Copy(BitConverter.GetBytes(remoteContext.ToInt32()), 0, pointersBytes, 4, 4);
                                    WriteProcessMemory(hProcess, remoteExceptionPointers, pointersBytes, 8, out _);

                                    // Generate perfect UE4Minidump.dmp with remote pointers
                                    using (FileStream fs = new FileStream(dmpPath, FileMode.Create, FileAccess.Write, FileShare.None))
                                    {
                                        MINIDUMP_EXCEPTION_INFORMATION expInfo = new MINIDUMP_EXCEPTION_INFORMATION();
                                        expInfo.ThreadId = debugEvent.dwThreadId;
                                        expInfo.ExceptionPointers = remoteExceptionPointers;
                                        expInfo.ClientPointers = 1;

                                        bool success = MiniDumpWriteDump(
                                            hProcess,
                                            processId,
                                            fs.SafeFileHandle.DangerousGetHandle(),
                                            0, // MiniDumpNormal
                                            ref expInfo,
                                            IntPtr.Zero,
                                            IntPtr.Zero
                                        );
                                    }

                                    // Free remote memory allocated
                                    VirtualFreeEx(hProcess, remoteContext, 0, 0x8000);
                                    VirtualFreeEx(hProcess, remoteExceptionRecord, 0, 0x8000);
                                    VirtualFreeEx(hProcess, remoteExceptionPointers, 0, 0x8000);
                                }
                                else
                                {
                                    // Fallback if VirtualAllocEx fails
                                    using (FileStream fs = new FileStream(dmpPath, FileMode.Create, FileAccess.Write, FileShare.None))
                                    {
                                        MiniDumpWriteDump(
                                            hProcess,
                                            processId,
                                            fs.SafeFileHandle.DangerousGetHandle(),
                                            0,
                                            IntPtr.Zero,
                                            IntPtr.Zero,
                                            IntPtr.Zero
                                        );
                                    }
                                }
                            }
                            else
                            {
                                writer.WriteLine("Failed to retrieve thread context.");
                            }
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
