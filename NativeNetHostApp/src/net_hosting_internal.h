#pragma once
#include <optional>
#include <filesystem>

#include <hostfxr.h>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

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
		HMODULE module = NULL;
		HostFxrFuncs funcs;
	};

	// Find and load the hostfxr by using nethost, or load it directly if the path is specified.
	HMODULE LoadHostFxr(std::optional<std::filesystem::path> pathToRuntime);
}
