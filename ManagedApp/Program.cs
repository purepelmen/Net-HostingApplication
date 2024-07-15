namespace ManagedApp
{
    public class Program
    {
        public static void Main()
        {
            Console.WriteLine($"Hello, World in C#! The HostComm::IsInitialized is {HostComm.IsInitialized}.");

            Action testUtility = HostComm.RequireNativeUtility<Action>("test_utility");
            testUtility.Invoke();
        }
    }
}
