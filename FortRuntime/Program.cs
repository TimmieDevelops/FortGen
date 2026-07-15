using FortRuntime;

class Program
{
    static void Main(string[] args)
    {
        FolderManager.LoadFolders();
        FolderManager.LoadLastDllPath();

        while (true)
        {
            Console.WriteLine("\n--- FortRuntime Injector/Launcher ---");
            Console.WriteLine("1. Add Fortnite Folder");
            Console.WriteLine("2. Remove Fortnite Folder");
            Console.WriteLine("3. Launch Game (Check status)");
            Console.WriteLine("4. Exit");
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
                FolderManager.CheckLaunchGame();
            }
            else if (Choice == "4")
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