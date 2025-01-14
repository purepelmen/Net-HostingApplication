# Net-HostingApplication

## Structure
NativeNetHostApp *hosts* a .NET application/library (ManagedApp). It uses `nethost` lib that helps it to find the installed .NET and load the `hostfxr` which can actually initialize one of the installed runtimes with the right version.
Additionally the native side also provides a tiny abstraction for this process to simplify things, and also helps to comminicate with the managed side.

### net_hosting
This module defines an abstraction over hostfxr and loading it.
* net_hosting.h
* net_hosting.cpp
* net_hosting_internal.h

There's enough comments to explain what's happenning there. Additional corrections will be made if I find more information.


The main API parts of it are defined in the `NetHost` namespace and shortly described below.

#### Main Initialization-related Functions
The whole net_hosting module must be initialized with `Init()` function (and optionally passing the path to hostfxr, instead of automatic searching for it).
The deinitialization could be done via `Shutdown()`, and the current status can be checked with `IsInited()`.

#### Two Host Initialization Functions
After initialization, you can now create a hosting context that allows you to host your managed app the way you want.
* `InitForCommandLine()`
* `InitForRuntimeConfig()`

#### Host Context
The created hosting context object allows you to further work with the app like:
* `RunApp()` to run everything like an application (if created with InitForCommandLine).
* Use Get*() functions to get function objects that allows you to call *runtime delegates* of hostfxr to do different things (like loading assemblies, getting function pointers).
* `Close()` the hosting context.

#### Runtime Delegate Functors
The host context allows you to get runtime delegates in the form of functors that's basically a class `rd_*` with an overloaded function call operator to invoke the delegate.
* `rd_LoadAssemblyAndGetFuncPointer`
* `rd_GetFuncPointer`

### HostComm
This module provides a way to communicate between two sides.

On the native side it allows you to `Init()` communication, and register some *native utilities* and give them a specific name.
* `RegisterNativeUtility()`
* `UnregisterNativeUtility()`

The managed side can use `ManagedApp.HostComm` class to request some native utilities in the form of delegates to invoke it.
This is only permitted after a successful HostComm initialization which can be queried via a special property.
* `HostComm.GetNativeUtility()`
* `HostComm.RequireNativeUtility()`
* `HostComm.IsInitialized`

## TODO
- [ ] Improve error handling that's currently simply checked with `assert()` calls.
- [ ] Create CMake project to build for Linux.
- [ ] 32-bit support?
- [ ] Allow to embed CoreCLR as a build option?
