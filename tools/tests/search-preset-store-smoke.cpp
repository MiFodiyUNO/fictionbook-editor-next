#include "../../src/fbe/stdafx.h"
#include "../../src/fbe/search/SearchPresetStore.h"

#include <iostream>

using namespace FbeSearchPresets;

namespace
{
CString TempDirectory()
{
    wchar_t root[MAX_PATH] = {};
    if (::GetTempPathW(_countof(root), root) == 0) return CString();
    wchar_t temporary[MAX_PATH] = {};
    if (::GetTempFileNameW(root, L"fsp", 0, temporary) == 0) return CString();
    ::DeleteFileW(temporary);
    if (!::CreateDirectoryW(temporary, NULL)) return CString();
    return CString(temporary);
}

void WriteUtf16(const CString& path, const CString& text)
{
    HANDLE file = ::CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) throw 100;
    DWORD written = 0;
    const DWORD bytes = static_cast<DWORD>(text.GetLength() * sizeof(wchar_t));
    const bool ok = ::WriteFile(file, static_cast<LPCWSTR>(text), bytes, &written, NULL) != FALSE && written == bytes;
    ::CloseHandle(file);
    if (!ok) throw 101;
}

SearchPreset MakePreset(const wchar_t* id, SearchUiContext context, bool hasReplacement)
{
    SearchPreset preset;
    preset.id = id;
    preset.name = L"Имя & < > \" ' \U0001F642";
    preset.findText = L"строка & <tag> \" ' \r\nвторая";
    preset.hasReplacement = hasReplacement;
    preset.replacementText = L"";
    preset.regexp = true;
    preset.matchCase = true;
    preset.wholeWord = false;
    preset.unicodeProperties = context == SearchUiContext::Design;
    preset.context = context;
    return preset;
}

bool Same(const SearchPreset& left, const SearchPreset& right)
{
    return left.id == right.id && left.name == right.name && left.findText == right.findText &&
        left.hasReplacement == right.hasReplacement && left.replacementText == right.replacementText &&
        left.regexp == right.regexp && left.matchCase == right.matchCase &&
        left.wholeWord == right.wholeWord && left.unicodeProperties == right.unicodeProperties &&
        left.context == right.context && !left.builtIn && !right.builtIn;
}
}

int wmain()
{
    ::CoInitialize(NULL);
    int result = 0;
    CString directory = TempDirectory();
    if (directory.IsEmpty()) return 1;
    try
    {
        SearchPresetStore store(directory);
        std::vector<SearchPreset> loaded;
        if (!store.Load(loaded) || !loaded.empty()) result = 2; // empty store

        std::vector<SearchPreset> expected;
        expected.push_back(MakePreset(L"one", SearchUiContext::Design, true)); // empty replacement means delete
        expected.push_back(MakePreset(L"two", SearchUiContext::Source, false)); // find-only
        if (result == 0 && !store.Save(expected)) result = 3;
        HANDLE saved = ::CreateFileW(store.FilePath(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        wchar_t bom = 0; DWORD bomRead = 0;
        const bool hasUtf16Bom = saved != INVALID_HANDLE_VALUE && ::ReadFile(saved, &bom, sizeof(bom), &bomRead, NULL) != FALSE && bomRead == sizeof(bom) && bom == 0xFEFF;
        if (saved != INVALID_HANDLE_VALUE) ::CloseHandle(saved);
        if (result == 0 && !hasUtf16Bom) result = 13;
        loaded.clear();
        if (result == 0 && (!store.Load(loaded) || loaded.size() != expected.size() || !Same(loaded[0], expected[0]) || !Same(loaded[1], expected[1]))) result = 4;

        WriteUtf16(store.FilePath(), L"<SearchTemplates version=\"1\"><Template id=\"bad\" /></SearchTemplates>");
        loaded.clear();
        if (result == 0 && (!store.Load(loaded) || !loaded.empty())) result = 5; // malformed Template is skipped safely

        WriteUtf16(store.FilePath(), L"<SearchTemplates version=\"1\"><Template>");
        if (result == 0 && store.Load(loaded)) result = 6; // malformed XML

        WriteUtf16(store.FilePath(), L"<SearchTemplates version=\"2\" />");
        if (result == 0 && store.Load(loaded)) result = 7; // unsupported version
        WriteUtf16(store.FilePath(), L"<!DOCTYPE SearchTemplates [<!ENTITY e SYSTEM 'file:///not-allowed'>]><SearchTemplates version=\"1\"><Template id=\"a\" name=\"A\" context=\"design\" regexp=\"1\" matchCase=\"0\" wholeWord=\"0\" unicodeProperties=\"0\" hasReplacement=\"0\"><Find>&e;</Find></Template></SearchTemplates>");
        if (result == 0 && store.Load(loaded)) result = 8; // DTD and external entities are prohibited

        WriteUtf16(store.FilePath(), L"<SearchTemplates version=\"1\"><Template id=\"a\" name=\"A\" context=\"design\" regexp=\"1\" matchCase=\"0\" wholeWord=\"0\" unicodeProperties=\"0\" hasReplacement=\"0\"><Find>x</Find></Template><Template id=\"a\" name=\"B\" context=\"design\" regexp=\"1\" matchCase=\"0\" wholeWord=\"0\" unicodeProperties=\"0\" hasReplacement=\"0\"><Find>y</Find></Template></SearchTemplates>");
        loaded.clear();
        if (result == 0 && (!store.Load(loaded) || loaded.size() != 1 || loaded[0].findText != L"x")) result = 10; // duplicate id skipped

        std::vector<SearchPreset> tooMany;
        for (int index = 0; index != 257; ++index) { CString id; id.Format(L"id-%d", index); tooMany.push_back(MakePreset(id, SearchUiContext::Design, false)); }
        if (result == 0 && store.Save(tooMany)) result = 10;

        if (result == 0 && !store.Save(expected)) result = 10;
#if defined(FBE_SEARCH_PRESET_STORE_TESTING)
        expected[0].name = L"new value";
        store.FailNextWriteForTesting();
        if (result == 0 && store.Save(expected)) result = 10;
        loaded.clear();
        if (result == 0 && (!store.Load(loaded) || loaded.size() != 2 || loaded[0].name == L"new value")) result = 11; // old good file survives failed atomic write
#endif

        if (result == 0 && !store.Save(expected)) result = 14;
        std::vector<SearchPreset> oversized;
        for (int index = 0; index != 256; ++index)
        {
            CString id;
            id.Format(L"oversized-%d", index);
            SearchPreset preset = MakePreset(id, SearchUiContext::Design, false);
            preset.findText = CString(L'x', 8192);
            oversized.push_back(preset);
        }
        if (result == 0 && store.Save(oversized)) result = 15;
        loaded.clear();
        if (result == 0 && (!store.Load(loaded) || loaded.size() != expected.size())) result = 16; // oversized save preserves old file
    }
    catch (...) { result = 12; }
    ::DeleteFileW(directory + L"\\SearchTemplates.xml");
    ::DeleteFileW(directory + L"\\SearchTemplates.xml.tmp");
    ::RemoveDirectoryW(directory);
    ::CoUninitialize();
    return result;
}
