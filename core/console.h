#pragma once

namespace exporter {

// Allocates a fresh console window and wires stdin/stdout/stderr to it.
// Idempotent (safe to call from every prompt site) and a no-op on
// non-Windows targets. Used right before an interactive prompt, so a
// console only ever appears when we actually need to ask the user
// something.
void ensure_console_visible();

// Detaches and closes the console opened by ensure_console_visible(), if
// any. Call once the interactive prompts that needed it are done, so the
// user never has to close the window by hand (its close button is not a
// reliable way to do this — see console.cpp). No-op on non-Windows targets
// or if no console was ever allocated.
void hide_console();

}  // namespace exporter
