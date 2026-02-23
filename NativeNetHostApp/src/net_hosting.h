#pragma once
#include <string>
#include <optional>
#include <filesystem>

// About executing managed methods from the native side.
// A method can be invoked using type name, and method name. However...
// 1. Usually you must specify delegate type name, which actually points to a delegate type defined in a .NET assembly.
// 2. Otherwise, if you haven't specified it, the signature you use when calling the managed method from the native side is:
// int func(void* args, int sizeBytes). The same signature should be in the managed method: int Func(IntPtr args, int sizeBytes).
// This specifies a pointer to method arguments, and the size of arguments in bytes. Should the return type be int? I don't remember.
// 3. Also you can invoke a managed method without a predefined signature when you haven't specified the delegate typename.
// To do this you pass NetHost::UNMANAGED_CALLERS_ONLY constant to the delegate type name argument, and mark your managed method
// with the UnmanagedCallersOnly attribute. After that you will no longer be able to execute it from the managed code directly,
// but you will be able to execute it from the native code with any signature. It will work on .NET 5 and above.
// 4. You can invoke only static methods this way. Otherwise you should use Marshal.GetFunctionPointerForDelegate<T>() on the
// managed side, so you can have a native pointer to execute any method, not only static ones.

// Additional things will be corrected and improved as long as I find something new.

// ---
// Delegates returned by CoreCLR are __stdcall on x86.
#if defined(_WIN32)
	#define DELEGATE_CALLTYPE __stdcall
	#define ERRWRITER_CALLTYPE __cdecl

	#define NH_STR(x) L##x
	#ifdef _WCHAR_T_DEFINED
		typedef wchar_t char_t;
	#else
		typedef unsigned short char_t;
	#endif
#else
	#define DELEGATE_CALLTYPE
	#define ERRWRITER_CALLTYPE

	#define NH_STR(x) x
	typedef char char_t;
#endif

namespace NetHost
{
	typedef void (ERRWRITER_CALLTYPE *ErrorWriterCallback)(const char_t* message);
	typedef int (DELEGATE_CALLTYPE *DefaultDNetCallback)(void* args, int sizeBytes);

	// [NET 5+] Use to call a C# method without a predefined signature, but it must be marked with the UnmanagedCallersOnly attribute.
	const char_t* const UNMANAGED_CALLERS_ONLY = (const char_t*)-1;

	// Initialize the utilities, and find and load hostfxr. Returns true if it's already loaded, and false if initialization has failed. 
	/// @param pathToRuntime Optionally a custom path may be specified to the folder containing hostfxr which will skip using nethost to find the hostfxr library.
	bool Init(std::optional<std::filesystem::path> pathToRuntime = {});

	// Deinitialize the utilities and unload the hostfxr library. Does nothing if it's not inited.
	void Shutdown();

	// Determine is library inited successfully.
	bool IsInited();

	// Specify a callback that will be invoked by .NET hosting components when an error is occured.
	void SetErrorWriter(ErrorWriterCallback callback);

	// Base class for all runtime delegates, which are basically functors that wrap runtime delegates that hostfxr provides via `get_runtime_delegate`.
	class RuntimeDelegate
	{
	protected:
		void* delegate = nullptr;

	public:
		RuntimeDelegate(void* delegate) : delegate(delegate) {}
		RuntimeDelegate() = default;

		bool HasValue() const { return delegate != nullptr; }
	};

	// A delegate to load assembly and then get function pointer to a managed method. When invoking the delegate, the typename
	// doesn't necessarily have to be stored in the assembly being loaded. It could be in one of the dependencies.
	class rd_LoadAssemblyAndGetFuncPointer : public RuntimeDelegate
	{
	public:
		using RuntimeDelegate::RuntimeDelegate;

		void* operator()(const char_t* assemblyPath, const char_t* typeName, const char_t* methodName, const char_t* delegateTypeName = nullptr) const;
	};

	// A delegate to get function pointer to a managed method [NET 5+].
	class rd_GetFuncPointer : public RuntimeDelegate
	{
	public:
		using RuntimeDelegate::RuntimeDelegate;

		void* operator()(const char_t* typeName, const char_t* methodName, const char_t* delegateTypeName = nullptr) const;
	};

	class HostContext
	{
	private:
		void* handle = nullptr;

	public:
		HostContext(void* handle);
		HostContext(HostContext&& other) noexcept;

		HostContext(const HostContext& handle) = delete;
		HostContext& operator=(const HostContext& handle) = delete;

		void Close();

		rd_LoadAssemblyAndGetFuncPointer GetLoadAssemblyAndGetFuncPointer() const;
		rd_GetFuncPointer GetGetFuncPointer() const;

		// Run the managed app (works if you use InitForCommandLine() to host an app) and return when it exits.
		int RunApp() const;

		// A host context is valid if it wasn't disposed or moved to another object, meaning that you can safely work with it.
		inline bool IsValid() const { return handle != nullptr; }

	private:
		void ThrowIfNoValidHandle() const;
	};

	// Initialize for running an application using the path to an executable from the command args passed (like: TestManaged.dll arg1 arg2).
	// The application part means the managed (.DLL) will work as if you **run** an executable, rather than loading
	// a library/component (i.e. to customize how the managed app is started).
	// ( ! ) Allowed to be called once per process.
	HostContext NewContextForCommandLine(int argc, const char_t** argv);

	// Initialize for running as a component using a runtime configuration.
	// The component part means you **load** a library in addition to the native application, in contrast to running it as a
	// managed sub-application.
	HostContext NewContextForRuntimeConfig(const char_t* configPath);
}
