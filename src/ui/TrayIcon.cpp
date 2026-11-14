#include "ui/TrayIcon.h"

#include <Windows.h>
#include <shellapi.h>

#include "app/AppState.h"

namespace app {

void AddTrayIcon(AppState& app) {
    if (app.trayIconAdded) {
        return;
    }

    NOTIFYICONDATAW iconData{};
    iconData.cbSize = sizeof(iconData);
    iconData.hWnd = app.window;
    iconData.uID = kTrayIconId;
    iconData.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    iconData.uCallbackMessage = kTrayMessage;
    iconData.hIcon = LoadIconW(nullptr, IDI_APPLICATION);

    lstrcpynW(iconData.szTip, L"LowLevelKeyMapper", 128);

    app.trayIconAdded =
        Shell_NotifyIconW(NIM_ADD, &iconData) != FALSE;
}

void RemoveTrayIcon(AppState& app) {
    if (!app.trayIconAdded) {
        return;
    }

    NOTIFYICONDATAW iconData{};
    iconData.cbSize = sizeof(iconData);
    iconData.hWnd = app.window;
    iconData.uID = kTrayIconId;

    Shell_NotifyIconW(NIM_DELETE, &iconData);
    app.trayIconAdded = false;
}

void ShowTrayMenu(AppState& app) {
    HMENU menu = CreatePopupMenu();
    if (menu == nullptr) {
        return;
    }

    AppendMenuW(menu, MF_STRING, kRestoreWindowCommand, L"Open");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, kExitApplicationCommand, L"Exit");

    POINT cursorPosition{};
    GetCursorPos(&cursorPosition);

    SetForegroundWindow(app.window);

    TrackPopupMenu(
        menu,
        TPM_RIGHTBUTTON,
        cursorPosition.x,
        cursorPosition.y,
        0,
        app.window,
        nullptr
    );

    DestroyMenu(menu);
}


} // namespace app
