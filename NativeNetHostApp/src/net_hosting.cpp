#include "net_hosting.h"
#include "net_hosting_internal.h"

#include <cassert>
#include <utility>
#include <stdexcept>
#include <iostream>

#include <nethost.h>
#include <hostfxr.h>
#include <coreclr_delegates.h>
#include <error_codes.h>

namespace NetHost
{
	static bool i_isHostFxrLoaded = false;
	static LoadedHostFxr i_loadedFxr{};

	static void ThrowIfUninitialized()
	{
		if (!i_isHostFxrLoaded)
		{
			throw std::runtime_error("Init is required to execute this");
		}
	}

	bool Init(std::optional<std::filesystem::path> pathToRuntime)
	{
		if (i_isHostFxrLoaded)
			return true;

		i_loadedFxr.module = LoadHostFxr(std::move(pathToRuntime));
		if (i_loadedFxr.module == NULL)
		{
			return false;
		}

		i_loadedFxr.funcs.set_error_writer = (hostfxr_set_error_writer_fn)GetProcAddress(i_loadedFxr.module, "hostfxr_set_error_writer");

		i_loadedFxr.funcs.initialize_for_dotnet_command_line = (hostfxr_initialize_for_dotnet_command_line_fn)GetProcAddress(i_loadedFxr.module, "hostfxr_initialize_for_dotnet_command_line");
		i_loadedFxr.funcs.initialize_for_runtime_config = (hostfxr_initialize_for_runtime_config_fn)GetProcAddress(i_loadedFxr.module, "hostfxr_initialize_for_runtime_config");
		i_loadedFxr.funcs.get_runtime_delegate = (hostfxr_get_runtime_delegate_fn)GetProcAddress(i_loadedFxr.module, "hostfxr_get_runtime_delegate");
		i_loadedFxr.funcs.run_app = (hostfxr_run_app_fn)GetProcAddress(i_loadedFxr.module, "hostfxr_run_app");
		i_loadedFxr.funcs.close = (hostfxr_close_fn)GetProcAddress(i_loadedFxr.module, "hostfxr_close");

		i_isHostFxrLoaded = true;
		return true;
	}

	void Shutdown()
	{
		if (!i_isHostFxrLoaded)
			return;

		FreeLibrary(i_loadedFxr.module);
		i_isHostFxrLoaded = false;
	}

	bool IsInited()
	{
		return i_isHostFxrLoaded;
	}

	void SetErrorWriter(ErrorWriterCallback callback)
	{
		ThrowIfUninitialized();
		i_loadedFxr.funcs.set_error_writer(callback);
	}

	HostContext::HostContext(void* handle) : handle(handle)
	{
	}

	HostContext::HostContext(HostContext&& other) noexcept
	{
		handle = std::exchange(other.handle, nullptr);
	}

	void HostContext::Close()
	{
		ThrowIfUninitialized();
		ThrowIfNoValidHandle();

		i_loadedFxr.funcs.close(handle);
		handle = nullptr;
	}

	int HostContext::RunApp() const
	{
		ThrowIfUninitialized();
		ThrowIfNoValidHandle();

		return i_loadedFxr.funcs.run_app(handle);
	}

	void HostContext::ThrowIfNoValidHandle() const
	{
		if (!IsValid())
		{
			throw std::runtime_error("Handle is nullptr");
		}
	}

	void* LoadAssemblyAndGetFuncPointer_RuntimeDelegate::operator()(std::wstring_view assemblyPath, std::wstring_view typeName, std::wstring_view methodName, const wchar_t* delegateTypeName) const
	{
		void* outCallback;
		int result = ((load_assembly_and_get_function_pointer_fn)delegate)(assemblyPath.data(), typeName.data(), methodName.data(), delegateTypeName, nullptr, &outCallback);

		assert(result == S_OK);
		return outCallback;
	}

	void* GetFuncPointer_RuntimeDelegate::operator()(std::wstring_view typeName, std::wstring_view methodName, const wchar_t* delegateTypeName) const
	{
		void* outCallback;
		int result = ((get_function_pointer_fn)delegate)(typeName.data(), methodName.data(), delegateTypeName, nullptr, nullptr, &outCallback);

		assert(result == S_OK);
		return outCallback;
	}

	LoadAssemblyAndGetFuncPointer_RuntimeDelegate HostContext::GetLoadAssemblyAndGetFuncPointer() const
	{
		ThrowIfUninitialized();
		ThrowIfNoValidHandle();

		hostfxr_delegate_type delegateType = hdt_load_assembly_and_get_function_pointer;
		void* delegate = nullptr;

		int result = i_loadedFxr.funcs.get_runtime_delegate(handle, delegateType, &delegate);
		assert(result == S_OK);

		return LoadAssemblyAndGetFuncPointer_RuntimeDelegate(delegate);
	}

	GetFuncPointer_RuntimeDelegate HostContext::GetGetFuncPointer() const
	{
		ThrowIfUninitialized();
		ThrowIfNoValidHandle();

		hostfxr_delegate_type delegateType = hdt_get_function_pointer;
		void* delegate = nullptr;

		int result = i_loadedFxr.funcs.get_runtime_delegate(handle, delegateType, &delegate);
		assert(result == S_OK);

		return GetFuncPointer_RuntimeDelegate(delegate);
	}

	HostContext InitForCommandLine(int argc, const wchar_t** argv)
	{
		ThrowIfUninitialized();

		hostfxr_handle hostHandle;
		int result = i_loadedFxr.funcs.initialize_for_dotnet_command_line(argc, argv, NULL, &hostHandle);
		assert(result == S_OK);

		return HostContext(hostHandle);
	}

	HostContext InitForRuntimeConfig(const wchar_t* configPath)
	{
		ThrowIfUninitialized();

		hostfxr_handle hostHandle;
		int result = i_loadedFxr.funcs.initialize_for_runtime_config(configPath, NULL, &hostHandle);
		assert(result == S_OK);

		return HostContext(hostHandle);
	}

	HMODULE LoadHostFxr(std::optional<std::filesystem::path> pathToRuntime)
	{
		auto dllToLoad = pathToRuntime.value_or(std::filesystem::path());
		if (!pathToRuntime.has_value())
		{
			char_t hostFxrPathBuffer[MAX_PATH];
			size_t buffSize = MAX_PATH;

			int result = get_hostfxr_path(hostFxrPathBuffer, &buffSize, nullptr);
			if (result == StatusCode::CoreHostLibMissingFailure)
			{
				std::wcerr << "The core host lib is missing. Failed to find hostfxr.dll.\n";
				return NULL;
			}

			assert(result != StatusCode::HostApiBufferTooSmall);
			assert(STATUS_CODE_SUCCEEDED(result));

			dllToLoad = std::wstring_view(hostFxrPathBuffer, buffSize);
		}
		else
		{
			if (!std::filesystem::is_directory(dllToLoad))
			{
				std::wcerr << "The path must point to a folder containing runtime, but it's not a directory.\n";
				return NULL;
			}

			dllToLoad /= L"hostfxr.dll";
			if (!std::filesystem::exists(dllToLoad))
			{
				std::wcerr << "Attempt to load a custom provided .NET runtime path has failed. The DLL is not found at path: ";
				std::wcerr << dllToLoad << "\n";
				return NULL;
			}
		}

		HMODULE module = LoadLibraryW(dllToLoad.c_str());
		if (module == NULL)
		{
			std::wcerr << "Failed to load .NET's hostfxr.dll.\n";
			return NULL;
		}

		return module;
	}
}
