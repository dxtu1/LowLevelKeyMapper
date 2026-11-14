#include <Windows.h>

#include "app/Application.h"

int WINAPI WinMain(
    HINSTANCE instance,
    HINSTANCE,
    LPSTR,
    int showCommand
) {
    return RunApplication(instance, showCommand);
}
