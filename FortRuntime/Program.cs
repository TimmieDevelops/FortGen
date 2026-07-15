using System;
using System.Runtime.InteropServices;
using FortRuntime;

class Program
{
    // Declare P/Invoke for console control handler
    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern bool SetConsoleCtrlHandler(ConsoleCtrlDelegate handler, bool add);

    private delegate bool ConsoleCtrlDelegate(int ctrlType);
    private const int CTRL_CLOSE_EVENT = 2;

    // Prevent GC from collecting the delegate
    private static ConsoleCtrlDelegate? _consoleCtrlDelegate;

    private static bool HandlerRoutine(int ctrlType)
    {
        if (ctrlType == CTRL_CLOSE_EVENT)
        {
            GameLauncher.QuitGame();
        }
        return false; // Let the system continue with termination
    }

    static void Main(string[] args)
    {
        // Register AppDomain process exit fallback
        AppDomain.CurrentDomain.ProcessExit += (sender, e) =>
        {
            GameLauncher.QuitGame();
        };

        // Register Windows console close event handler
        if (OperatingSystem.IsWindows())
        {
            try
            {
                _consoleCtrlDelegate = new ConsoleCtrlDelegate(HandlerRoutine);
                SetConsoleCtrlHandler(_consoleCtrlDelegate, true);
            }
            catch (Exception ex)
            {
                Console.WriteLine($"Error registering console control handler: {ex.Message}");
            }
        }

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
