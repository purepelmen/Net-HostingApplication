#include "host_comm.h"
#include <unordered_map>
#include <stdexcept>

static bool g_isInitialized = false;
static std::unordered_map<std::wstring, void*> g_nativeUtilities{};

static std::wstring g_tempCheckingString{};

void HostComm::RegisterNativeUtility(std::wstring utilityName, void* callback)
{
	if (utilityName.length() == 0)
		throw std::invalid_argument{ "The utility name is unfortunatelly" };
	if (callback == nullptr)
		throw std::invalid_argument{ "The callback is null pointer (not allowed)" };

	g_nativeUtilities.try_emplace(std::move(utilityName), callback);
}

void HostComm::UnregisterNativeUtility(const std::wstring& utilityName)
{
	g_nativeUtilities.erase(utilityName);
}

void* HostComm::GetNativeUtility(const wchar_t* utilityName)
{
	g_tempCheckingString = utilityName;
	if (!g_nativeUtilities.contains(g_tempCheckingString))
	{
		return nullptr;
	}

	return g_nativeUtilities.at(g_tempCheckingString);
}
