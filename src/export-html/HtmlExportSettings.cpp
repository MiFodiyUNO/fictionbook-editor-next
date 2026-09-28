#include "stdafx.h"
#include "HtmlExportSettings.h"

#include "TemplateResolver.h"

namespace {

int Clamp(DWORD value, int minimum, int maximum)
{
	return max(minimum, min(maximum, static_cast<int>(value)));
}

bool ReadBool(CRegKey& registry, LPCWSTR name, bool fallback)
{
	return U::QueryIV(registry, name, fallback ? 1 : 0) != 0;
}

int ReadInt(CRegKey& registry, LPCWSTR name, int fallback, int minimum, int maximum)
{
	return Clamp(U::QueryIV(registry, name, static_cast<DWORD>(fallback)), minimum, maximum);
}

void WriteBool(CRegKey& registry, LPCWSTR name, bool value)
{
	registry.SetDWORDValue(name, value ? 1 : 0);
}

void WriteInt(CRegKey& registry, LPCWSTR name, int value)
{
	registry.SetDWORDValue(name, static_cast<DWORD>(value));
}

}

void HtmlExportSettingsStore::Load(CRegKey& registry, HtmlExportSettings& settings)
{
	const ExportHtmlTemplateSelection templateSelection = ResolveExportHtmlTemplate(registry);
	settings.templatePath = templateSelection.path;
	settings.usingCustomTemplate = templateSelection.custom;
	settings.customCss = U::QuerySV(registry, L"CustomCss", L"");
	settings.includeToc = ReadBool(registry, L"IncludeToc", true);
	settings.tocDepth = ReadInt(registry, L"TOCDepth", 1, 1, 10);
	settings.imageMaxWidth = ReadInt(registry, L"ImageMaxWidth", 0, 0, 10000);
	settings.imageMaxHeight = ReadInt(registry, L"ImageMaxHeight", 0, 0, 10000);

	DWORD metadataValue = 0;
	const bool hasMetadataSettings = registry.QueryDWORDValue(L"IncludeAnnotation", metadataValue) == ERROR_SUCCESS;
	const bool legacyIncludeDescription = ReadBool(registry, L"IncludeDesc", true);
	settings.includeMetadata = ReadBool(registry, L"IncludeMetadata", legacyIncludeDescription);
	settings.includeDescription = ReadBool(registry, L"IncludeDescription", legacyIncludeDescription);
	settings.includeAnnotation = ReadBool(registry, L"IncludeAnnotation", legacyIncludeDescription);
	settings.includeTitleInfo = ReadBool(registry, L"IncludeTitleInfo", legacyIncludeDescription);
	settings.includeDocumentInfo = ReadBool(registry, L"IncludeDocumentInfo", legacyIncludeDescription);
	settings.includePublishInfo = ReadBool(registry, L"IncludePublishInfo", legacyIncludeDescription);
	settings.includeHistory = ReadBool(registry, L"IncludeHistory", legacyIncludeDescription);
	settings.includeAuthors = ReadBool(registry, L"IncludeAuthors", legacyIncludeDescription);
	settings.includeTranslators = ReadBool(registry, L"IncludeTranslators", legacyIncludeDescription);
	settings.includeCustomInfo = ReadBool(registry, L"IncludeCustomInfo", legacyIncludeDescription);
	if (!hasMetadataSettings) {
		settings.includeDescription = legacyIncludeDescription;
		settings.includeAnnotation = legacyIncludeDescription;
		settings.includeTitleInfo = legacyIncludeDescription;
		settings.includeDocumentInfo = legacyIncludeDescription;
		settings.includePublishInfo = legacyIncludeDescription;
		settings.includeHistory = legacyIncludeDescription;
		settings.includeAuthors = legacyIncludeDescription;
		settings.includeTranslators = legacyIncludeDescription;
		settings.includeCustomInfo = legacyIncludeDescription;
	}

	settings.documentStructure = ReadInt(registry, L"DocumentStructure", 0, 0, 1);
	settings.style = ReadInt(registry, L"Style", 0, 0, 2);
	settings.fontFamily = ReadInt(registry, L"FontFamily", 0, 0, 3);
	settings.customFontFamily = U::QuerySV(registry, L"CustomFontFamily", L"");
	settings.fontSize = ReadInt(registry, L"FontSize", 0, 0, 72);
	settings.lineHeight = ReadInt(registry, L"LineHeight", 0, 0, 200);
	if (settings.lineHeight != 0 && settings.lineHeight < 100) settings.lineHeight = 100;
	settings.contentMaxWidth = ReadInt(registry, L"ContentMaxWidth", 0, 0, 10000);
	settings.pageMargins = ReadInt(registry, L"PageMargins", 1, 0, 2);
	settings.textAlignment = ReadInt(registry, L"TextAlignment", 1, 0, 1);
	settings.headingAlignment = ReadInt(registry, L"HeadingAlignment", 0, 0, 1);
	settings.coverMode = ReadInt(registry, L"CoverMode", 0, 0, 2);
	settings.externalImagesFolderMode = ReadInt(registry, L"ExternalImagesFolderMode", 0, 0, 1);
	settings.externalImagesFolderName = U::QuerySV(registry, L"ExternalImagesFolderName", L"");
	settings.standaloneWarningMiB = ReadInt(registry, L"StandaloneWarningMiB", 50, 0, 10240);
	settings.notePlacement = ReadInt(registry, L"NotePlacement", 0, 0, 2);
}

void HtmlExportSettingsStore::Save(CRegKey& registry, const HtmlExportSettings& settings)
{
	registry.SetDWORDValue(L"UseCustomTemplate", settings.usingCustomTemplate ? 1 : 0);
	if (settings.usingCustomTemplate) registry.SetStringValue(L"Template", settings.templatePath);
	else registry.DeleteValue(L"Template");
	registry.SetStringValue(L"CustomCss", settings.customCss);
	WriteBool(registry, L"IncludeToc", settings.includeToc);
	WriteBool(registry, L"IncludeMetadata", settings.includeMetadata);
	WriteInt(registry, L"TOCDepth", max(1, min(10, settings.tocDepth)));
	WriteInt(registry, L"ImageMaxWidth", max(0, min(10000, settings.imageMaxWidth)));
	WriteInt(registry, L"ImageMaxHeight", max(0, min(10000, settings.imageMaxHeight)));
	WriteBool(registry, L"IncludeDesc", settings.includeDescription);
	WriteBool(registry, L"IncludeDescription", settings.includeDescription);
	WriteBool(registry, L"IncludeAnnotation", settings.includeAnnotation);
	WriteBool(registry, L"IncludeTitleInfo", settings.includeTitleInfo);
	WriteBool(registry, L"IncludeDocumentInfo", settings.includeDocumentInfo);
	WriteBool(registry, L"IncludePublishInfo", settings.includePublishInfo);
	WriteBool(registry, L"IncludeHistory", settings.includeHistory);
	WriteBool(registry, L"IncludeAuthors", settings.includeAuthors);
	WriteBool(registry, L"IncludeTranslators", settings.includeTranslators);
	WriteBool(registry, L"IncludeCustomInfo", settings.includeCustomInfo);
	WriteInt(registry, L"DocumentStructure", settings.documentStructure);
	WriteInt(registry, L"Style", settings.style);
	WriteInt(registry, L"FontFamily", settings.fontFamily);
	registry.SetStringValue(L"CustomFontFamily", settings.customFontFamily);
	WriteInt(registry, L"FontSize", settings.fontSize);
	WriteInt(registry, L"LineHeight", settings.lineHeight);
	WriteInt(registry, L"ContentMaxWidth", settings.contentMaxWidth);
	WriteInt(registry, L"PageMargins", settings.pageMargins);
	WriteInt(registry, L"TextAlignment", settings.textAlignment);
	WriteInt(registry, L"HeadingAlignment", settings.headingAlignment);
	WriteInt(registry, L"CoverMode", settings.coverMode);
	WriteInt(registry, L"ExternalImagesFolderMode", settings.externalImagesFolderMode);
	registry.SetStringValue(L"ExternalImagesFolderName", settings.externalImagesFolderName);
	WriteInt(registry, L"StandaloneWarningMiB", settings.standaloneWarningMiB);
	WriteInt(registry, L"NotePlacement", settings.notePlacement);
}
