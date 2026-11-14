#pragma once

#include <Windows.h>
#include <functional>

struct KeyEvent {
    DWORD vkCode;
    DWORD scanCode;
    DWORD flags;

    bool isDown;
    bool isUp;
    bool isInjected;
};

class KeyboardHook {
public:
    using Callback = std::function<bool(const KeyEvent&)>;

    explicit KeyboardHook(Callback callback);
    ~KeyboardHook();

    bool Install();
    void Uninstall();

private:
    static LRESULT CALLBACK HookProc(
        int nCode,
        WPARAM wParam,
        LPARAM lParam
    );

    bool HandleEvent(WPARAM wParam, LPARAM lParam);

    HHOOK m_hook = nullptr;
    Callback m_callback;

    static KeyboardHook* s_instance;
};