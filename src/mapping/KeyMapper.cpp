
#include "mapping/KeyMapper.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <fstream>
#include <filesystem>
#include <system_error>
#include <unordered_set>
#include <utility>

using json = nlohmann::json;

namespace {

    // 문자열을 가상 키 코드로 변환
    std::optional<DWORD> KeyNameToVK(std::string name) {

        std::transform(
            name.begin(),
            name.end(),
            name.begin(),
            [](unsigned char c) {
                return static_cast<char>(std::toupper(c));
            }
        );

        // A-Z
        if (name.size() == 1 &&
            name[0] >= 'A' && name[0] <= 'Z') {
            return static_cast<DWORD>(name[0]);
        }

        // 0-9
        if (name.size() == 1 &&
            name[0] >= '0' && name[0] <= '9') {
            return static_cast<DWORD>(name[0]);
        }

        // F1-F24
        if (name.size() >= 2 && name[0] == 'F') {
            try {
                const int number = std::stoi(name.substr(1));

                if (number >= 1 && number <= 24 &&
                    name == "F" + std::to_string(number)) {
                    return VK_F1 + number - 1;
                }
            }
            catch (...) {
                return std::nullopt;
            }
        }

        static const std::unordered_map<std::string, DWORD> keys = {

            {"ESC", VK_ESCAPE},
            {"SPACE", VK_SPACE},
            {"ENTER", VK_RETURN},
            {"TAB", VK_TAB},
            {"BACKSPACE", VK_BACK},
            {"DELETE", VK_DELETE},
            {"INSERT", VK_INSERT},
            {"HOME", VK_HOME},
            {"END", VK_END},
            {"PAGEUP", VK_PRIOR},
            {"PAGEDOWN", VK_NEXT},

            // Generic Modifier
            {"SHIFT", VK_SHIFT},
            {"CTRL", VK_CONTROL},
            {"ALT", VK_MENU},

            // Left / Right Modifier
            {"LSHIFT", VK_LSHIFT},
            {"RSHIFT", VK_RSHIFT},

            {"LCTRL", VK_LCONTROL},
            {"RCTRL", VK_RCONTROL},

            {"LALT", VK_LMENU},
            {"RALT", VK_RMENU},

            {"LWIN", VK_LWIN},
            {"RWIN", VK_RWIN},

            {"CAPSLOCK", VK_CAPITAL},
            {"NUMLOCK", VK_NUMLOCK},
            {"SCROLLLOCK", VK_SCROLL},

            {"UP", VK_UP},
            {"DOWN", VK_DOWN},
            {"LEFT", VK_LEFT},
            {"RIGHT", VK_RIGHT},

            {"PRINTSCREEN", VK_SNAPSHOT},
            {"PAUSE", VK_PAUSE},

            {"NUMPAD0", VK_NUMPAD0},
            {"NUMPAD1", VK_NUMPAD1},
            {"NUMPAD2", VK_NUMPAD2},
            {"NUMPAD3", VK_NUMPAD3},
            {"NUMPAD4", VK_NUMPAD4},
            {"NUMPAD5", VK_NUMPAD5},
            {"NUMPAD6", VK_NUMPAD6},
            {"NUMPAD7", VK_NUMPAD7},
            {"NUMPAD8", VK_NUMPAD8},
            {"NUMPAD9", VK_NUMPAD9},

            {"MULTIPLY", VK_MULTIPLY},
            {"ADD", VK_ADD},
            {"SUBTRACT", VK_SUBTRACT},
            {"DECIMAL", VK_DECIMAL},
            {"DIVIDE", VK_DIVIDE}
        };

        const auto it = keys.find(name);

        if (it == keys.end()) {
            return std::nullopt;
        }

        return it->second;
    }


    // 가상 키 코드를 문자열로 변환
    std::optional<std::string> VKToKeyName(DWORD vkCode) {

        // A-Z
        if (vkCode >= 'A' && vkCode <= 'Z') {
            return std::string(1, static_cast<char>(vkCode));
        }

        // 0-9
        if (vkCode >= '0' && vkCode <= '9') {
            return std::string(1, static_cast<char>(vkCode));
        }

        // F1-F24
        if (vkCode >= VK_F1 && vkCode <= VK_F24) {
            return "F" + std::to_string(vkCode - VK_F1 + 1);
        }

        static const std::unordered_map<DWORD, std::string> keys = {

            {VK_ESCAPE, "ESC"},
            {VK_SPACE, "SPACE"},
            {VK_RETURN, "ENTER"},
            {VK_TAB, "TAB"},
            {VK_BACK, "BACKSPACE"},
            {VK_DELETE, "DELETE"},
            {VK_INSERT, "INSERT"},
            {VK_HOME, "HOME"},
            {VK_END, "END"},
            {VK_PRIOR, "PAGEUP"},
            {VK_NEXT, "PAGEDOWN"},

            // Generic Modifier
            {VK_SHIFT, "SHIFT"},
            {VK_CONTROL, "CTRL"},
            {VK_MENU, "ALT"},

            // Left / Right Modifier
            {VK_LSHIFT, "LSHIFT"},
            {VK_RSHIFT, "RSHIFT"},

            {VK_LCONTROL, "LCTRL"},
            {VK_RCONTROL, "RCTRL"},

            {VK_LMENU, "LALT"},
            {VK_RMENU, "RALT"},

            {VK_LWIN, "LWIN"},
            {VK_RWIN, "RWIN"},

            {VK_CAPITAL, "CAPSLOCK"},
            {VK_NUMLOCK, "NUMLOCK"},
            {VK_SCROLL, "SCROLLLOCK"},

            {VK_UP, "UP"},
            {VK_DOWN, "DOWN"},
            {VK_LEFT, "LEFT"},
            {VK_RIGHT, "RIGHT"},

            {VK_SNAPSHOT, "PRINTSCREEN"},
            {VK_PAUSE, "PAUSE"},

            {VK_NUMPAD0, "NUMPAD0"},
            {VK_NUMPAD1, "NUMPAD1"},
            {VK_NUMPAD2, "NUMPAD2"},
            {VK_NUMPAD3, "NUMPAD3"},
            {VK_NUMPAD4, "NUMPAD4"},
            {VK_NUMPAD5, "NUMPAD5"},
            {VK_NUMPAD6, "NUMPAD6"},
            {VK_NUMPAD7, "NUMPAD7"},
            {VK_NUMPAD8, "NUMPAD8"},
            {VK_NUMPAD9, "NUMPAD9"},

            {VK_MULTIPLY, "MULTIPLY"},
            {VK_ADD, "ADD"},
            {VK_SUBTRACT, "SUBTRACT"},
            {VK_DECIMAL, "DECIMAL"},
            {VK_DIVIDE, "DIVIDE"}
        };

        const auto it = keys.find(vkCode);

        if (it == keys.end()) {
            return std::nullopt;
        }

        return it->second;
    }

}


// 매핑 등록
void KeyMapper::SetMapping(
    DWORD source,
    std::vector<DWORD> targets
) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_mappings[source] = std::move(targets);
}


// 매핑 삭제
void KeyMapper::RemoveMapping(DWORD source) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_mappings.erase(source);
}

void KeyMapper::ClearMappings() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_mappings.clear();
}


// 매핑 조회
std::optional<std::vector<DWORD>> KeyMapper::FindMapping(
    DWORD source
) const {
    std::lock_guard<std::mutex> lock(m_mutex);

    const auto it = m_mappings.find(source);

    if (it == m_mappings.end()) {
        return std::nullopt;
    }

    return it->second;
}


// 매핑 가져오기
std::unordered_map<DWORD, std::vector<DWORD>>
KeyMapper::GetMappings() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_mappings;
}


// JSON 파일에서 매핑 불러오기
bool KeyMapper::LoadFromFile(
    const std::string& path,
    std::string& error
) {
    error.clear();

    // 파일이 없으면 기본 설정 생성
    {
        std::ifstream existing(path);

        if (!existing.good()) {
            std::ofstream output(path);

            if (!output) {
                error = "Failed to create config file.";
                return false;
            }

            output << R"(
            {
                "mappings": {
                
                }
            }
            )";
        }
    }

    std::ifstream file(path);

    if (!file) {
        error = "Failed to open config file.";
        return false;
    }

    json config;

    try {
        file >> config;
    }
    catch (const json::exception& e) {
        error = std::string("JSON parsing failed: ") + e.what();
        return false;
    }

    if (!config.is_object() ||
        !config.contains("mappings") ||
        !config["mappings"].is_object()) {
        error = "Invalid config structure.";
        return false;
    }

    std::unordered_map<DWORD, std::vector<DWORD>> newMappings;

    for (const auto& [sourceName, targetValue] :
         config["mappings"].items()) {

        const auto source = ParseKeyName(sourceName);

        if (!source.has_value()) {
            error = "Unknown source key: " + sourceName;
            return false;
        }

        std::vector<std::string> targetNames;

        // 기존 문자열 방식 지원
        if (targetValue.is_string()) {
            targetNames.push_back(
                targetValue.get<std::string>()
            );
        }

        // 배열 방식 지원
        else if (targetValue.is_array()) {

            if (targetValue.empty()) {
                error = "Target array cannot be empty: " + sourceName;
                return false;
            }

            for (const auto& item : targetValue) {

                if (!item.is_string()) {
                    error = "Target keys must be strings: " + sourceName;
                    return false;
                }

                targetNames.push_back(
                    item.get<std::string>()
                );
            }
        }
        else {
            error = "Invalid mapping value: " + sourceName;
            return false;
        }

        std::vector<DWORD> targets;
        std::unordered_set<DWORD> usedTargets;

        for (const auto& targetName : targetNames) {

            const auto target = ParseKeyName(targetName);

            if (!target.has_value()) {
                error = "Unknown target key: " + targetName;
                return false;
            }

            // 동일한 대상 키 중복 방지
            if (usedTargets.insert(*target).second) {
                targets.push_back(*target);
            }
        }

        if (newMappings.contains(*source)) {
            error = "Duplicate source key: " + sourceName;
            return false;
        }

        newMappings[*source] = std::move(targets);
    }

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_mappings.swap(newMappings);
    }

    return true;
}


// JSON 파일에 현재 매핑 저장
bool KeyMapper::SaveToFile(
    const std::string& path,
    std::string& error
) const {
    error.clear();
    std::lock_guard<std::mutex> lock(m_mutex);

    json config;
    config["mappings"] = json::object();

    for (const auto& [source, targets] : m_mappings) {

        const auto sourceName = KeyCodeToName(source);

        if (!sourceName.has_value()) {
            error = "Unknown source scan code: " +
                    std::to_string(source);
            return false;
        }

        json targetArray = json::array();

        for (DWORD target : targets) {

            const auto targetName = KeyCodeToName(target);

            if (!targetName.has_value()) {
                error = "Unknown target scan code: " +
                        std::to_string(target);
                return false;
            }

            targetArray.push_back(*targetName);
        }

        config["mappings"][*sourceName] = targetArray;
    }

    const std::string tempPath = path + ".tmp";
    const std::string backupPath = path + ".bak";
    {
        std::ofstream file(tempPath, std::ios::binary | std::ios::trunc);
        if (!file) { error = "Failed to create temporary config file."; return false; }
        file << config.dump(4) << "\n";
        file.flush();
        if (!file.good()) { error = "Failed to write temporary config file."; return false; }
    }

    // Keep the previous valid config as a backup before replacing it.
    std::ifstream current(path, std::ios::binary);
    if (current.good()) {
        current.close();
        std::error_code ec;
        std::filesystem::copy_file(path, backupPath,
            std::filesystem::copy_options::overwrite_existing, ec);
        if (ec) { error = "Could not create config backup: " + ec.message(); return false; }
    }

    std::error_code ec;
    std::filesystem::rename(tempPath, path, ec);
    if (ec) {
        // Windows rename does not replace an existing destination.
        std::filesystem::remove(path, ec);
        ec.clear();
        std::filesystem::rename(tempPath, path, ec);
    }
    if (ec) { error = "Could not replace config file: " + ec.message(); return false; }
    return true;
}

std::optional<DWORD> KeyMapper::ParseKeyName(
    const std::string& name
) {
    const auto vk = KeyNameToVK(name);
    if (!vk) return std::nullopt;

    UINT mapped = MapVirtualKeyExW(
        *vk, MAPVK_VK_TO_VSC_EX, GetKeyboardLayout(0)
    );
    if (mapped == 0) return std::nullopt;

    // Internal representation: low byte = scan code, bit 8 = E0 prefix.
    if ((mapped & 0xFF00u) == 0xE000u)
        return static_cast<DWORD>((mapped & 0xFFu) | 0x100u);

    return static_cast<DWORD>(mapped & 0xFFu);
}

std::optional<std::string> KeyMapper::KeyCodeToName(
    DWORD scanCode
) {
    UINT mapped = scanCode & 0xFFu;
    if (scanCode & 0x100u) mapped |= 0xE000u;

    const UINT vk = MapVirtualKeyExW(
        mapped, MAPVK_VSC_TO_VK_EX, GetKeyboardLayout(0)
    );
    if (vk == 0) return std::nullopt;

    return VKToKeyName(vk);
}