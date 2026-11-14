#pragma once

#include <Windows.h>

#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

class KeyMapper {
public:
    void SetMapping(DWORD source, std::vector<DWORD> targets);

    void RemoveMapping(DWORD source);

    void ClearMappings();

    std::optional<std::vector<DWORD>> FindMapping(DWORD source) const;

    std::unordered_map<DWORD, std::vector<DWORD>> GetMappings() const;

    bool LoadFromFile(
        const std::string& path,
        std::string& error
    );

    bool SaveToFile(
        const std::string& path,
        std::string& error
    ) const;

    static std::optional<DWORD> ParseKeyName(
        const std::string& name
    );

    static std::optional<std::string> KeyCodeToName(
        DWORD scanCode
    );

private:
    mutable std::mutex m_mutex;

    std::unordered_map<DWORD, std::vector<DWORD>> m_mappings;
};