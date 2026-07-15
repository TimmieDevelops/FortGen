using System;
using System.IO;
using System.Diagnostics;
using System.Collections.Generic;

namespace FortRuntime
{
    public static class GameLauncher
    {
        public static void CheckLaunchGame()
        {
            var folders = FolderManager.Folders;
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
                        Process.Start(startInfo);
                        Console.WriteLine("Game process started successfully.");

                        // Prompt for DLL injection stub
                        Console.Write("Do you want to inject a DLL? (y/n): ");
                        string? injectResponse = Console.ReadLine();
                        if (injectResponse?.Trim().ToLower() == "y")
                        {
                            Console.WriteLine("[Info] DLL injection is not implemented in this application. Please implement your custom loader/injection logic here.");
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

        public static void QuitGame()
        {
            Console.WriteLine("Searching for active FortniteClient-Win32-Shipping.exe processes...");
            try
            {
                // Find all processes with name "FortniteClient-Win32-Shipping"
                Process[] processes = Process.GetProcessesByName("FortniteClient-Win32-Shipping");
                if (processes.Length == 0)
                {
                    Console.WriteLine("No running FortniteClient-Win32-Shipping processes found.");
                    return;
                }

                Console.WriteLine($"Found {processes.Length} running process(es). Attempting to quit...");
                foreach (var process in processes)
                {
                    try
                    {
                        Console.WriteLine($"Killing process PID: {process.Id}");
                        process.Kill();
                        process.WaitForExit(5000);
                        Console.WriteLine($"Process {process.Id} exited successfully.");
                    }
                    catch (Exception ex)
                    {
                        Console.WriteLine($"Failed to terminate process {process.Id}: {ex.Message}");
                    }
                }
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Error searching for game processes: {ex.Message}");
            }
        }
    }
}
