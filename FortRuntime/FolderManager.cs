using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Reflection.Metadata.Ecma335;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace FortRuntime
{
    public static class FolderManager
    {
        private static readonly string ConfigFilePath = Path.Combine(AppContext.BaseDirectory, "fortnite_folders.txt");
        private static List<string> folders = new List<string>();

        private static readonly string DLLConfigFilePath = Path.Combine(AppContext.BaseDirectory, "last_dll_path.txt");
        private static string? lastDllPath = null;

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern IntPtr OpenProcess(uint processAccess, bool bInheritHandle, int processId);

        [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Auto)]
        private static extern IntPtr VirtualAllocEx(IntPtr hProcess, IntPtr lpAddress, uint dwSize, uint flAllocationType, uint flProtect);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern bool WriteProcessMemory(IntPtr hProcess, IntPtr lpBaseAddress, byte[] lpBuffer, uint nSize, out IntPtr lpNumberOfBytesWritten);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern IntPtr GetModuleHandle(string lpModuleName);

        [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Ansi, ExactSpelling = true)]
        private static extern IntPtr GetProcAddress(IntPtr hModule, string procName);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern IntPtr CreateRemoteThread(IntPtr hProcess, IntPtr lpThreadAttributes, uint dwStackSize,
            IntPtr lpStartAddress, IntPtr lpParameter, uint dwCreationFlags, out IntPtr lpThreadId);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern uint WaitForSingleObject(IntPtr hHandle, uint dwMilliseconds);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern bool GetExitCodeThread(IntPtr hThread, out uint lpExitCode);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern bool CloseHandle(IntPtr hObject);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern bool IsWow64Process(IntPtr hProcess, out bool wow64Process);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern IntPtr CreateToolhelp32Snapshot(uint dwFlags, uint th32ProcessID);

        [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Ansi)]
        private static extern bool Module32First(IntPtr hSnapshot, ref MODULEENTRY32 lpme);

        [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Ansi)]
        private static extern bool Module32Next(IntPtr hSnapshot, ref MODULEENTRY32 lpme);

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

        private const uint TH32CS_SNAPMODULE = 0x00000008;
        private const uint TH32CS_SNAPMODULE32 = 0x00000010;

        private const uint PROCESS_ALL_ACCESS = 0x001F0FFF;
        private const uint MEM_COMMIT = 0x1000;
        private const uint MEM_RESERVE = 0x2000;
        private const uint PAGE_READWRITE = 0x04;
        private const uint INFINITE = 0xFFFFFFFF;

        private static IntPtr GetRemoteModuleBase(int processId, string moduleName)
        {
            IntPtr hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, (uint)processId);
            if (hSnapshot == (IntPtr)(-1))
                return IntPtr.Zero;

            try
            {
                MODULEENTRY32 modEntry = new MODULEENTRY32();
                modEntry.dwSize = (uint)Marshal.SizeOf(typeof(MODULEENTRY32));

                if (Module32First(hSnapshot, ref modEntry))
                {
                    do
                    {
                        if (string.Equals(modEntry.szModule, moduleName, StringComparison.OrdinalIgnoreCase))
                        {
                            return modEntry.modBaseAddr;
                        }
                    } while (Module32First(hSnapshot, ref modEntry));
                }
            } 
            finally
            {
                CloseHandle(hSnapshot);
            }

            return IntPtr.Zero;
        }

        private static uint GetExportRVA(string dllPath, string functionName)
        {

            try
            {
                byte[] fileBytes = File.ReadAllBytes(dllPath);

                int ntHeaderOffset = BitConverter.ToInt32(fileBytes, 0x3C);
                int optHeaderOffset = ntHeaderOffset + 4 + 20;
                ushort magic = BitConverter.ToUInt16(fileBytes, optHeaderOffset);

                int exportDirRvaOffset = optHeaderOffset + (magic == 0x10B ? 96 : 112);
                uint exportDirRVA = BitConverter.ToUInt32(fileBytes, exportDirRvaOffset);
                uint exportDirSize = BitConverter.ToUInt32(fileBytes, exportDirRvaOffset + 4);

                if (exportDirRVA == 0) return 0;

                ushort numSections = BitConverter.ToUInt16(fileBytes, ntHeaderOffset + 4 + 2);
                ushort sizeOptHeader = BitConverter.ToUInt16(fileBytes, ntHeaderOffset + 4 + 16);
                int sectionHeaderOffset = ntHeaderOffset + 4 + 20 + sizeOptHeader;

                uint exportFileOffset = 0;

                for (int i = 0; i < numSections; i++)
                {
                    int secOffset = sectionHeaderOffset + (i * 40);
                    uint virtualAddress = BitConverter.ToUInt32(fileBytes, secOffset + 12);
                    uint virtualSize = BitConverter.ToUInt32(fileBytes, secOffset + 8);
                    uint pointerToRawData = BitConverter.ToUInt32(fileBytes, secOffset + 20);

                    if (exportDirRVA >= virtualAddress && exportDirRVA < virtualAddress + virtualSize)
                    {
                        exportFileOffset = pointerToRawData + (exportDirRVA - virtualAddress);
                        break;
                    }
                }

                if (exportFileOffset == 0) return 0;

                uint numberOfNames = BitConverter.ToUInt32(fileBytes, (int)exportFileOffset + 24);
                uint addressOfFunctionsRVA = BitConverter.ToUInt32(fileBytes, (int)exportFileOffset + 28);
                uint addressOfNamesRVA = BitConverter.ToUInt32(fileBytes, (int)exportFileOffset + 32);
                uint addressOfNameOrdinalsRVA = BitConverter.ToUInt32(fileBytes, (int)exportFileOffset + 36);

                uint addressOfFunctionsOffset = 0;
                uint addressOfNamesOffset = 0;
                uint addressOfNameOrdinalsOffset = 0;

                for (int i = 0; i < numSections; i++)
                {
                    int secOffset = sectionHeaderOffset + (i * 40);
                    uint virtualAddress = BitConverter.ToUInt32(fileBytes, secOffset + 12);
                    uint virtualSize = BitConverter.ToUInt32(fileBytes, secOffset + 8);
                    uint pointerToRawData = BitConverter.ToUInt32(fileBytes, secOffset + 20);

                    if (addressOfFunctionsRVA >= virtualAddress && addressOfFunctionsRVA < virtualAddress + virtualSize)
                        addressOfFunctionsOffset = pointerToRawData + (addressOfFunctionsRVA - virtualAddress);

                    if (addressOfNamesRVA >= virtualAddress && addressOfNamesRVA < virtualAddress + virtualSize)
                        addressOfNamesOffset = pointerToRawData + (addressOfNamesRVA - virtualAddress);

                    if (addressOfNameOrdinalsRVA >= virtualAddress && addressOfNameOrdinalsRVA < virtualAddress + virtualSize)
                        addressOfNameOrdinalsOffset = pointerToRawData + (addressOfNameOrdinalsRVA - virtualAddress);
                }

                if (addressOfFunctionsOffset == 0 || addressOfNamesOffset == 0 || addressOfNameOrdinalsOffset == 0)
                    return 0;

                for (uint i = 0; i < numberOfNames; i++)
                {
                    uint nameRVA = BitConverter.ToUInt32(fileBytes, (int)addressOfNamesOffset + (int)(i * 4));
                    uint nameOffset = 0;
                    for (int s = 0; s < numSections; s++)
                    {
                        int secOffset = sectionHeaderOffset + (s * 40);
                        uint virtualAddress = BitConverter.ToUInt32(fileBytes, secOffset + 12);
                        uint virtualSize = BitConverter.ToUInt32(fileBytes, secOffset + 8);
                        uint pointerToRawData = BitConverter.ToUInt32(fileBytes, secOffset + 20);

                        if (nameRVA >= virtualAddress && nameRVA < virtualAddress + virtualSize)
                        {
                            nameOffset = pointerToRawData + (nameRVA - virtualAddress);
                            break;
                        }
                    }

                    if (nameOffset == 0) continue;

                    int length = 0;
                    while (fileBytes[nameOffset + length] != 0) length++;
                    string name = Encoding.ASCII.GetString(fileBytes, (int)nameOffset, length);

                    if (name == functionName)
                    {
                        ushort ordinal = BitConverter.ToUInt16(fileBytes, (int)addressOfNameOrdinalsOffset + (int)(i * 2));
                        uint functionRVA = BitConverter.ToUInt32(fileBytes, (int)addressOfFunctionsOffset + (int)(ordinal * 4));
                        return functionRVA;
                    }
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Error parsing PE exports: {ex.Message}");
            }

            return 0;
        }

        private static bool InjectDLL(int processId, string dllPath)
        {
            if (!File.Exists(dllPath))
            {
                Console.WriteLine($"DLL not found: {dllPath}");
                return false;
            }

            IntPtr hProcess = OpenProcess(PROCESS_ALL_ACCESS, false, processId);
            if (hProcess == IntPtr.Zero)
            {
                Console.WriteLine($"Failed to open process (PID {processId}). Error: {Marshal.GetLastWin32Error()}");
                return false;
            }

            try
            {
                byte[] dllPathBytes = Encoding.Default.GetBytes(dllPath + "\0");
                uint size = (uint)dllPathBytes.Length;

                IntPtr allocMemAddress = VirtualAllocEx(hProcess, IntPtr.Zero, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
                if (allocMemAddress == IntPtr.Zero)
                {
                    Console.WriteLine($"VirtualAllocEx failed. Error: {Marshal.GetLastWin32Error()}");
                    return false;
                }

                if (!WriteProcessMemory(hProcess, allocMemAddress, dllPathBytes, size, out _))
                {
                    Console.WriteLine($"WriteProcessMemory failed. Error: {Marshal.GetLastWin32Error()}");
                    return false;
                }

                IntPtr loadLibraryAddr = IntPtr.Zero;

                bool isTargetWow64 = false;
                if (IsWow64Process(hProcess, out bool wow64) && wow64)
                    isTargetWow64 = true;

                if (Environment.Is64BitProcess && isTargetWow64)
                {
                    IntPtr targetkernel32base = GetRemoteModuleBase(processId, "kernel32.dll");
                    if (targetkernel32base == IntPtr.Zero)
                    {
                        Console.WriteLine("Failed to find kernel32.dll in target 32-bit process.");
                        return false;
                    }

                    string sysWow64Path = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.Windows), "SysWOW64\\kernel32.dll");
                    if (!File.Exists(sysWow64Path))
                    {
                        Console.WriteLine("SysWOW64\\kernel32.dll not found.");
                        return false;
                    }

                    uint loadLibraryRVA = GetExportRVA(sysWow64Path, "LoadLibraryA");
                    if (loadLibraryRVA == 0)
                    {
                        Console.WriteLine("Failed to find LoadLibraryA export RVA in 32-bit kernel32.dll");
                        return false;
                    }

                    loadLibraryAddr = (IntPtr)((long)targetkernel32base + loadLibraryRVA);
                    Console.WriteLine($"Resolved 32-bit LoadLibraryA address in remote process: 0x{loadLibraryAddr.ToInt64():X}");
                }
                else
                {
                    IntPtr kernel32Handle = GetModuleHandle("kernel32.dll");
                    loadLibraryAddr = GetProcAddress(kernel32Handle, "LoadLibraryA");
                }

                if (loadLibraryAddr == IntPtr.Zero)
                {
                    Console.WriteLine("Failed to resolve LoadLibraryA address");
                    return false;
                }

                IntPtr hThread = CreateRemoteThread(hProcess, IntPtr.Zero, 0, loadLibraryAddr, allocMemAddress, 0, out _);
                if (hThread == IntPtr.Zero)
                {
                    Console.WriteLine($"CreateRemoteThread failed. Error: {Marshal.GetLastWin32Error()}");
                    return false;
                }

                WaitForSingleObject(hThread, INFINITE);

                GetExitCodeThread(hThread, out uint moduleHandle);
                CloseHandle(hThread);

                if (moduleHandle == 0)
                {
                    Console.WriteLine("loadLibraryA returned NULL - DLL failed to load in the target process.");
                    return false;
                }

                Console.WriteLine($"DLL injected successfully! Module base in remote process: 0x{moduleHandle:X}");
                return true;
            } 
            finally
            {
                CloseHandle(hProcess);
            }
        }

        public static void LoadFolders()
        {
            try
            {
                if (File.Exists(ConfigFilePath))
                {
                    var lines = File.ReadAllLines(ConfigFilePath);
                    folders.Clear();
                    foreach (var line in lines)
                    {
                        if (!string.IsNullOrWhiteSpace(line))
                        {
                            folders.Add(line.Trim());
                        }
                    }
                }
            } catch (Exception ex)
            {
                Console.WriteLine($"Error loading folders: {ex.Message}");
            }
        }

        public static void LoadLastDllPath()
        {
            try
            {
                if (File.Exists(DLLConfigFilePath))
                {
                    string content = File.ReadAllText(DLLConfigFilePath).Trim();
                    lastDllPath = string.IsNullOrWhiteSpace(content) ? null : content;
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Error loading last DLL path: {ex.Message}");
            }
        }

        public static void SaveFolders()
        {
            try
            {
                File.WriteAllLines(ConfigFilePath, folders);
            } catch (Exception ex)
            {
                Console.WriteLine($"Error saving folders: {ex.Message}");
            }
        }

        public static void SaveLastDLLPath(string dllPath)
        {
            try
            {
                File.WriteAllText(DLLConfigFilePath, dllPath);
                lastDllPath = dllPath;
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Error saving last DLL path: {ex.Message}");
            }
        }

        public static string? GetLastDllPath() => lastDllPath;

        public static void AddFolder()
        {
            Console.Write("Entry Fortnite folder path: ");

            string? folderPath = Console.ReadLine();
            if (string.IsNullOrWhiteSpace(folderPath))
            {
                Console.WriteLine("Folder path cannot be empty.");
                return;
            }

            folderPath = folderPath.Trim();

            string exePath = Path.Combine(folderPath, "FortniteGame\\Binaries\\Win32\\FortniteClient-Win32-Shipping.exe");

            if (!File.Exists(exePath))
            {
                Console.WriteLine("FortniteClient-Win32-Shipping.exe don't exist");
                return;
            }

            if (folders.Contains(folderPath))
            {
                Console.WriteLine("Folder is already added.");
                return;
            }

            folders.Add(folderPath);
            SaveFolders();
            Console.WriteLine("Folder added successfully!");
        }

        public static void RemoveFolder()
        {
            if (folders.Count == 0)
            {
                Console.WriteLine("No folders to remove.");
                return;
            }

            Console.WriteLine("Saved folders:");
            for (int i = 0; i < folders.Count; i++)
            {
                Console.WriteLine($"{i + 1}. {folders[i]}");
            }

            Console.WriteLine("Select a folder number to remove: ");
            string? input = Console.ReadLine();
            if (int.TryParse(input, out int index) && index >= 1 && index <= folders.Count)
            {
                string removed = folders[index - 1];
                folders.RemoveAt(index - 1);
                SaveFolders();
                Console.WriteLine($"Removed folder: {removed}");
            }
            else
            {
                Console.WriteLine("Invalid selection.");
            }
        }

        public static void CheckLaunchGame()
        {
            if (folders.Count == 0)
            {
                Console.WriteLine("No folders configured. Please add a Fortnite folder first.");
                return;
            }

            string? targetFolder = null;
            string? targetExePath = null;

            foreach (var folder in folders)
            {
                string exePath = Path.Combine(folder, "FortniteGame\\Binaries\\Win32\\FortniteClient-Win32-Shipping.exe");
                if (File.Exists(exePath))
                {
                    targetFolder = folder;
                    targetExePath = exePath;
                    Console.WriteLine($"FortniteClient-Win32-Shipping.exe exists in folder: {folder}");
                }
            }

            if (targetFolder != null && targetExePath != null)
            {
                Console.WriteLine("You can launch into the game!");
                Console.WriteLine("Do you want to launch the game now? (y/n): ");
                string? response = Console.ReadLine();
                if (response?.Trim().ToLower() == "y")
                {
                    try
                    {
                        Console.WriteLine($"Launching Fortnite from: {targetExePath}");
                        ProcessStartInfo startInfo = new ProcessStartInfo
                        {
                            FileName = targetExePath,
                            WorkingDirectory = targetFolder,
                            UseShellExecute = false,
                        };
                        Process proc = Process.Start(startInfo);
                        Console.Write("Do you want to inject a DLL? (y/n): ");
                        string? injectresponse = Console.ReadLine();
                        if (injectresponse?.Trim().ToLower() == "y")
                        {
                            string? dllPath;
                            string? lastPath = GetLastDllPath();

                            if (!String.IsNullOrWhiteSpace(lastPath))
                            {
                                Console.Write($"Enter path to DLL (press Enter to reuse: {lastPath}): ");
                            }
                            else
                            {
                                Console.Write("Enter path to DLL: ");
                            }

                            string? input = Console.ReadLine();

                            if (string.IsNullOrWhiteSpace(input) && !string.IsNullOrWhiteSpace(lastPath))
                            {
                                dllPath = lastPath;
                            }
                            else
                            {
                                dllPath = input?.Trim();
                            }

                            if (!string.IsNullOrWhiteSpace(dllPath) && proc != null)
                            {
                                System.Threading.Thread.Sleep(1000);
                                bool success = InjectDLL(proc.Id, dllPath);
                                if (success)
                                {
                                    SaveLastDLLPath(dllPath);
                                }
                            }
                            else
                            {
                                Console.WriteLine("Invalid DLL path or process handle.");
                            }
                        }
                    } catch (Exception ex)
                    {
                        Console.WriteLine($"Error launching game: {ex.Message}");
                    }
                }
            } else
            {
                Console.WriteLine("FortniteClient-Win32-Shipping.exe don't exist. You cannot launch into the game.");
            }
        }
    }
}
