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

        public static string? GetLastDllPath() => lastDllPath;
        public static List<string> GetFolders() => folders;
    }
}
