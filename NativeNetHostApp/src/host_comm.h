#pragma once
#include <string>

namespace HostComm
{
	void RegisterNativeUtility(std::wstring utilityName, void* callback);
	void UnregisterNativeUtility(const std::wstring& utilityName);

	void* GetNativeUtility(const wchar_t* utilityName);
}
