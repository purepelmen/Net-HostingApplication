#include "net_hosting.h"
#include "host_comm.h"

#include <iostream>
#include <filesystem>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

using std::filesystem::path;

static path GetExecutablePath();

static void DebugDNetError(const wchar_t* message);
static void DoTestUtility();

using VoidNoArgPointer = void (*)();
using HostCommInitCallback = void (*)(void* utilityLocator);

int main()
{
    // Essential paths.
    path executableDir = GetExecutablePath();
    path pathToRuntimeConfig = executableDir / L"ManagedApp.runtimeconfig.json";
    path assemblyPath = executableDir / L"ManagedApp.dll";

    if (!NetHost::Init())
    {
        std::cout << "Failed to initialize .NET host.\n";
        return -1;
    }
    
    std::cout << "The .NET hosting environment has been initialized.\n";
    NetHost::SetErrorWriter(DebugDNetError);

    NetHost::HostContext context = NetHost::InitForRuntimeConfig(pathToRuntimeConfig.c_str());
    auto loadAndGetDelegate = context.GenerateLoadAssemblyAndGetFuncPointerDelegate();

    void* callback;

    std::cout << "Switching to the .NET world...\n";
    callback = loadAndGetDelegate.Perform(assemblyPath.native(), L"ManagedApp.HostComm, ManagedApp", L"Init", NetHost::UNMANAGED_CALLERS_ONLY);
    ((HostCommInitCallback)callback)(&HostComm::GetNativeUtility);

    HostComm::RegisterNativeUtility(L"test_utility", &DoTestUtility);

    callback = loadAndGetDelegate.Perform(assemblyPath.native(), L"ManagedApp.Program, ManagedApp", L"Main", L"System.Action, mscorlib");
    ((VoidNoArgPointer)callback)();

    context.Close();
    NetHost::Shutdown();
}

path GetExecutablePath()
{
    wchar_t buffer[MAX_PATH]{};

    std::filesystem::path exePath = std::wstring(buffer, GetModuleFileName(NULL, buffer, MAX_PATH));
    return exePath.parent_path();
}

void DebugDNetError(const wchar_t* message)
{
    std::cout << "[.NET Error Writer Handler] -> ";
    std::wcout << message << std::endl;
}

void DoTestUtility()
{
    std::cout << "You've invoked the test utility on the native side.\n";
}
