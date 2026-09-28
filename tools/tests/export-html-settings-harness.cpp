#include "../../src/export-html/stdafx.h"
#include "../../src/export-html/HtmlExportSettings.h"

#include <iostream>

CComModule _Module;
CRegKey _Settings;
CString _SettingsPath;

static bool Expect(bool value, const char* message)
{
	if (!value) std::cerr << message << std::endl;
	return value;
}

int main()
{
	_Module.Init(NULL, ::GetModuleHandle(NULL));
	CRegKey registry;
	const CString path = L"Software\\FBETeam\\ExportHtmlSettingsHarness";
	if (registry.Create(HKEY_CURRENT_USER, path) != ERROR_SUCCESS) return 1;
	registry.SetDWORDValue(L"IncludeDesc", 0);
	registry.SetDWORDValue(L"TOCDepth", 99);
	registry.SetDWORDValue(L"ImageMaxWidth", 50000);
	registry.SetDWORDValue(L"ImageMaxHeight", 50000);

	HtmlExportSettings settings;
	HtmlExportSettingsStore::Load(registry, settings);
	bool ok = true;
	ok &= Expect(!settings.includeDescription && !settings.includeAnnotation && !settings.includeCustomInfo, "IncludeDesc migration failed");
	ok &= Expect(settings.tocDepth == 10 && settings.imageMaxWidth == 10000 && settings.imageMaxHeight == 10000, "numeric clamping failed");
	settings.includeDescription = true;
	settings.includeAnnotation = false;
	settings.customCss = L"C:\\Тест\\стиль.css";
	settings.externalImagesFolderMode = 1;
	settings.externalImagesFolderName = L"книга_files";
	settings.notePlacement = 2;
	settings.fontSize = 18;
	HtmlExportSettingsStore::Save(registry, settings);

	HtmlExportSettings roundTrip;
	HtmlExportSettingsStore::Load(registry, roundTrip);
	ok &= Expect(roundTrip.includeDescription && !roundTrip.includeAnnotation, "per-section metadata persistence failed");
	ok &= Expect(roundTrip.customCss == settings.customCss && roundTrip.externalImagesFolderName == settings.externalImagesFolderName, "Unicode path persistence failed");
	ok &= Expect(roundTrip.notePlacement == 2 && roundTrip.fontSize == 18, "new settings persistence failed");
	registry.Close();
	CRegKey parent;
	if (parent.Open(HKEY_CURRENT_USER, L"Software\\FBETeam", KEY_WRITE) == ERROR_SUCCESS)
		parent.RecurseDeleteKey(L"ExportHtmlSettingsHarness");
	_Module.Term();
	return ok ? 0 : 1;
}
