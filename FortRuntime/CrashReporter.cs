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
        private static extern bool WaitForDebugEvent(ref DEBUG_EVENT lpDebugEvent, int dwMilliseconds);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern bool ContinueDebugEvent(uint dwProcessId, uint dwThreadId, uint dwContinueStatus);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern IntPtr OpenProcess(uint dwDesiredAccess, bool bInheritHandle, int dwProcessId);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern IntPtr OpenThread(uint dwDesiredAccess, bool bInheritHandle, uint dwThreadId);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern bool GetThreadContext(IntPtr hThread, ref CONTEXT lpContext);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern bool ReadProcessMemory(IntPtr hProcess, IntPtr lpBaseAddress, [Out] byte[] lpBuffer, int dwSize, out IntPtr lpNumberOfBytesRead);

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

        // Structure definitions matching x86 layout
        [StructLayout(LayoutKind.Explicit)]
        private struct DEBUG_EVENT
        {
            [FieldOffset(0)]
            public uint dwDebugEventCode;
            [FieldOffset(4)]
            public uint dwProcessId;
            [FieldOffset(8)]
            public uint dwThreadId;

            [FieldOffset(12)]
            public EXCEPTION_DEBUG_INFO Exception;
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct EXCEPTION_DEBUG_INFO
        {
            public EXCEPTION_RECORD ExceptionRecord;
            public uint dwFirstChance;
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct EXCEPTION_RECORD
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
                DEBUG_EVENT debugEvent = new DEBUG_EVENT();
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

        private static void GenerateCrashReport(int processId, DEBUG_EVENT debugEvent)
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
                // Generate FortniteGame.txt
                using (StreamWriter writer = new StreamWriter(txtPath, false, Encoding.UTF8))
                {
                    writer.WriteLine($"Crash Time: {DateTime.Now:yyyy-MM-dd HH:mm:ss}");

                    var exception = debugEvent.Exception.ExceptionRecord;
                    writer.WriteLine($"Exception Code: 0x{exception.ExceptionCode:X8} ({GetExceptionCodeString(exception.ExceptionCode)})");
                    writer.WriteLine($"Exception Address: 0x{exception.ExceptionAddress.ToInt64():X8}");

                    writer.WriteLine("\n[Process Information]");
                    writer.WriteLine($"Process ID: {processId}");

                    try
                    {
                        using (Process proc = Process.GetProcessById(processId))
                        {
                            writer.WriteLine($"Process Path: {proc.MainModule?.FileName}");
                        }
                    }
                    catch { }

                    // Loaded Modules
                    writer.WriteLine("\n[Loaded Modules]");
                    var modules = GetLoadedModules(processId);
                    foreach (var mod in modules)
                    {
                        writer.WriteLine($"0x{mod.BaseAddr.ToInt64():X8} - 0x{(mod.BaseAddr.ToInt64() + mod.Size):X8}  {mod.Name} ({mod.Path})");
                    }

                    // Stack Trace
                    writer.WriteLine("\n[Stack Trace]");
                    IntPtr hThread = OpenThread(THREAD_GET_CONTEXT, false, debugEvent.dwThreadId);
                    if (hThread != IntPtr.Zero)
                    {
                        try
                        {
                            CONTEXT ctx = new CONTEXT();
                            ctx.ContextFlags = 0x00010007; // CONTEXT_FULL (x86)
                            if (GetThreadContext(hThread, ref ctx))
                            {
                                IntPtr currentEbp = (IntPtr)ctx.Ebp;
                                IntPtr currentEip = (IntPtr)ctx.Eip;

                                for (int i = 0; i < 64; i++)
                                {
                                    if (currentEip == IntPtr.Zero)
                                        break;

                                    string modName = "Unknown";
                                    long offset = 0;

                                    foreach (var mod in modules)
                                    {
                                        long ipVal = currentEip.ToInt64();
                                        long baseVal = mod.BaseAddr.ToInt64();
                                        if (ipVal >= baseVal && ipVal < baseVal + mod.Size)
                                        {
                                            modName = mod.Name;
                                            offset = ipVal - baseVal;
                                            break;
                                        }
                                    }

                                    if (modName != "Unknown")
                                    {
                                        writer.WriteLine($"  Frame {i}: 0x{currentEip.ToInt64():X8} ({modName} + 0x{offset:X})");
                                    }
                                    else
                                    {
                                        writer.WriteLine($"  Frame {i}: 0x{currentEip.ToInt64():X8}");
                                    }

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

                // Generate UE4Minidump.dmp
                using (FileStream fs = new FileStream(dmpPath, FileMode.Create, FileAccess.Write, FileShare.None))
                {
                    bool success = MiniDumpWriteDump(
                        hProcess,
                        processId,
                        fs.SafeFileHandle.DangerousGetHandle(),
                        0, // MiniDumpNormal
                        IntPtr.Zero,
                        IntPtr.Zero,
                        IntPtr.Zero
                    );
                }

                // Notify user
                string message = $"A crash has occurred! A crash report has been automatically generated and saved to:\nCrashes/{crashId}";
                MessageBox(IntPtr.Zero, message, "FortRuntime Crash Reporter", MB_OK | MB_ICONERROR);
            }
            finally
            {
                CloseHandle(hProcess);
            }
        }
    }
}
