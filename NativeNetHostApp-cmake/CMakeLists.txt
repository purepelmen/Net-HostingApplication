cmake_minimum_required(VERSION 3.8)
project(NativeNetHostApp)

# Enable hot reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

if(NOT DEFINED CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Determining target platform (Windows(x86 vs x64) vs Linux).
if(WIN32)
    set(TARGET_IDENTIFIER "win-x64")
    if(CMAKE_SIZEOF_VOID_P EQUAL 4)
        set(TARGET_IDENTIFIER "win-x86")
    else()
        set(TARGET_IDENTIFIER "win-x64")
    endif()
else()
    set(TARGET_IDENTIFIER "linux-x64")
endif()
message(STATUS "TargetIdentifier = '${TARGET_IDENTIFIER}'")

set(SOLUTION_DIR "${CMAKE_SOURCE_DIR}/../")
set(THIRDPARTY_DIR "${SOLUTION_DIR}/.Thirdparty")
set(SRC_DIR "${SOLUTION_DIR}/NativeNetHostApp/src")

add_executable(${CMAKE_PROJECT_NAME} "${SRC_DIR}/main.cpp" "${SRC_DIR}/net_hosting.cpp" "${SRC_DIR}/host_comm.cpp")
if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET ${CMAKE_PROJECT_NAME} PROPERTY CXX_STANDARD 20)
endif()

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE "${THIRDPARTY_DIR}/include")
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE "${THIRDPARTY_DIR}/libs-${TARGET_IDENTIFIER}")
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE "nethost")

if(WIN32)
    target_compile_definitions(${CMAKE_PROJECT_NAME} PUBLIC UNICODE _UNICODE)
    target_compile_definitions(${CMAKE_PROJECT_NAME} PUBLIC PLATFORM_WINDOWS)

    # On Windows we must copy all .DLLs.
    add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${THIRDPARTY_DIR}/libs-${TARGET_IDENTIFIER}/nethost.dll $<TARGET_FILE_DIR:NativeNetHostApp>
    )
else()
    target_compile_definitions(${CMAKE_PROJECT_NAME} PUBLIC PLATFORM_LINUX)
endif()

set(MSBUILD_OUTPUT "${NativeNetHostApp_BINARY_DIR}/${CMAKE_CFG_INTDIR}")
include("cmake/BuildManagedLib.cmake")

add_dependencies(${CMAKE_PROJECT_NAME} BuildManagedProject)
