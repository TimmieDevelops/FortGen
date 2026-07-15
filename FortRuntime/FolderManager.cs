using System;
using System.IO;
using System.Diagnostics;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;

namespace FortRuntime
{
    public static class FolderManager
    {
        private static readonly string ConfigFilePath = Path.Combine(AppContext.BaseDirectory, "fortnite_folders.txt");
        private static List<string> folders = new List<string>();

        [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Ansi)]
        private static extern IntPtr OpenProcess(uint processAccess, bool bInheritHandle, int processId);

        [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Ansi)]
        private static extern IntPtr VirtualAllocEx(IntPtr hProcess, IntPtr lpAddress, uint dwSize, uint flAllocationType, uint flProtect);

        [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Ansi)]
        private static extern bool WriteProcessMemory(IntPtr hProcess, IntPtr lpBaseAddress, byte[] lpBuffer, uint nSize, out IntPtr lpNumberOfBytesWritten);

        [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Ansi)]
        private static extern IntPtr CreateRemoteThread(IntPtr hProcess, IntPtr lpThreadAttributes, uint dwStackSize, IntPtr lpStartAddress, IntPtr lpParameter, uint dwCreationFlags, out IntPtr lpThreadId);

        [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Ansi)]
        private static extern IntPtr GetProcAddress(IntPtr hModule, string lpProcName);

        [DllImport("kernel32.dll", SetLastError = true, CharSet = CharSet.Ansi)]
        private static extern IntPtr GetModuleHandle(string lpModuleName);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern bool CloseHandle(IntPtr hObject);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern uint WaitForSingleObject(IntPtr hHandle, uint dwMilliseconds);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern bool GetExitCodeThread(IntPtr hThread, out uint lpExitCode);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern bool VirtualFreeEx(IntPtr hProcess, IntPtr lpAddress, uint dwSize, uint dwFreeType);

        private const uint PROCESS_CREATE_THREAD = 0x0002;
        private const uint PROCESS_QUERY_INFORMATION = 0x0400;
        private const uint PROCESS_VM_OPERATION = 0x0008;
        private const uint PROCESS_VM_WRITE = 0x0020;
        private const uint PROCESS_VM_READ = 0x0010;

        private const uint MEM_COMMIT = 0x00001000;
        private const uint MEM_RESERVE = 0x00002000;
        private const uint MEM_RELEASE = 0x00008000;
        private const uint PAGE_READWRITE = 0x04;

        private const uint INFINITE = 0xFFFFFFFF;
        private const uint WAIT_OBJECT_0 = 0x00000000;

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
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Error loading folders: {ex.Message}");
            }
        }

        private static void SaveFolders()
        {
            try
            {
                File.WriteAllLines(ConfigFilePath, folders);
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Error saving folders: {ex.Message}");
            }
        }

        public static void AddFolder()
        {
            Console.Write("Enter Fortnite folder path: ");
            string? folderPath = Console.ReadLine();
            if (string.IsNullOrWhiteSpace(folderPath))
            {
                Console.WriteLine("Folder path cannot be empty.");
                return;
            }

            folderPath = folderPath.Trim();
            string exePath = Path.Combine(folderPath, "FortniteClient-Win32-Shipping.exe");

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

            Console.Write("Select a folder number to remove: ");
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
                string exePath = Path.Combine(folder, "FortniteClient-Win32-Shipping.exe");
                if (File.Exists(exePath))
                {
                    targetFolder = folder;
                    targetExePath = exePath;
                    Console.WriteLine($"FortniteClient-Win32-Shipping.exe exists in folder: {folder}");
                    break;
                }
            }

            if (targetFolder != null && targetExePath != null)
            {
                Console.WriteLine("You can launch into the game!");
                Console.Write("Do you want to launch the game now? (y/n): ");
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
                            UseShellExecute = false
                        };
                        Process? gameProcess = Process.Start(startInfo);
                        if (gameProcess == null)
                        {
                            Console.WriteLine("[Error] Failed to start game process.");
                            return;
                        }
                        Console.WriteLine("Game process started successfully.");

                        // Prompt for DLL injection
                        Console.Write("Do you want to inject a DLL? (y/n): ");
                        string? injectResponse = Console.ReadLine();
                        if (injectResponse?.Trim().ToLower() == "y")
                        {
                            Console.Write("Enter path to the DLL (default: FortGen.dll): ");
                            string? dllPathInput = Console.ReadLine();
                            string dllPath = string.IsNullOrWhiteSpace(dllPathInput) ? "FortGen.dll" : dllPathInput.Trim();
                            InjectDll(gameProcess, dllPath);
                        }
                    }
                    catch (Exception ex)
                    {
                        Console.WriteLine($"Error launching game: {ex.Message}");
                    }
                }
            }
            else
            {
                Console.WriteLine("FortniteClient-Win32-Shipping.exe don't exist. You cannot launch into the game.");
            }
        }

        private static void InjectDll(Process targetProcess, string dllPath)
        {
            if (targetProcess == null)
            {
                Console.WriteLine("[Error] Target process is null.");
                return;
            }

            if (!RuntimeInformation.IsOSPlatform(OSPlatform.Windows))
            {
                Console.WriteLine("[Error] DLL injection is only supported on Windows.");
                return;
            }

            string fullDllPath = Path.GetFullPath(dllPath);
            if (!File.Exists(fullDllPath))
            {
                Console.WriteLine($"[Error] DLL file not found at: {fullDllPath}");
                return;
            }

            Console.WriteLine($"[Info] Attempting to inject: {fullDllPath}");
            Console.WriteLine($"[Info] Target process ID: {targetProcess.Id}");

            IntPtr hProcess = OpenProcess(
                PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ,
                false,
                targetProcess.Id
            );

            if (hProcess == IntPtr.Zero)
            {
                int error = Marshal.GetLastWin32Error();
                Console.WriteLine($"[Error] Failed to open target process. Error code: {error}");
                return;
            }

            IntPtr lpAddress = IntPtr.Zero;
            IntPtr hThread = IntPtr.Zero;
            try
            {
                byte[] dllPathBytes = Encoding.ASCII.GetBytes(fullDllPath + "\0");
                uint size = (uint)dllPathBytes.Length;

                lpAddress = VirtualAllocEx(hProcess, IntPtr.Zero, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
                if (lpAddress == IntPtr.Zero)
                {
                    int error = Marshal.GetLastWin32Error();
                    Console.WriteLine($"[Error] Failed to allocate memory in target process. Error code: {error}");
                    return;
                }

                if (!WriteProcessMemory(hProcess, lpAddress, dllPathBytes, size, out IntPtr bytesWritten))
                {
                    int error = Marshal.GetLastWin32Error();
                    Console.WriteLine($"[Error] Failed to write memory in target process. Error code: {error}");
                    return;
                }

                IntPtr hKernel32 = GetModuleHandle("kernel32.dll");
                if (hKernel32 == IntPtr.Zero)
                {
                    int error = Marshal.GetLastWin32Error();
                    Console.WriteLine($"[Error] Failed to get module handle for kernel32.dll. Error code: {error}");
                    return;
                }

                IntPtr lpLoadLibraryA = GetProcAddress(hKernel32, "LoadLibraryA");
                if (lpLoadLibraryA == IntPtr.Zero)
                {
                    int error = Marshal.GetLastWin32Error();
                    Console.WriteLine($"[Error] Failed to get address for LoadLibraryA. Error code: {error}");
                    return;
                }

                hThread = CreateRemoteThread(hProcess, IntPtr.Zero, 0, lpLoadLibraryA, lpAddress, 0, out IntPtr threadId);
                if (hThread == IntPtr.Zero)
                {
                    int error = Marshal.GetLastWin32Error();
                    Console.WriteLine($"[Error] Failed to create remote thread in target process. Error code: {error}");
                    return;
                }

                Console.WriteLine($"[Info] Remote thread created successfully. Thread ID: {threadId}");
                Console.WriteLine("[Info] Waiting for injection thread to finish...");

                uint waitResult = WaitForSingleObject(hThread, INFINITE);
                if (waitResult == WAIT_OBJECT_0)
                {
                    if (GetExitCodeThread(hThread, out uint exitCode))
                    {
                        if (exitCode == 0)
                        {
                            Console.WriteLine("[Error] LoadLibraryA returned NULL - DLL failed to load in the target process.");
                            Console.WriteLine("[Diag] Possible causes:");
                            Console.WriteLine("       - Architecture mismatch: The target process is 32-bit (x86), ensure the DLL is compiled as 32-bit (x86).");
                            Console.WriteLine("       - Bitness of Injector: The injector is running in a different bitness (ensure both are x86).");
                            Console.WriteLine("       - Missing dependencies: The DLL relies on runtime libraries or dependencies that cannot be found/loaded.");
                        }
                        else
                        {
                            Console.WriteLine($"[Success] DLL successfully injected! LoadLibraryA returned: 0x{exitCode:X}");
                        }
                    }
                    else
                    {
                        int error = Marshal.GetLastWin32Error();
                        Console.WriteLine($"[Error] Failed to retrieve thread exit code. Error code: {error}");
                    }
                }
                else
                {
                    Console.WriteLine($"[Warning] Wait for thread timed out or failed. Wait result: {waitResult}");
                }
            }
            finally
            {
                if (hThread != IntPtr.Zero)
                {
                    CloseHandle(hThread);
                }

                if (lpAddress != IntPtr.Zero)
                {
                    VirtualFreeEx(hProcess, lpAddress, 0, MEM_RELEASE);
                }

                if (hProcess != IntPtr.Zero)
                {
                    CloseHandle(hProcess);
                }
            }
        }
    }
}
