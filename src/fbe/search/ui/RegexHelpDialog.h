#pragma once

#include "..\\SearchPreset.h"

// Modal, native Win32 help: no browser/WebView dependency and safe on Win7.
void ShowRegexHelpDialog(HWND owner, FbeSearchPresets::SearchUiContext context);