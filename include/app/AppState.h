#pragma once

#include <Windows.h>

#include <filesystem>
#include <memory>

#include "input/KeyboardHook.h"
#include "mapping/KeyMapper.h"
#include "input/KeyStateManager.h"

namespace app {

inline constexpr wchar_t kWindowClassName[] = L"LowLevelKeyMapperWindow";

inline constexpr UINT kTrayMessage = WM_APP + 1;
inline constexpr UINT kCapturedKeyMessage = WM_APP + 2;
inline constexpr UINT kTrayIconId = 1;

enum ControlId : int {
    kSourceEditId = 1001,
    kTargetEditId,
    kAddMappingId,
    kRemoveMappingId,
    kSaveConfigId,
    kMappingListId,
    kEnableRemappingId,
    kCaptureSourceId,
    kCaptureTargetId,
    kClearMappingsId,
    kRunAtStartupId,
};

enum TrayCommand : UINT {
    kRestoreWindowCommand = 2001,
    kExitApplicationCommand,
};

enum class CaptureMode {
    None,
    Source,
    Target,
};

struct AppState {
    KeyMapper mapper;
    KeyStateManager keyStateManager;
    std::unique_ptr<KeyboardHook> keyboardHook;

    HWND window = nullptr;
    HWND sourceEdit = nullptr;
    HWND targetEdit = nullptr;
    HWND mappingList = nullptr;
    HWND statusLabel = nullptr;
    HWND enableCheckBox = nullptr;
    HWND startupCheckBox = nullptr;

    bool remappingEnabled = true;
    bool trayIconAdded = false;
    CaptureMode captureMode = CaptureMode::None;

    std::filesystem::path configPath;
};

} // namespace app
