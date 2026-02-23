#include "host_comm.h"

#include <unordered_map>
#include <stdexcept>

typedef void (DELEGATE_CALLTYPE* HostCommInitCallback)(void* (*utilityLocator)(const char*));

static bool g_isInitialized = false;
static std::unordered_map<std::string, void*> g_nativeUtilities{};

static std::string g_tempCheckingString{};

// Utility locator adapter (will be called from .NET).
static void* GetNativeUtility_Raw(const char* utilityName)
{
	return HostComm::GetNativeUtility(utilityName);
}

void HostComm::Init(const NetHost::HostContext& hostContext, const char_t* assemblyPath, const char_t* assemblyName)
{
	if (g_isInitialized)
		throw std::runtime_error{ "Already initialized" };

	std::basic_string<char_t> fullTypeName{ NH_STR("ManagedApp.HostComm, ") };
	fullTypeName += assemblyName;

	auto loadAndGetFuncPointer = hostContext.GetLoadAssemblyAndGetFuncPointer();
	void* callback = loadAndGetFuncPointer(assemblyPath, fullTypeName.c_str(), NH_STR("Init"), NetHost::UNMANAGED_CALLERS_ONLY);

	((HostCommInitCallback)callback)(&GetNativeUtility_Raw);
	g_isInitialized = true;
}

void HostComm::RegisterNativeUtility(const char* utilityName, void* callback)
{
	if (utilityName == nullptr || *utilityName == '\0')
		throw std::invalid_argument{ "The utility name is null or empty" };
	if (callback == nullptr)
		throw std::invalid_argument{ "The callback is null pointer (not allowed)" };

	g_nativeUtilities.try_emplace(std::move(utilityName), callback);
}

void HostComm::UnregisterNativeUtility(const char* utilityName)
{
	g_nativeUtilities.erase(utilityName);
}

void* HostComm::GetNativeUtility(const char* utilityName)
{
	g_tempCheckingString = utilityName;
	if (!g_nativeUtilities.contains(g_tempCheckingString))
	{
		return nullptr;
	}

	return g_nativeUtilities.at(g_tempCheckingString);
}
