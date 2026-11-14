#pragma once

#include <Windows.h>

#include <unordered_map>
#include <unordered_set>
#include <vector>

struct InputResult {
    UINT requested;
    UINT sent;

    bool IsComplete() const {
        return requested == sent;
    }
};

class KeyStateManager {
public:
    bool HandleEvent(
        DWORD source,
        const std::vector<DWORD>& targets,
        bool isDown
    );

    // 반환값: 원래 물리 입력을 차단할지 여부
    bool HandlePhysicalEvent(
        DWORD scanCode,
        bool isDown
    );

    void ReleaseAll();

private:
    bool SendKey(DWORD scanCode, bool isDown);

    bool IsPhysicallyPressed(DWORD scanCode) const;
    bool IsTargetActive(DWORD scanCode) const;

    std::unordered_set<DWORD> m_physicalScanCodes;

    std::unordered_map<DWORD, std::vector<DWORD>> m_activeSources;

    std::unordered_map<DWORD, unsigned int> m_targetCounts;

    InputResult SendKeyBatch(const std::vector<DWORD>& keys, bool isDown);

    std::unordered_set<DWORD> m_pendingReleases;
    
    bool RetryPendingReleases();
};
