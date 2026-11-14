#include "input/KeyboardHook.h"

#include <utility>

KeyboardHook* KeyboardHook::s_instance = nullptr;

KeyboardHook::KeyboardHook(Callback callback)
    : m_callback(std::move(callback)) {
}

KeyboardHook::~KeyboardHook() {
    Uninstall();
}

bool KeyboardHook::Install() {
    if (m_hook != nullptr || s_instance != nullptr) {
        return false;
    }

    s_instance = this;

    m_hook = SetWindowsHookExW(
        WH_KEYBOARD_LL,
        HookProc,
        GetModuleHandleW(nullptr),
        0
    );

    if (m_hook == nullptr) {
        s_instance = nullptr;
        return false;
    }

    return true;
}

void KeyboardHook::Uninstall() {
    if (m_hook != nullptr) {
        UnhookWindowsHookEx(m_hook);
        m_hook = nullptr;
    }

    if (s_instance == this) {
        s_instance = nullptr;
    }
}

LRESULT CALLBACK KeyboardHook::HookProc(
    int nCode,
    WPARAM wParam,
    LPARAM lParam
) {
    if (nCode == HC_ACTION && s_instance != nullptr) {
        if (s_instance->HandleEvent(wParam, lParam)) {
            return 1;
        }
    }

    return CallNextHookEx(
        nullptr,
        nCode,
        wParam,
        lParam
    );
}

bool KeyboardHook::HandleEvent(
    WPARAM wParam,
    LPARAM lParam
) {
    const auto* key =
        reinterpret_cast<const KBDLLHOOKSTRUCT*>(lParam);

    const bool keyDown =
        wParam == WM_KEYDOWN ||
        wParam == WM_SYSKEYDOWN;

    const bool keyUp =
        wParam == WM_KEYUP ||
        wParam == WM_SYSKEYUP;

    if (!keyDown && !keyUp) {
        return false;
    }

    if (!m_callback) {
        return false;
    }

    const KeyEvent event{
        key->vkCode,
        key->scanCode,
        key->flags,
        keyDown,
        keyUp,
        (key->flags & LLKHF_INJECTED) != 0
    };

    return m_callback(event);
}