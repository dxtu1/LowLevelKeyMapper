#include <algorithm>
#include "input/KeyStateManager.h"

namespace {

    // Scan-code representation: bit 8 marks an E0-prefixed key.
    bool IsModifierKey(DWORD scanCode) {
        return scanCode == 0x2A || scanCode == 0x36 || // Shift
               scanCode == 0x1D || scanCode == 0x11D || // Ctrl
               scanCode == 0x38 || scanCode == 0x138 || // Alt
               scanCode == 0x15B || scanCode == 0x15C;   // Windows
    }

}

bool KeyStateManager::SendKey(
    DWORD scanCode,
    bool isDown
) {
    INPUT input{};
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = 0;
    input.ki.wScan = static_cast<WORD>(scanCode & 0xFFu);
    input.ki.dwFlags = KEYEVENTF_SCANCODE |
        ((scanCode & 0x100u) ? KEYEVENTF_EXTENDEDKEY : 0) |
        (isDown ? 0 : KEYEVENTF_KEYUP);

    return SendInput(1, &input, sizeof(INPUT)) == 1;
}

InputResult KeyStateManager::SendKeyBatch(
    const std::vector<DWORD>& keys,
    bool isDown
) {
    if (keys.empty()) return { 0, 0 };

    std::vector<INPUT> inputs;
    inputs.reserve(keys.size());

    for (DWORD scanCode : keys) {
        INPUT input{};
        input.type = INPUT_KEYBOARD;
        input.ki.wVk = 0;
        input.ki.wScan = static_cast<WORD>(scanCode & 0xFFu);
        input.ki.dwFlags = KEYEVENTF_SCANCODE |
            ((scanCode & 0x100u) ? KEYEVENTF_EXTENDEDKEY : 0) |
            (isDown ? 0 : KEYEVENTF_KEYUP);
        inputs.push_back(input);
    }

    UINT sent = SendInput(
        static_cast<UINT>(inputs.size()), inputs.data(), sizeof(INPUT)
    );
    return { static_cast<UINT>(inputs.size()), sent };
}

bool KeyStateManager::RetryPendingReleases() {
    if (m_pendingReleases.empty()) {
        return true;
    }

    std::vector<DWORD> keys(
        m_pendingReleases.begin(),
        m_pendingReleases.end()
    );

    InputResult result = SendKeyBatch(keys, false);

    // 성공적으로 해제된 키만 추적 목록에서 제거
    for (UINT i = 0; i < result.sent; ++i) {
        m_pendingReleases.erase(keys[i]);
    }

    return result.IsComplete();
}

bool KeyStateManager::IsPhysicallyPressed(
    DWORD scanCode
) const {
    for (DWORD physicalScanCode : m_physicalScanCodes) {
        if (scanCode == physicalScanCode) {
            return true;
        }
    }

    return false;
}

bool KeyStateManager::IsTargetActive(
    DWORD scanCode
) const {
    for (const auto& [target, count] : m_targetCounts) {
        if (count > 0 && scanCode == target) {
            return true;
        }
    }

    return false;
}

bool KeyStateManager::HandlePhysicalEvent(
    DWORD scanCode,
    bool isDown
) {
    if (isDown) {
        m_physicalScanCodes.insert(scanCode);
    }
    else {
        m_physicalScanCodes.erase(scanCode);
    }

    return IsTargetActive(scanCode);
}

bool KeyStateManager::HandleEvent(
    DWORD source,
    const std::vector<DWORD>& targets,
    bool isDown
) {
    RetryPendingReleases();

    auto sourceIt = m_activeSources.find(source);

    // KEY DOWN
    if (isDown) {

        // 자동 반복 처리
        if (sourceIt != m_activeSources.end()) {

            std::vector<DWORD> repeatKeys;

            for (DWORD target : sourceIt->second) {
                if (!IsModifierKey(target)) {
                    repeatKeys.push_back(target);
                }
            }

            return SendKeyBatch(repeatKeys, true).IsComplete();
        }

        if (targets.empty()) {
            return false;
        }

        // 중복 대상 키 제거
        std::vector<DWORD> uniqueTargets;

        for (DWORD target : targets) {
            if (std::find(
                uniqueTargets.begin(),
                uniqueTargets.end(),
                target
            ) == uniqueTargets.end()) {
                uniqueTargets.push_back(target);
            }
        }

        std::vector<DWORD> keysToPress;

        // 실제 입력이 필요한 대상 키만 선별
        for (DWORD target : uniqueTargets) {

            auto it = m_targetCounts.find(target);

            if (it != m_targetCounts.end() && it->second > 0) {
                continue;
            }

            if (IsPhysicallyPressed(target)) {
                continue;
            }

            keysToPress.push_back(target);
        }

        // 대상 키를 한 번의 호출로 입력
        InputResult result = SendKeyBatch(keysToPress, true);

        if (!result.IsComplete()) {

            // 실제로 전송된 키만 추출
            std::vector<DWORD> rollbackKeys(
                keysToPress.begin(),
                keysToPress.begin() + result.sent
            );

            // 입력 순서의 역순으로 해제
            std::reverse(rollbackKeys.begin(), rollbackKeys.end());

            // 이미 전송된 키들을 해제
            InputResult rollbackResult =
                SendKeyBatch(rollbackKeys, false);

            // 해제에 실패한 키는 별도로 추적
            for (size_t i = rollbackResult.sent;
                i < rollbackKeys.size();
                ++i) {

                m_pendingReleases.insert(rollbackKeys[i]);
            }

            return false;
        }

        // 입력 성공 후 참조 횟수 기록
        for (DWORD target : uniqueTargets) {
            ++m_targetCounts[target];
        }

        m_activeSources[source] = uniqueTargets;

        return true;
    }

    // KEY UP
    if (sourceIt == m_activeSources.end()) {
        return false;
    }

    const auto& activeTargets = sourceIt->second;

    bool success = true;

    // KEY DOWN 역순으로 KEY UP
    for (auto it = activeTargets.rbegin();
        it != activeTargets.rend();
        ++it) {

        DWORD target = *it;

        auto targetIt = m_targetCounts.find(target);

        if (targetIt == m_targetCounts.end()) {
            continue;
        }

        if (targetIt->second <= 1) {

            // 실제 물리 키가 눌려 있지 않을 때만 해제
            if (!IsPhysicallyPressed(target)) {
                if (!SendKey(target, false)) {
                    // 원본 KEY UP 이벤트는 다시 오지 않을 수 있으므로
                    // 해제 실패 키를 별도 재시도 목록으로 옮긴다.
                    m_pendingReleases.insert(target);
                    success = false;
                }
            }

            m_targetCounts.erase(targetIt);
        }
        else {
            --targetIt->second;
        }
    }

    // 해제에 실패한 키는 m_pendingReleases가 추적하므로
    // 원본 키의 활성 상태는 여기서 정리한다.
    m_activeSources.erase(sourceIt);
    (void)success;
    return true;
}

void KeyStateManager::ReleaseAll() {
    RetryPendingReleases();
    

    for (auto it = m_targetCounts.begin();
         it != m_targetCounts.end();) {

        DWORD target = it->first;
        unsigned int count = it->second;

        if (count > 0 && !IsPhysicallyPressed(target)) {

            if (!SendKey(target, false)) {
                ++it;
                continue;
            }
        }

        it = m_targetCounts.erase(it);
    }

    m_activeSources.clear();
    m_physicalScanCodes.clear();
}
