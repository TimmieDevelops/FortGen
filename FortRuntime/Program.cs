using FortRuntime;
using System.Runtime.InteropServices;

class Program
{
    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern bool SetConsoleCtrlHandler(ConsoleCtrlDelegate handler, bool add);

    private delegate bool ConsoleCtrlDelegate(int ctrlType);
    private const int CTRL_CLOSE_EVENT = 2;

    private static ConsoleCtrlDelegate? _consoleCtrlDelegate;

    private static bool HandlerRoutine(int ctrlType)
    {
        if (ctrlType == CTRL_CLOSE_EVENT)
            GameLauncher.QuitGame();

        return false;
    }

    static void Main(string[] args)
    {
        AppDomain.CurrentDomain.ProcessExit += (sender, e) =>
        {
            GameLauncher.QuitGame();
        };

        if (OperatingSystem.IsWindows())
        {
            try
            {
                _consoleCtrlDelegate = new ConsoleCtrlDelegate(HandlerRoutine);
                SetConsoleCtrlHandler(_consoleCtrlDelegate, true);
            } catch (Exception ex)
            {
                Console.WriteLine($"Error registering console control handler: {ex.Message}");
            }
        }

        FolderManager.LoadFolders();
        FolderManager.LoadLastDllPath();

        while (true)
        {
            Console.WriteLine("\n--- FortRuntime Injector/Launcher ---");
            Console.WriteLine("1. Add Fortnite Folder");
            Console.WriteLine("2. Remove Fortnite Folder");
            Console.WriteLine("3. Launch Game (Check status)");
            Console.WriteLine("4. Quit Game");
            Console.WriteLine("5. Exit");
            Console.Write("Choose an option: ");

            string? Choice = Console.ReadLine();

            if (Choice == "1")
            {
                FolderManager.AddFolder();
            }
            else if (Choice == "2")
            {
                FolderManager.RemoveFolder();
            }
            else if (Choice == "3")
            {
                GameLauncher.CheckLaunchGame();
            }
            else if (Choice == "4")
            {
                GameLauncher.QuitGame();
            }
            else if (Choice == "5")
            {
                GameLauncher.QuitGame();
                break;
            }
            else
            {
                Console.WriteLine("Invalid option. Please try again.");
            }
        }
    }
}