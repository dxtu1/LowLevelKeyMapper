#include "ui/MainWindow.h"

#include <algorithm>
#include <iterator>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>


#include "app/AppState.h"
#include "ui/TrayIcon.h"

namespace app {
namespace {

[[nodiscard]] HFONT GetDefaultFont() {
    return static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
}

[[nodiscard]] std::wstring GetWindowText(HWND window) {
    const int length = GetWindowTextLengthW(window);
    std::wstring text(static_cast<std::size_t>(length) + 1, L'\0');

    GetWindowTextW(window, text.data(), length + 1);
    text.resize(static_cast<std::size_t>(length));

    return text;
}

[[nodiscard]] std::string ToAscii(const std::wstring& text) {
    std::string result;
    result.reserve(text.size());

    for (const wchar_t character : text) {
        if (character > 0x7F) {
            return {};
        }

        result.push_back(static_cast<char>(character));
    }

    return result;
}

[[nodiscard]] std::wstring FromAscii(const std::string& text) {
    return {text.begin(), text.end()};
}

void SetStatus(AppState& app, const wchar_t* message) {
    if (app.statusLabel != nullptr) {
        SetWindowTextW(app.statusLabel, message);
    }
}

[[nodiscard]] HWND CreateControl(
    HWND parent,
    const wchar_t* className,
    const wchar_t* text,
    DWORD style,
    int x,
    int y,
    int width,
    int height,
    int controlId
) {
    HWND control = CreateWindowExW(
        0,
        className,
        text,
        WS_CHILD | WS_VISIBLE | style,
        x,
        y,
        width,
        height,
        parent,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(controlId)),
        GetModuleHandleW(nullptr),
        nullptr
    );

    if (control != nullptr) {
        SendMessageW(
            control,
            WM_SETFONT,
            reinterpret_cast<WPARAM>(GetDefaultFont()),
            TRUE
        );
    }

    return control;
}

[[nodiscard]] DWORD GetScanCode(const KeyEvent& event) {
    constexpr DWORD kExtendedScanCodeFlag = 0x100;

    return event.scanCode |
        ((event.flags & LLKHF_EXTENDED) != 0 ? kExtendedScanCodeFlag : 0);
}

void RefreshMappingList(AppState& app) {
    SendMessageW(app.mappingList, LB_RESETCONTENT, 0, 0);

    const auto mappings = app.mapper.GetMappings();

    std::vector<std::pair<DWORD, std::vector<DWORD>>> sortedMappings(
        mappings.begin(),
        mappings.end()
    );

    std::sort(
        sortedMappings.begin(),
        sortedMappings.end(),
        [](const auto& left, const auto& right) {
            return left.first < right.first;
        }
    );

    for (const auto& [source, targets] : sortedMappings) {
        const auto sourceName = KeyMapper::KeyCodeToName(source);
        if (!sourceName) {
            continue;
        }

        std::wstring displayText = FromAscii(*sourceName) + L"  ->  ";

        for (std::size_t index = 0; index < targets.size(); ++index) {
            const auto targetName = KeyMapper::KeyCodeToName(targets[index]);
            if (!targetName) {
                continue;
            }

            if (index > 0) {
                displayText += L" + ";
            }

            displayText += FromAscii(*targetName);
        }

        const LRESULT itemIndex = SendMessageW(
            app.mappingList,
            LB_ADDSTRING,
            0,
            reinterpret_cast<LPARAM>(displayText.c_str())
        );

        if (itemIndex != LB_ERR) {
            SendMessageW(
                app.mappingList,
                LB_SETITEMDATA,
                static_cast<WPARAM>(itemIndex),
                static_cast<LPARAM>(source)
            );
        }
    }
}

void AddMapping(AppState& app) {
    const std::string sourceText = ToAscii(GetWindowText(app.sourceEdit));
    std::string targetText = ToAscii(GetWindowText(app.targetEdit));

    const auto source = KeyMapper::ParseKeyName(sourceText);

    if (!source || targetText.empty()) {
        SetStatus(app, L"Enter a valid source and target.");
        return;
    }

    std::replace(targetText.begin(), targetText.end(), '+', ' ');

    std::istringstream input(targetText);
    std::string token;
    std::vector<DWORD> targets;

    while (input >> token) {
        const auto target = KeyMapper::ParseKeyName(token);

        if (!target) {
            SetStatus(app, L"Unknown target key.");
            return;
        }

        if (std::find(targets.begin(), targets.end(), *target) == targets.end()) {
            targets.push_back(*target);
        }
    }

    if (targets.empty()) {
        SetStatus(app, L"At least one target is required.");
        return;
    }

    app.mapper.SetMapping(*source, std::move(targets));
    RefreshMappingList(app);
    SetStatus(app, L"Mapping added. Save to persist.");
}

void SaveConfiguration(AppState& app) {
    std::string error;

    if (!app.mapper.SaveToFile(app.configPath.string(), error)) {
        MessageBoxA(
            app.window,
            error.c_str(),
            "Save failed",
            MB_OK | MB_ICONERROR
        );
        return;
    }

    SetStatus(
        app,
        L"Configuration saved. Previous version backed up to config.json.bak."
    );
}

void SetRunAtStartup(bool enabled) {
    constexpr wchar_t kRunRegistryPath[] =
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
    constexpr wchar_t kApplicationValueName[] = L"LowLevelKeyMapper";

    HKEY registryKey = nullptr;

    const LSTATUS result = RegCreateKeyExW(
        HKEY_CURRENT_USER,
        kRunRegistryPath,
        0,
        nullptr,
        0,
        KEY_SET_VALUE,
        nullptr,
        &registryKey,
        nullptr
    );

    if (result != ERROR_SUCCESS) {
        return;
    }

    if (enabled) {
        wchar_t executablePath[32768]{};
        GetModuleFileNameW(
            nullptr,
            executablePath,
            static_cast<DWORD>(std::size(executablePath))
        );

        const std::wstring quotedPath =
            L"\"" + std::wstring(executablePath) + L"\"";

        RegSetValueExW(
            registryKey,
            kApplicationValueName,
            0,
            REG_SZ,
            reinterpret_cast<const BYTE*>(quotedPath.c_str()),
            static_cast<DWORD>(
                (quotedPath.size() + 1) * sizeof(wchar_t)
            )
        );
    }
    else {
        RegDeleteValueW(registryKey, kApplicationValueName);
    }

    RegCloseKey(registryKey);
}

[[nodiscard]] bool IsRunAtStartupEnabled() {
    constexpr wchar_t kRunRegistryPath[] =
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
    constexpr wchar_t kApplicationValueName[] = L"LowLevelKeyMapper";

    HKEY registryKey = nullptr;

    if (RegOpenKeyExW(
            HKEY_CURRENT_USER,
            kRunRegistryPath,
            0,
            KEY_QUERY_VALUE,
            &registryKey
        ) != ERROR_SUCCESS) {
        return false;
    }

    DWORD valueType = 0;
    DWORD valueSize = 0;

    const bool isEnabled = RegQueryValueExW(
        registryKey,
        kApplicationValueName,
        nullptr,
        &valueType,
        nullptr,
        &valueSize
    ) == ERROR_SUCCESS;

    RegCloseKey(registryKey);
    return isEnabled;
}

void LayoutControls(AppState& app) {
    RECT clientRect{};
    GetClientRect(app.window, &clientRect);

    const int clientWidth = clientRect.right;

    MoveWindow(app.sourceEdit, 20, 65, 100, 27, TRUE);
    MoveWindow(app.targetEdit, 135, 65, 190, 27, TRUE);

    MoveWindow(
        GetDlgItem(app.window, kCaptureSourceId),
        20, 98, 100, 26, TRUE
    );
    MoveWindow(
        GetDlgItem(app.window, kCaptureTargetId),
        135, 98, 120, 26, TRUE
    );
    MoveWindow(
        GetDlgItem(app.window, kAddMappingId),
        340, 65, 110, 28, TRUE
    );

    MoveWindow(
        app.mappingList,
        20, 155, clientWidth - 40, 220, TRUE
    );

    MoveWindow(
        GetDlgItem(app.window, kRemoveMappingId),
        20, 390, 100, 30, TRUE
    );
    MoveWindow(
        GetDlgItem(app.window, kClearMappingsId),
        130, 390, 100, 30, TRUE
    );
    MoveWindow(
        GetDlgItem(app.window, kSaveConfigId),
        240, 390, 100, 30, TRUE
    );

    MoveWindow(
        app.statusLabel,
        20, 435, clientWidth - 40, 24, TRUE
    );
    MoveWindow(
        app.enableCheckBox,
        clientWidth - 240, 15, 220, 24, TRUE
    );
    MoveWindow(
        app.startupCheckBox,
        clientWidth - 240, 42, 220, 24, TRUE
    );
}

void CreateMainControls(AppState& app) {
    const HWND window = app.window;

    CreateControl(
        window, L"STATIC", L"LowLevelKeyMapper",
        0, 20, 15, 250, 28, 0
    );

    app.enableCheckBox = CreateControl(
        window, L"BUTTON", L"Enable remapping",
        BS_AUTOCHECKBOX, 360, 15, 200, 24, kEnableRemappingId
    );
    SendMessageW(app.enableCheckBox, BM_SETCHECK, BST_CHECKED, 0);

    app.startupCheckBox = CreateControl(
        window, L"BUTTON", L"Run at Windows startup",
        BS_AUTOCHECKBOX, 360, 42, 200, 24, kRunAtStartupId
    );
    SendMessageW(
        app.startupCheckBox,
        BM_SETCHECK,
        IsRunAtStartupEnabled() ? BST_CHECKED : BST_UNCHECKED,
        0
    );

    CreateControl(
        window, L"STATIC", L"Source physical key",
        0, 20, 42, 100, 20, 0
    );
    CreateControl(
        window, L"STATIC", L"Target keys (e.g. CTRL + C)",
        0, 135, 42, 220, 20, 0
    );

    app.sourceEdit = CreateControl(
        window, L"EDIT", L"",
        WS_BORDER | ES_AUTOHSCROLL,
        20, 65, 100, 27, kSourceEditId
    );
    app.targetEdit = CreateControl(
        window, L"EDIT", L"",
        WS_BORDER | ES_AUTOHSCROLL,
        135, 65, 190, 27, kTargetEditId
    );

    CreateControl(
        window, L"BUTTON", L"Capture source",
        BS_PUSHBUTTON, 20, 98, 100, 26, kCaptureSourceId
    );
    CreateControl(
        window, L"BUTTON", L"Capture target",
        BS_PUSHBUTTON, 135, 98, 120, 26, kCaptureTargetId
    );
    CreateControl(
        window, L"BUTTON", L"Add mapping",
        BS_PUSHBUTTON, 340, 65, 110, 28, kAddMappingId
    );

    app.mappingList = CreateControl(
        window, L"LISTBOX", L"",
        WS_BORDER | WS_VSCROLL | LBS_NOTIFY | LBS_HASSTRINGS,
        20, 155, 520, 220, kMappingListId
    );

    CreateControl(
        window, L"BUTTON", L"Remove",
        BS_PUSHBUTTON, 20, 390, 100, 30, kRemoveMappingId
    );
    CreateControl(
        window, L"BUTTON", L"Clear all",
        BS_PUSHBUTTON, 130, 390, 100, 30, kClearMappingsId
    );
    CreateControl(
        window, L"BUTTON", L"Save",
        BS_PUSHBUTTON, 240, 390, 100, 30, kSaveConfigId
    );

    app.statusLabel = CreateControl(
        window,
        L"STATIC",
        L"Ready. Closing the window minimizes to the system tray.",
        0,
        20, 435, 520, 24,
        0
    );
}

bool HandleKeyboardEvent(AppState& app, const KeyEvent& event) {
    if (event.isInjected) {
        return false;
    }

    const DWORD scanCode = GetScanCode(event);

    if (app.captureMode != CaptureMode::None && event.isDown) {
        PostMessageW(
            app.window,
            kCapturedKeyMessage,
            static_cast<WPARAM>(scanCode),
            static_cast<LPARAM>(app.captureMode)
        );
        return true;
    }

    if (event.isDown) {
        if (!app.remappingEnabled) {
            return app.keyStateManager.HandlePhysicalEvent(scanCode, true);
        }

        const auto mapping = app.mapper.FindMapping(scanCode);

        if (!mapping) {
            return app.keyStateManager.HandlePhysicalEvent(scanCode, true);
        }

        return app.keyStateManager.HandleEvent(scanCode, *mapping, true);
    }

    if (event.isUp) {
        if (app.keyStateManager.HandleEvent(scanCode, {}, false)) {
            return true;
        }

        return app.keyStateManager.HandlePhysicalEvent(scanCode, false);
    }

    return false;
}

void InstallKeyboardHook(AppState& app) {
    app.keyboardHook = std::make_unique<KeyboardHook>(
        [&app](const KeyEvent& event) {
            return HandleKeyboardEvent(app, event);
        }
    );
}

void HandleCapturedKey(AppState& app, WPARAM wParam, LPARAM lParam) {
    const DWORD scanCode = static_cast<DWORD>(wParam);
    const auto keyName = KeyMapper::KeyCodeToName(scanCode);

    if (keyName) {
        const std::wstring displayName = FromAscii(*keyName);

        if (static_cast<CaptureMode>(lParam) == CaptureMode::Source) {
            SetWindowTextW(app.sourceEdit, displayName.c_str());
        }
        else {
            std::wstring currentText = GetWindowText(app.targetEdit);

            if (!currentText.empty()) {
                currentText += L" + ";
            }

            currentText += displayName;
            SetWindowTextW(app.targetEdit, currentText.c_str());
        }
    }

    app.captureMode = CaptureMode::None;
    SetStatus(app, L"Key captured.");
}

void RemoveSelectedMapping(AppState& app) {
    const LRESULT selectedIndex = SendMessageW(
        app.mappingList,
        LB_GETCURSEL,
        0,
        0
    );

    if (selectedIndex == LB_ERR) {
        return;
    }

    const DWORD source = static_cast<DWORD>(
        SendMessageW(
            app.mappingList,
            LB_GETITEMDATA,
            static_cast<WPARAM>(selectedIndex),
            0
        )
    );

    app.mapper.RemoveMapping(source);
    RefreshMappingList(app);
    SetStatus(app, L"Mapping removed. Save to persist.");
}

void ClearMappings(AppState& app) {
    const int result = MessageBoxW(
        app.window,
        L"Remove every mapping? Save to make this permanent.",
        L"Confirm",
        MB_YESNO | MB_ICONQUESTION
    );

    if (result != IDYES) {
        return;
    }

    app.mapper.ClearMappings();
    RefreshMappingList(app);
    SetStatus(app, L"All mappings cleared.");
}

void SetRemappingEnabled(AppState& app) {
    app.remappingEnabled =
        SendMessageW(app.enableCheckBox, BM_GETCHECK, 0, 0) == BST_CHECKED;

    if (!app.remappingEnabled) {
        app.keyStateManager.ReleaseAll();
    }

    SetStatus(
        app,
        app.remappingEnabled
            ? L"Remapping enabled."
            : L"Remapping disabled."
    );
}

void SetStartupPreference(AppState& app) {
    const bool enabled =
        SendMessageW(app.startupCheckBox, BM_GETCHECK, 0, 0) == BST_CHECKED;

    SetRunAtStartup(enabled);
    SetStatus(app, L"Startup preference updated for the current Windows user.");
}

void RestoreMainWindow(HWND window) {
    ShowWindow(window, SW_SHOW);
    ShowWindow(window, SW_RESTORE);
    SetForegroundWindow(window);
}

void HandleCommand(AppState& app, int commandId) {
    switch (commandId) {
    case kAddMappingId:
        AddMapping(app);
        break;

    case kRemoveMappingId:
        RemoveSelectedMapping(app);
        break;

    case kClearMappingsId:
        ClearMappings(app);
        break;

    case kSaveConfigId:
        SaveConfiguration(app);
        break;

    case kCaptureSourceId:
        app.captureMode = CaptureMode::Source;
        SetStatus(app, L"Press one key to capture as source.");
        break;

    case kCaptureTargetId:
        app.captureMode = CaptureMode::Target;
        SetStatus(app, L"Press a target key. Repeat to add more target keys.");
        break;

    case kEnableRemappingId:
        SetRemappingEnabled(app);
        break;

    case kRunAtStartupId:
        SetStartupPreference(app);
        break;

    case kRestoreWindowCommand:
        RestoreMainWindow(app.window);
        break;

    case kExitApplicationCommand:
        DestroyWindow(app.window);
        break;

    default:
        break;
    }
}

} // namespace

LRESULT CALLBACK WindowProcedure(
    HWND window,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
) {
    AppState* app = reinterpret_cast<AppState*>(
        GetWindowLongPtrW(window, GWLP_USERDATA)
    );

    if (message == WM_NCCREATE) {
        const auto* createInfo =
            reinterpret_cast<const CREATESTRUCTW*>(lParam);

        app = static_cast<AppState*>(createInfo->lpCreateParams);
        app->window = window;

        SetWindowLongPtrW(
            window,
            GWLP_USERDATA,
            reinterpret_cast<LONG_PTR>(app)
        );
    }

    switch (message) {
    case WM_CREATE:
        CreateMainControls(*app);
        InstallKeyboardHook(*app);

        if (!app->keyboardHook->Install()) {
            MessageBoxW(
                window,
                L"Could not install the low-level keyboard hook.",
                L"Error",
                MB_OK | MB_ICONERROR
            );
            return -1;
        }

        RefreshMappingList(*app);
        AddTrayIcon(*app);
        LayoutControls(*app);
        return 0;

    case WM_SIZE:
        if (app != nullptr && app->mappingList != nullptr) {
            LayoutControls(*app);
        }

        if (wParam == SIZE_MINIMIZED) {
            ShowWindow(window, SW_HIDE);
        }
        return 0;

    case kCapturedKeyMessage:
        HandleCapturedKey(*app, wParam, lParam);
        return 0;

    case WM_COMMAND:
        if (app != nullptr) {
            HandleCommand(*app, LOWORD(wParam));
            return 0;
        }
        break;

    case kTrayMessage:
        if (app != nullptr && wParam == kTrayIconId) {
            if (lParam == WM_LBUTTONDBLCLK) {
                RestoreMainWindow(window);
            }
            else if (lParam == WM_RBUTTONUP) {
                ShowTrayMenu(*app);
            }
        }
        return 0;

    case WM_CLOSE:
        if (app != nullptr) {
            ShowWindow(window, SW_HIDE);
            SetStatus(
                *app,
                L"Still running in the system tray. Choose Exit from the tray menu to quit."
            );
        }
        return 0;

    case WM_DESTROY:
        if (app != nullptr) {
            if (app->keyboardHook) {
                app->keyboardHook->Uninstall();
                app->keyStateManager.ReleaseAll();
                app->keyboardHook.reset();
            }

            RemoveTrayIcon(*app);
        }

        PostQuitMessage(0);
        return 0;

    default:
        break;
    }

    return DefWindowProcW(window, message, wParam, lParam);
}


bool RegisterMainWindowClass(HINSTANCE instance) {
    WNDCLASSW windowClass{};
    windowClass.lpfnWndProc = WindowProcedure;
    windowClass.hInstance = instance;
    windowClass.lpszClassName = kWindowClassName;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    windowClass.hbrBackground =
        reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);

    return RegisterClassW(&windowClass) != 0;
}

} // namespace app
