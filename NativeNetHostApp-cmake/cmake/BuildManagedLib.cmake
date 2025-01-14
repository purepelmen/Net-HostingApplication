find_program(IS_DOTNET_INSTALLED dotnet HINTS ENV PATH REQUIRED)
if(NOT IS_DOTNET_INSTALLED)
    message(FATAL_ERROR "'dotnet' CLI tool has not been found (not installed, or inaccessible). It's REQUIRED to build the managed project.")
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CSPROJ_BUILD_CONFIGURATION "Debug")
else()
    set(CSPROJ_BUILD_CONFIGURATION "Release")
endif()

set(CSPROJ_FILE "${SOLUTION_DIR}/ManagedApp/ManagedApp.csproj")
if(NOT MSBUILD_OUTPUT)
    message(FATAL_ERROR "MSBUILD_OUTPUT is not set. Where to build the managed project?")
endif()

add_custom_target(BuildManagedProject
    COMMAND ${CMAKE_COMMAND} -E echo "Building .NET project: ${CSPROJ_FILE}"
    COMMAND dotnet build ${CSPROJ_FILE} -c ${CSPROJ_BUILD_CONFIGURATION} /p:OutDir=${MSBUILD_OUTPUT}
    COMMENT "Building .NET managed project."
    VERBATIM
)
