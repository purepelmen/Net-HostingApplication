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

int main()
{
    SetEnvironmentVariable(L"_COREHOST_TRACE", L"1");

    // Essential paths.
    path executableDir = GetExecutablePath();
    path pathToRuntimeConfig = executableDir / L"ManagedApp.runtimeconfig.json";
    path assemblyPath = executableDir / L"ManagedApp.dll";

    if (!NetHost::Init())
    {
        std::cout << "Failed to initialize .NET host.\n";
        return -1;
    }
    
    NetHost::HostContext context = NetHost::InitForRuntimeConfig(pathToRuntimeConfig.c_str());
    NetHost::SetErrorWriter(DebugDNetError);
    std::cout << "The .NET hosting environment has been initialized.\n";

    std::cout << "Switching to the .NET world...\n";

    HostComm::Init(context, assemblyPath.native(), L"ManagedApp");
    HostComm::RegisterNativeUtility(L"test_utility", &DoTestUtility);

    auto loadAndGetDelegate = context.GetLoadAssemblyAndGetFuncPointer();

    void* callback;
    callback = loadAndGetDelegate(assemblyPath.native(), L"ManagedApp.Program, ManagedApp", L"Main", L"System.Action, mscorlib");
    
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
