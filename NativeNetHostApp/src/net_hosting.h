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
// 3. Also you can invoke a managed method without a predefined signature when you haven't specified the delegate typename).
// To do this you pass NetHost::UNMANAGED_CALLERS_ONLY constant to the delegate type name argument, and mark your managed method
// with the UnmanagedCallersOnly attribute. After that you will no longer be able to execute it from the managed code directly,
// but you will be able to execute it from the native code with any signature. It will work on .NET 5 and above.
// 4. You can invoke only static methods this way. Otherwise you should use Marshal.GetFunctionPointerForDelegate<T>() on the
// managed side, so you can have a native pointer to execute any method, not only static ones.

// Additional things will be corrected and improved as long as I find something new.

namespace NetHost
{
	typedef void(__cdecl* ErrorWriterCallback)(const wchar_t* message);
	typedef int(__cdecl *DefaultDNetCallback)(void* args, int sizeBytes);

	// [NET 5+] Use to call a C# method without a predefined signature, but it must be marked with the UnmanagedCallersOnly attribute.
	const wchar_t* const UNMANAGED_CALLERS_ONLY = (const wchar_t*) -1;

	// Initialize the utilities, and find and load hostfxr. Returns true if it's been loaded or already loaded.
	// And returns false if initialization has failed. You can also specify a custom path to the folder containing hostfxr.
	bool Init(std::optional<std::filesystem::path> pathToRuntime = {});

	// Deinitialize the utilities and unload the hostfxr library. Does nothing if it's not inited.
	void Shutdown();

	// Determine is library inited successfully.
	bool IsInited();

	// Specify a callback that will be invoked by .NET hosting components when an error is occured.
	void SetErrorWriter(ErrorWriterCallback callback);

	// A delegate to load assembly and then get function pointer to a managed method. When invoking the delegate, the typename
	// doesn't necessarily have to be stored in the assembly being loaded. It could be in one of the dependencies.
	class LoadAssemblyAndGetFuncPointer_RuntimeDelegate
	{
	private:
		void* delegate;

	public:
		LoadAssemblyAndGetFuncPointer_RuntimeDelegate(void* delegate) : delegate(delegate) {}
		void* operator()(std::wstring_view assemblyPath, std::wstring_view typeName, std::wstring_view methodName, const wchar_t* delegateTypeName = nullptr) const;
	};

	// A delegate to get function pointer to a managed method [NET 5+].
	class GetFuncPointer_RuntimeDelegate
	{
	private:
		void* delegate;

	public:
		GetFuncPointer_RuntimeDelegate(void* delegate) : delegate(delegate) {}
		void* operator()(std::wstring_view typeName, std::wstring_view methodName, const wchar_t* delegateTypeName = nullptr) const;
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

		LoadAssemblyAndGetFuncPointer_RuntimeDelegate GetLoadAssemblyAndGetFuncPointer() const;
		GetFuncPointer_RuntimeDelegate GetGetFuncPointer() const;

		// Run the app (works if you use InitForCommandLine() to host an app).
		int RunApp() const;

		inline bool IsValid() const { return handle != nullptr; }

	private:
		void ThrowIfNoValidHandle() const;
	};

	// Initialize for running an application using the path to an executable from the command args passed (like: TestManaged.dll arg1 arg2).
	// The application part means the managed (.DLL) will work as if you **run** an executable, rather than loading
	// a library/component (i.e. to customize how the managed app is started).
	// ( ! ) Allowed to be called once per process.
	HostContext InitForCommandLine(int argc, const wchar_t** argv);

	// Initialize for running as a component using a runtime configuration.
	// The component part means you **load** a library in addition to the native application, in contrast to running it as a
	// managed sub-application.
	HostContext InitForRuntimeConfig(const wchar_t* configPath);
}
