#include "core/console.h"

#ifdef _WIN32
#include <cstdio>
#include <iostream>
#include <windows.h>
#endif

namespace exporter {

#ifdef _WIN32
namespace {

bool g_console_allocated = false;

void redirect_stdio_to_console() {
    FILE* f;
    freopen_s(&f, "CONOUT$", "w", stdout);
    freopen_s(&f, "CONOUT$", "w", stderr);
    freopen_s(&f, "CONIN$", "r", stdin);
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    // A GUI-subsystem process starts with no valid stdin/stdout at all, so
    // std::cin/cout/cerr are already latched into a failed state from
    // startup — freopen() fixes the underlying C streams but doesn't clear
    // that state on its own.
    std::cin.clear();
    std::cout.clear();
    std::cerr.clear();
}

BOOL WINAPI console_ctrl_handler(DWORD ctrl_type) {
    if (ctrl_type == CTRL_CLOSE_EVENT) {
        // Best-effort only: for this specific event Windows does not
        // reliably wait on the handler's return value the way it does for
        // CTRL_C_EVENT — it may terminate the process after a short grace
        // period regardless. hide_console() (called once prompts are done)
        // is the real fix; this is just a fallback if the window gets
        // closed manually before that happens.
        FreeConsole();
        g_console_allocated = false;
        return TRUE;
    }
    return FALSE;
}

}  // namespace
#endif

void ensure_console_visible() {
#ifdef _WIN32
    if (g_console_allocated) {
        return;
    }
    g_console_allocated = true;

    AllocConsole();
    redirect_stdio_to_console();
    SetConsoleCtrlHandler(console_ctrl_handler, TRUE);
#endif
}

void hide_console() {
#ifdef _WIN32
    if (!g_console_allocated) {
        return;
    }
    g_console_allocated = false;
    FreeConsole();
#endif
}

}  // namespace exporter
