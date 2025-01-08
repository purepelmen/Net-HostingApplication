#pragma once
#include <string>
#include <string_view>

#include "net_hosting.h"

namespace HostComm
{
	/// Estabilish communication with the managed side. It instructs the managed HostComm to initialize so the managed side
	/// can now be able to communicate back to here (the native side).
	/// @param assemblyPath The path for the main assembly to load.
	/// @param assemblyName The assembly where HostComm managed class resides.
	void Init(const NetHost::HostContext& hostContext, std::wstring_view assemblyPath, std::wstring_view assemblyName);

	void RegisterNativeUtility(std::wstring utilityName, void* callback);
	void UnregisterNativeUtility(const std::wstring& utilityName);

	void* GetNativeUtility(std::wstring_view utilityName);
}
