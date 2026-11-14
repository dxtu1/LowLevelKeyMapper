#pragma once

namespace app {

struct AppState;

void AddTrayIcon(AppState& app);
void RemoveTrayIcon(AppState& app);
void ShowTrayMenu(AppState& app);

} // namespace app
