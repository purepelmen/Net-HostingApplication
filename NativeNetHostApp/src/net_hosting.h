#pragma once
#include <string>
#include <optional>
#include <filesystem>

// About executing managed methods from the native side.
// If a method has a very simple signature (void Method()), it can be invoked by just specifying type name, and method name. 
// However for a more complicated methods there are some important things to keep in mind.
// 1. To be able to pass any arguments directly when invoking retrieved function pointer you must specify delegate type name,
// which actually points to a delegate type defined in the .NET assembly.
// 2. Otherwise the signature you use when calling the managed method from the native side is: int func(void* args, int sizeBytes).
// The same signature should be in the managed method: int func(IntPtr args, int sizeBytes).
// This specifies a pointer to method arguments, and the size of arguments in bytes. Should the return type be int? I don't remember.
// 3. Also you can invoke a managed method without a predefined signature (when you haven't specified the delegate typename), or 
// the delegate type name. To do this you pass NetHost::UNMANAGED_CALLERS_ONLY constant to the delegate type name argument, and mark
// your managed method with UnmanagedCallersOnly attribute. After that you will no longer be able to execute it from the managed code
// directly, but you will be able to execute it from the native code with any signature.

// Additional things will be corrected and improved as long as I find something new.

namespace NetHost
{
	typedef void(__cdecl* ErrorWriterCallback)(const wchar_t* message);
	typedef int(__cdecl *DefaultDNetCallback)(void* args, int sizeBytes);

	// Use to call a C# method without a predefined signature. But it must be marked with [UnmanagedCallersOnly].
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

	class RuntimeDelegate
	{
	protected:
		int delegateType;
		void* delegate;

	public:
		RuntimeDelegate(int delegateType, void* delegate);

		void* LoadAndGet(std::wstring_view assemblyPath, std::wstring_view typeName, std::wstring_view methodName, const wchar_t* delegateTypeName = nullptr) const;
		void* Get(std::wstring_view typeName, std::wstring_view methodName, const wchar_t* delegateTypeName = nullptr) const;

	private:
		void AssertTypeIs(int type) const;
	};

	class HostContext
	{
	private:
		void* handle = nullptr;

	public:
		HostContext(void* handle);
		HostContext(HostContext&& other) noexcept;
		HostContext(const HostContext& handle) = delete;

		void Close();

		int RunApp() const;
		RuntimeDelegate LoadAssemblyAndGetFuncPointer() const;
		RuntimeDelegate GetFuncPointer() const;

		HostContext& operator=(const HostContext& handle) = delete;

	private:
		void ThrowIfNoValidHandle() const;
	};

	// Initialize for running an application using the path to an executable.
	// The application part means the managed (.DLL) will work as if it's an executable, rather than a component (library).
	HostContext InitForCommandLine(int argc, const wchar_t** argv);

	// Initialize for running as a component using a runtime configuration.
	// The component part means it's a library loaded in addition to the native application, in contrast to loading it as a
	// managed sub-application.
	HostContext InitForRuntimeConfig(const wchar_t* configPath);
}
