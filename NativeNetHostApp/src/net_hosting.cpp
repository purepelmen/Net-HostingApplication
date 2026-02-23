#include "net_hosting.h"

#include <cassert>
#include <utility>
#include <stdexcept>
#include <iostream>

#include <nethost.h>
#include <hostfxr.h>
#include <coreclr_delegates.h>
#include <error_codes.h>

#if _WIN32
	#define WIN32_LEAN_AND_MEAN
	#include <Windows.h>

	#define SHAREDLIB_LOAD(name) LoadLibrary((name))
	#define SHAREDLIB_FREE(handle) FreeLibrary((handle))
	#define SHAREDLIB_SYM(handle, name) GetProcAddress(handle, name)

	typedef HMODULE sharedlib_t;
#else
	#include <dlfcn.h>
	#include <limits.h>

	// Adapting Linux's PATH_MAX to Windows's MAX_PATH.
	#define MAX_PATH PATH_MAX

	#define SHAREDLIB_LOAD(name) dlopen((name), RTLD_LAZY)
	#define SHAREDLIB_FREE(handle) dlclose((handle))
	#define SHAREDLIB_SYM(handle, name) dlsym(handle, name)

	typedef void* sharedlib_t;
#endif

namespace NetHost
{
	struct HostFxrFuncs
	{
		hostfxr_set_error_writer_fn set_error_writer;

		hostfxr_initialize_for_dotnet_command_line_fn initialize_for_dotnet_command_line;
		hostfxr_initialize_for_runtime_config_fn initialize_for_runtime_config;
		hostfxr_get_runtime_delegate_fn get_runtime_delegate;
		hostfxr_run_app_fn run_app;
		hostfxr_close_fn close;
	};

	struct LoadedHostFxr
	{
		sharedlib_t module = NULL;
		HostFxrFuncs funcs;
	};

	static bool i_isHostFxrLoaded = false;
	static LoadedHostFxr i_loadedFxr{};

	// Find and load the hostfxr by using nethost, or load it directly if the path is specified.
	static sharedlib_t LoadHostFxr(std::optional<std::filesystem::path> pathToRuntime);

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

		i_loadedFxr.funcs.set_error_writer = (hostfxr_set_error_writer_fn)SHAREDLIB_SYM(i_loadedFxr.module, "hostfxr_set_error_writer");

		i_loadedFxr.funcs.initialize_for_dotnet_command_line = (hostfxr_initialize_for_dotnet_command_line_fn)SHAREDLIB_SYM(i_loadedFxr.module, "hostfxr_initialize_for_dotnet_command_line");
		i_loadedFxr.funcs.initialize_for_runtime_config = (hostfxr_initialize_for_runtime_config_fn)SHAREDLIB_SYM(i_loadedFxr.module, "hostfxr_initialize_for_runtime_config");
		i_loadedFxr.funcs.get_runtime_delegate = (hostfxr_get_runtime_delegate_fn)SHAREDLIB_SYM(i_loadedFxr.module, "hostfxr_get_runtime_delegate");
		i_loadedFxr.funcs.run_app = (hostfxr_run_app_fn)SHAREDLIB_SYM(i_loadedFxr.module, "hostfxr_run_app");
		i_loadedFxr.funcs.close = (hostfxr_close_fn)SHAREDLIB_SYM(i_loadedFxr.module, "hostfxr_close");

		i_isHostFxrLoaded = true;
		return true;
	}

	void Shutdown()
	{
		if (!i_isHostFxrLoaded)
			return;

		SHAREDLIB_FREE(i_loadedFxr.module);
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

	void* rd_LoadAssemblyAndGetFuncPointer::operator()(const char_t* assemblyPath, const char_t* typeName, const char_t* methodName, const char_t* delegateTypeName) const
	{
		void* outCallback;
		int result = ((load_assembly_and_get_function_pointer_fn)delegate)(assemblyPath, typeName, methodName, delegateTypeName, nullptr, &outCallback);

		assert(result == StatusCode::Success);
		return outCallback;
	}

	void* rd_GetFuncPointer::operator()(const char_t* typeName, const char_t* methodName, const char_t* delegateTypeName) const
	{
		void* outCallback;
		int result = ((get_function_pointer_fn)delegate)(typeName, methodName, delegateTypeName, nullptr, nullptr, &outCallback);

		assert(result == StatusCode::Success);
		return outCallback;
	}

	rd_LoadAssemblyAndGetFuncPointer HostContext::GetLoadAssemblyAndGetFuncPointer() const
	{
		ThrowIfUninitialized();
		ThrowIfNoValidHandle();

		hostfxr_delegate_type delegateType = hdt_load_assembly_and_get_function_pointer;
		void* delegate = nullptr;

		int result = i_loadedFxr.funcs.get_runtime_delegate(handle, delegateType, &delegate);
		if (!STATUS_CODE_SUCCEEDED(result))
			return {};

		return { delegate };
	}

	rd_GetFuncPointer HostContext::GetGetFuncPointer() const
	{
		ThrowIfUninitialized();
		ThrowIfNoValidHandle();

		hostfxr_delegate_type delegateType = hdt_get_function_pointer;
		void* delegate = nullptr;

		int result = i_loadedFxr.funcs.get_runtime_delegate(handle, delegateType, &delegate);
		if (!STATUS_CODE_SUCCEEDED(result))
			return {};

		return { delegate };
	}

	HostContext NewContextForCommandLine(int argc, const char_t** argv)
	{
		ThrowIfUninitialized();

		hostfxr_handle hostHandle;
		int result = i_loadedFxr.funcs.initialize_for_dotnet_command_line(argc, argv, NULL, &hostHandle);
		assert(result == StatusCode::Success);

		return HostContext(hostHandle);
	}

	HostContext NewContextForRuntimeConfig(const char_t* configPath)
	{
		ThrowIfUninitialized();

		hostfxr_handle hostHandle;
		int result = i_loadedFxr.funcs.initialize_for_runtime_config(configPath, NULL, &hostHandle);
		assert(result == StatusCode::Success);

		return HostContext(hostHandle);
	}

	sharedlib_t LoadHostFxr(std::optional<std::filesystem::path> pathToRuntime)
	{
		auto dllToLoad = pathToRuntime.value_or(std::filesystem::path());
		if (!pathToRuntime.has_value())
		{
			char_t hostFxrPathBuffer[MAX_PATH];
			size_t buffSize = MAX_PATH;

			int result = get_hostfxr_path(hostFxrPathBuffer, &buffSize, nullptr);
			if (result == StatusCode::CoreHostLibMissingFailure)
			{
				std::cerr << "The core host lib is missing. Failed to find hostfxr.dll.\n";
				return NULL;
			}

			assert(result != StatusCode::HostApiBufferTooSmall);
			assert(STATUS_CODE_SUCCEEDED(result));

			dllToLoad = std::basic_string_view<char_t>(hostFxrPathBuffer, buffSize);
		}
		else
		{
			if (!std::filesystem::is_directory(dllToLoad))
			{
				std::cerr << "The path must point to a folder containing runtime, but it's not a directory.\n";
				return NULL;
			}

			dllToLoad /= L"hostfxr.dll";
			if (!std::filesystem::exists(dllToLoad))
			{
				std::cerr << "Attempt to load a custom provided .NET runtime path has failed. The DLL is not found at path: ";
				std::cerr << dllToLoad << "\n";
				return NULL;
			}
		}

		sharedlib_t module = SHAREDLIB_LOAD(dllToLoad.c_str());
		if (module == NULL)
		{
			std::cerr << "Failed to load .NET's hostfxr.dll.\n";
			return NULL;
		}

		return module;
	}
}
