cmake_minimum_required(VERSION 3.21)

# --- vcpkg auto-bootstrap ------------------------------------------------
# Must run before project(), since CMAKE_TOOLCHAIN_FILE only takes effect
# if set before the first project() call. Downloads vcpkg into the project
# tree so a fresh checkout can build with nothing pre-installed but git,
# cmake and a C++ compiler.
set(VCPKG_ROOT_DIR "${CMAKE_SOURCE_DIR}/third_party/vcpkg")

if(NOT EXISTS "${VCPKG_ROOT_DIR}/.vcpkg-root")
    find_program(GIT_EXECUTABLE git REQUIRED)
    message(STATUS "vcpkg not found, cloning into ${VCPKG_ROOT_DIR} ...")
    execute_process(
        COMMAND "${GIT_EXECUTABLE}" clone --depth 1
                https://github.com/microsoft/vcpkg.git "${VCPKG_ROOT_DIR}"
        RESULT_VARIABLE VCPKG_CLONE_RESULT
    )
    if(NOT VCPKG_CLONE_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to clone vcpkg")
    endif()
endif()

if(WIN32)
    set(VCPKG_BOOTSTRAP_SCRIPT "${VCPKG_ROOT_DIR}/bootstrap-vcpkg.bat")
    set(VCPKG_EXECUTABLE "${VCPKG_ROOT_DIR}/vcpkg.exe")
else()
    set(VCPKG_BOOTSTRAP_SCRIPT "${VCPKG_ROOT_DIR}/bootstrap-vcpkg.sh")
    set(VCPKG_EXECUTABLE "${VCPKG_ROOT_DIR}/vcpkg")
endif()

if(NOT EXISTS "${VCPKG_EXECUTABLE}")
    message(STATUS "Bootstrapping vcpkg ...")
    execute_process(
        COMMAND "${VCPKG_BOOTSTRAP_SCRIPT}" -disableMetrics
        WORKING_DIRECTORY "${VCPKG_ROOT_DIR}"
        RESULT_VARIABLE VCPKG_BOOTSTRAP_RESULT
    )
    if(NOT VCPKG_BOOTSTRAP_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to bootstrap vcpkg")
    endif()
endif()

set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT_DIR}/scripts/buildsystems/vcpkg.cmake"
    CACHE STRING "Vcpkg toolchain file")

# Link everything statically so a release is a single self-contained .exe:
# no OpenSSL/zlib DLLs to ship alongside it and no Visual C++ Redistributable
# to install. Must be set before project(), since the triplet decides how
# vcpkg builds the dependencies.
if(WIN32)
    set(VCPKG_TARGET_TRIPLET "x64-windows-static" CACHE STRING "vcpkg triplet")
endif()

# --- Project --------------------------------------------------------------
project(TelegramChatExporter CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Static CRT, to match the static vcpkg triplet above. Two mechanisms on
# purpose: CMAKE_MSVC_RUNTIME_LIBRARY covers our own targets, while patching
# the flags directly covers TDLib — it declares an ancient
# cmake_minimum_required, so the policy that makes the former work
# (CMP0091) isn't active inside its subdirectory.
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

    foreach(flag_var
            CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
            CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO
            CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
            CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO)
        string(REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
    endforeach()
endif()

# --- TDLib (built from source, fetched into third_party/td) ---------------
include(FetchContent)
set(FETCHCONTENT_BASE_DIR "${CMAKE_SOURCE_DIR}/third_party")

# vcpkg installs gperf as a build tool, not a library — it doesn't end up
# on find_program()'s default search path, but TDLib's own CMakeLists.txt
# looks it up that way. Point CMake at where vcpkg actually put it.
if(DEFINED VCPKG_TARGET_TRIPLET)
    list(APPEND CMAKE_PROGRAM_PATH
        "${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/tools/gperf")
endif()

FetchContent_Declare(
    td
    GIT_REPOSITORY https://github.com/tdlib/td.git
    GIT_TAG 022d60202e446ad1287b9fb68e687c8a0760788b
)

# TDLib's own CMakeLists.txt declares a cmake_minimum_required far below
# what CMake 4.x still supports (< 3.5 was removed) — this makes CMake
# treat it as requiring 3.5 without touching our own project's policies.
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
FetchContent_MakeAvailable(td)

# tdutils/CMakeLists.txt macro-izes `register` (and `__gnu_inline__`) for one
# generated source file to keep old gperf-generated code compiling under
# GCC/Clang — MSVC's STL headers explicitly forbid macroizing keywords like
# `register` and fail to compile as a result. _ALLOW_KEYWORD_MACROS is the
# STL's own documented escape hatch for exactly this situation.
if(MSVC)
    target_compile_definitions(tdutils PRIVATE _ALLOW_KEYWORD_MACROS)
endif()

# --- Dependencies installed by vcpkg ---------------------------------------
find_package(nlohmann_json CONFIG REQUIRED)

add_executable(TelegramChatExporter
    main.cpp
        platform/windows/tray.cpp
        platform/windows/control_panel.cpp
        platform/windows/control_panel.h
        platform/windows/scenes/scene.cpp
        platform/windows/scenes/scene.h
        platform/windows/ui/ui.cpp
        platform/windows/ui/ui.h
        platform/client/client.cpp
        platform/client/client.h
)

target_include_directories(TelegramChatExporter PRIVATE "${CMAKE_SOURCE_DIR}")

target_link_libraries(TelegramChatExporter PRIVATE
    Td::TdStatic
    nlohmann_json::nlohmann_json
)

if(WIN32)
    target_link_libraries(TelegramChatExporter PRIVATE Shell32 Dwmapi Ole32)

    # No console is auto-created no matter how the app is launched
    # (double-click, Task Scheduler, ...) — only the tray icon shows up.
    # /ENTRY:mainCRTStartup keeps a plain main() working under
    # /SUBSYSTEM:WINDOWS (which WIN32_EXECUTABLE sets) instead of requiring
    # WinMain.
    set_target_properties(TelegramChatExporter PROPERTIES WIN32_EXECUTABLE TRUE)
    if(MSVC)
        target_link_options(TelegramChatExporter PRIVATE /ENTRY:mainCRTStartup)
    endif()
endif()
