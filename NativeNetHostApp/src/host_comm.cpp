#include "host_comm.h"

#include <unordered_map>
#include <stdexcept>

using HostCommInitCallback = void (*)(void* utilityLocator);

static bool g_isInitialized = false;
static std::unordered_map<std::wstring, void*> g_nativeUtilities{};

static std::wstring g_tempCheckingString{};

static void* GetNativeUtility_Raw(const wchar_t* utilityName)
{
	return HostComm::GetNativeUtility(utilityName);
}

void HostComm::Init(const NetHost::HostContext& hostContext, std::wstring_view assemblyPath, std::wstring_view assemblyName)
{
	auto loadAndGetFuncPointer = hostContext.GetLoadAssemblyAndGetFuncPointer();

	std::wstring fullTypeName{ L"ManagedApp.HostComm, " };
	fullTypeName += assemblyName;

	void* callback = loadAndGetFuncPointer(assemblyPath, fullTypeName, L"Init", NetHost::UNMANAGED_CALLERS_ONLY);
	((HostCommInitCallback)callback)(&GetNativeUtility_Raw);
}

void HostComm::RegisterNativeUtility(std::wstring utilityName, void* callback)
{
	if (utilityName.length() == 0)
		throw std::invalid_argument{ "The utility name is empty" };
	if (callback == nullptr)
		throw std::invalid_argument{ "The callback is null pointer (not allowed)" };

	g_nativeUtilities.try_emplace(std::move(utilityName), callback);
}

void HostComm::UnregisterNativeUtility(const std::wstring& utilityName)
{
	g_nativeUtilities.erase(utilityName);
}

void* HostComm::GetNativeUtility(std::wstring_view utilityName)
{
	g_tempCheckingString = utilityName;
	if (!g_nativeUtilities.contains(g_tempCheckingString))
	{
		return nullptr;
	}

	return g_nativeUtilities.at(g_tempCheckingString);
}
