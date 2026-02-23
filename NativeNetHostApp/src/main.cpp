#include "net_hosting.h"
#include "host_comm.h"

#include <iostream>
#include <filesystem>

#if _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#else
#include <unistd.h>
#include <limits.h>
#include <string.h>
#endif

using std::filesystem::path;

static path GetExecutablePath();

static void ERRWRITER_CALLTYPE DebugDNetError(const char_t* message);
static void DELEGATE_CALLTYPE DoTestUtility();

typedef void (DELEGATE_CALLTYPE* VoidNoArgPointer)();

int main()
{
    //SetEnvironmentVariable(L"COREHOST_TRACE", L"1");
    //SetEnvironmentVariable(L"COREHOST_TRACE_VERBOSITY", L"4"); // From 1 (lowest) to 4 (highest).

    // Essential paths.
    path executableDir = GetExecutablePath();
    path pathToRuntimeConfig = executableDir / L"ManagedApp.runtimeconfig.json";
    path assemblyPath = executableDir / L"ManagedApp.dll";

    if (!NetHost::Init())
    {
        std::cout << "Failed to initialize .NET host.\n";
        return -1;
    }

    NetHost::HostContext context = NetHost::NewContextForRuntimeConfig(pathToRuntimeConfig.c_str());
    NetHost::SetErrorWriter(DebugDNetError);
    std::cout << "The .NET hosting environment has been initialized.\n";

    std::cout << "Switching to the .NET world...\n";

    HostComm::Init(context, assemblyPath.c_str(), NH_STR("ManagedApp"));
    HostComm::RegisterNativeUtility("test_utility", (void*)&DoTestUtility);

    auto loadAndGetDelegate = context.GetLoadAssemblyAndGetFuncPointer();

    void* callback;
    callback = loadAndGetDelegate(assemblyPath.native().c_str(), NH_STR("ManagedApp.Program, ManagedApp"), NH_STR("Main"), NH_STR("System.Action, netstandard"));

    ((VoidNoArgPointer)callback)();

    context.Close();
    NetHost::Shutdown();
}

path GetExecutablePath()
{
#ifdef _WIN32
    TCHAR buffer[MAX_PATH]{};
    path exePath = std::basic_string<TCHAR>(buffer, GetModuleFileName(NULL, buffer, MAX_PATH));
#else
    char buffer[PATH_MAX];
    memset(buffer, 0, sizeof(buffer)); // readlink does not null terminate!

    int chRead = readlink("/proc/self/exe", buffer, PATH_MAX);
    path exePath = std::string(buffer, chRead);
#endif

    return exePath.parent_path();
}

void ERRWRITER_CALLTYPE DebugDNetError(const char_t* message)
{
    std::cout << "[.NET Error Writer Handler] -> ";
    std::wcout << message << std::endl;
}

void DELEGATE_CALLTYPE DoTestUtility()
{
    std::cout << "You've invoked the test utility on the native side.\n";
}
