#pragma once

#include "SearchPreset.h"
#include <vector>

namespace FbeSearchPresets
{
class SearchPresetStore
{
public:
    // A supplied directory gives tests an isolated portable-like store.
    // Production defaults to DeploymentContext::SettingsDirectory().
    explicit SearchPresetStore(const CString& settingsDirectory = CString());

    bool Load(std::vector<SearchPreset>& presets) const;
    bool Save(const std::vector<SearchPreset>& presets) const;
    CString FilePath() const;

#if defined(FBE_SEARCH_PRESET_STORE_TESTING)
    void FailNextWriteForTesting() { m_failNextWrite = true; }
#endif

private:
    CString m_settingsDirectory;
#if defined(FBE_SEARCH_PRESET_STORE_TESTING)
    mutable bool m_failNextWrite;
#endif
};
}