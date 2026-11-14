#pragma once

#include <Windows.h>

namespace app {

struct AppState;

LRESULT CALLBACK WindowProcedure(
    HWND window,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
);

bool RegisterMainWindowClass(HINSTANCE instance);

} // namespace app
