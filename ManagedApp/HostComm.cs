using System.Runtime.InteropServices;

namespace ManagedApp
{
    /// <summary>
    /// The main class utility to communicate with the host.
    /// </summary>
    public static class HostComm
    {
        /// <summary>
        /// A native hostcom handler that locates a native registered function and returns it, or nullptr otherwise.
        /// </summary>
        [UnmanagedFunctionPointer(CallingConvention.Cdecl, CharSet = CharSet.Ansi)]
        private delegate IntPtr UtilityLocator(string utilityName);

        public static bool IsInitialized => _isInitialized;

        private static bool _isInitialized = false;
        private static UtilityLocator _utilityLocator;

        /// <summary>
        /// Asks hostcom on the native side to locate a special utility with the specified name,
        /// and converts it to a callable managed delegate.
        /// </summary>
        /// <returns>The managed delegate to call the native utility, or null if not found.</returns>
        public static T GetNativeUtility<T>(string utilityName) where T : Delegate
        {
            ThrowIfUninitialized();

            IntPtr utility = _utilityLocator.Invoke(utilityName);
            if (utility == IntPtr.Zero)
                return null;

            return Marshal.GetDelegateForFunctionPointer<T>(utility);
        }

        /// <summary>
        /// Safe call to <seealso cref="GetNativeUtility{T}(string)"/> which guarantees to return not-null value,
        /// or throw exception otherwise.
        /// </summary>
        /// <returns>The managed delegate to call the utility.</returns>
        public static T RequireNativeUtility<T>(string utilityName) where T : Delegate
        {
            return GetNativeUtility<T>(utilityName) ??
                throw new InvalidOperationException($"Required native utility '{utilityName}' is missing");
        }

        [UnmanagedCallersOnly]
        internal static void Init(IntPtr hostUtilsLocator)
        {
            if (_isInitialized)
                throw new InvalidOperationException("Double init happened");

            if (hostUtilsLocator == IntPtr.Zero)
            {
                Console.WriteLine("[HostComm::Init Failure] The native side didn't provide an utility locator. Crashing...");
                Environment.Exit(-1);
            }

            _isInitialized = true;
            _utilityLocator = Marshal.GetDelegateForFunctionPointer<UtilityLocator>(hostUtilsLocator);
        }

        private static void ThrowIfUninitialized()
        {
            if (!_isInitialized)
                throw new InvalidOperationException("Host communcation hasn't initialized yet");
        }
    }
}
