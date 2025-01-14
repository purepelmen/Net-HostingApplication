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

set(SOLUTION_DIR "${CMAKE_SOURCE_DIR}/../")
set(THIRDPARTY_DIR "${SOLUTION_DIR}/.Thirdparty")
set(SRC_DIR "${SOLUTION_DIR}/NativeNetHostApp/src/")

add_executable(${CMAKE_PROJECT_NAME} "${SRC_DIR}/main.cpp" "${SRC_DIR}/net_hosting.cpp" "${SRC_DIR}/host_comm.cpp")
if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET ${CMAKE_PROJECT_NAME} PROPERTY CXX_STANDARD 20)
endif()

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE "${SOLUTION_DIR}/.Thirdparty/include")
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE ${THIRDPARTY_DIR})
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE "nethost.lib")

if(WIN32)
    target_compile_definitions(${CMAKE_PROJECT_NAME} PUBLIC UNICODE _UNICODE)
    target_compile_definitions(${CMAKE_PROJECT_NAME} PUBLIC PLATFORM_WINDOWS)
else()
    target_compile_definitions(${CMAKE_PROJECT_NAME} PUBLIC PLATFORM_LINUX)
endif()

add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${THIRDPARTY_DIR}/nethost.dll ${NativeNetHostApp_BINARY_DIR}
)

set(MSBUILD_OUTPUT "${NativeNetHostApp_BINARY_DIR}/")
include("cmake/BuildManagedLib.cmake")

add_dependencies(${CMAKE_PROJECT_NAME} BuildManagedProject)
