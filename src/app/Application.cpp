#include "app/Application.h"

#include <Windows.h>

#include <filesystem>
#include <string>

#include "app/AppState.h"
#include "ui/MainWindow.h"

namespace {

std::filesystem::path GetExecutableDirectory() {
    std::wstring path(32768, L'\0');

    const DWORD length = GetModuleFileNameW(
        nullptr,
        path.data(),
        static_cast<DWORD>(path.size())
    );

    if (length == 0 || length >= path.size()) {
        return std::filesystem::current_path();
    }

    path.resize(length);
    return std::filesystem::path(path).parent_path();
}

} // namespace

int RunApplication(HINSTANCE instance, int showCommand) {
    app::AppState state;
    state.configPath = GetExecutableDirectory() / L"config.json";

    std::string error;

    if (!state.mapper.LoadFromFile(state.configPath.string(), error)) {
        MessageBoxA(
            nullptr,
            error.c_str(),
            "Configuration error",
            MB_OK | MB_ICONERROR
        );
        return 1;
    }

    if (!app::RegisterMainWindowClass(instance)) {
        MessageBoxW(
            nullptr,
            L"Window class registration failed.",
            L"Error",
            MB_OK | MB_ICONERROR
        );
        return 1;
    }

    HWND window = CreateWindowExW(
        0,
        app::kWindowClassName,
        L"LowLevelKeyMapper",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        620,
        520,
        nullptr,
        nullptr,
        instance,
        &state
    );

    if (window == nullptr) {
        return 1;
    }

    ShowWindow(window, showCommand);
    UpdateWindow(window);

    MSG message{};

    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    return static_cast<int>(message.wParam);
}
