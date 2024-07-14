#include "net_hosting.h"
#include "host_comm.h"

#include <iostream>
#include <filesystem>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

static void DebugDNetError(const wchar_t* message);
static std::wstring GetExecutableDirW();

using VoidNoArgPointer = void (*)();
using HostCommInitCallback = void (*)(void* utilityLocator);

int main()
{
    // Essential paths.
    std::wstring executableDir = GetExecutableDirW();
    std::wstring pathToRuntimeConfig = executableDir + L"\\ManagedApp.runtimeconfig.json";
    std::wstring assemblyPath = executableDir + L"\\ManagedApp.dll";

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
    callback = loadAndGetDelegate.Perform(assemblyPath, L"ManagedApp.HostComm, ManagedApp", L"Init", NetHost::UNMANAGED_CALLERS_ONLY);
    ((HostCommInitCallback)callback)(&HostComm::GetNativeUtility);

    callback = loadAndGetDelegate.Perform(assemblyPath, L"ManagedApp.Program, ManagedApp", L"Main");
    ((NetHost::DefaultDNetCallback)callback)(nullptr, 0);
    callback = loadAndGetDelegate.Perform(assemblyPath, L"ManagedApp.Program, ManagedApp", L"Bark", L"ManagedApp.Program+TestDelegate, ManagedApp");
    ((VoidNoArgPointer)callback)();
    callback = loadAndGetDelegate.Perform(assemblyPath, L"ManagedApp.Program, ManagedApp", L"DoAnother", NetHost::UNMANAGED_CALLERS_ONLY);
    ((VoidNoArgPointer)callback)();

    context.Close();
    NetHost::Shutdown();
}

void DebugDNetError(const wchar_t* message)
{
    std::cout << "[.NET Error Writer Handler] -> ";
    std::wcout << message << std::endl;
}

std::wstring GetExecutableDirW()
{
    wchar_t buffer[MAX_PATH]{};

    std::filesystem::path exePath = std::wstring(buffer, GetModuleFileName(NULL, buffer, MAX_PATH));
    return exePath.parent_path().wstring();
}
