using System.Runtime.InteropServices;

namespace ManagedApp
{
    public class Program
    {
        public delegate void TestDelegate();

        public static int Main(IntPtr args, int sizeBytes)
        {
            Console.WriteLine($"Hello, World in C#! The HostComm::IsInitialized is {HostComm.IsInitialized}.");
            return 0;
        }
        
        public static void Bark()
        {
            Console.WriteLine("Bark!");
        }

        [UnmanagedCallersOnly]
        public static void DoAnother()
        {
            Console.WriteLine("You're doing something another!");
        }
    }
}
