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

# --- Project --------------------------------------------------------------
project(TelegramChatExporter CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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
find_package(spdlog CONFIG REQUIRED)
find_package(CLI11 CONFIG REQUIRED)

add_executable(TelegramChatExporter
    main.cpp
    core/tdlib_client.cpp
    core/auth.cpp
    core/config.cpp
    core/console.cpp
)

target_include_directories(TelegramChatExporter PRIVATE "${CMAKE_SOURCE_DIR}")

target_link_libraries(TelegramChatExporter PRIVATE
    Td::TdStatic
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    CLI11::CLI11
)

if(WIN32)
    target_sources(TelegramChatExporter PRIVATE
        platform/windows/windows_agent.cpp
        platform/windows/menu_window.cpp
    )

    # No console is auto-created by Windows for this process, no matter how
    # it's launched (double-click, Task Scheduler, ...) — core/console.cpp
    # allocates one on demand only when an interactive prompt is actually
    # needed. /ENTRY:mainCRTStartup keeps a plain main() working under
    # /SUBSYSTEM:WINDOWS (which WIN32_EXECUTABLE sets) instead of requiring
    # WinMain.
    set_target_properties(TelegramChatExporter PROPERTIES WIN32_EXECUTABLE TRUE)
    if(MSVC)
        target_link_options(TelegramChatExporter PRIVATE /ENTRY:mainCRTStartup)
    endif()

    target_link_libraries(TelegramChatExporter PRIVATE Shell32 Dwmapi UxTheme)
endif()
