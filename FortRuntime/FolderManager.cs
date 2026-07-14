using System;
using System.IO;
using System.Collections.Generic;

namespace FortRuntime
{
    public static class FolderManager
    {
        private static readonly string ConfigFilePath = Path.Combine(AppContext.BaseDirectory, "fortnite_folders.txt");
        private static List<string> folders = new List<string>();

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

            bool anyExist = false;
            foreach (var folder in folders)
            {
                string exePath = Path.Combine(folder, "FortniteClient-Win32-Shipping.exe");
                if (File.Exists(exePath))
                {
                    anyExist = true;
                    Console.WriteLine($"FortniteClient-Win32-Shipping.exe exists in folder: {folder}");
                }
            }

            if (anyExist)
            {
                Console.WriteLine("You can launch into the game!");
            }
            else
            {
                Console.WriteLine("FortniteClient-Win32-Shipping.exe don't exist. You cannot launch into the game.");
            }
        }
    }
}
