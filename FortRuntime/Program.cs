using System;
using FortRuntime;

class Program
{
    static void Main(string[] args)
    {
        FolderManager.LoadFolders();

        while (true)
        {
            Console.WriteLine("\n--- FortRuntime Injector/Launcher ---");
            Console.WriteLine("1. Add Fortnite Folder");
            Console.WriteLine("2. Remove Fortnite Folder");
            Console.WriteLine("3. Launch Game (Check status)");
            Console.WriteLine("4. Quit Game");
            Console.WriteLine("5. Exit");
            Console.Write("Choose an option: ");
            string? choice = Console.ReadLine();

            if (choice == "1")
            {
                FolderManager.AddFolder();
            }
            else if (choice == "2")
            {
                FolderManager.RemoveFolder();
            }
            else if (choice == "3")
            {
                GameLauncher.CheckLaunchGame();
            }
            else if (choice == "4")
            {
                GameLauncher.QuitGame();
            }
            else if (choice == "5")
            {
                break;
            }
            else
            {
                Console.WriteLine("Invalid option. Please try again.");
            }
        }
    }
}
