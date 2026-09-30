#ifndef SEARCHREPLACE_H
#define SEARCHREPLACE_H

#include "ModelessDialog.h"
#include "Settings.h"
#include "settings\\ui\\SettingsTooltips.h"
#include "RuntimeLocalization.h"
#include "utils.h"
#include "apputils.h"
#include "search\\SearchPresetCatalog.h"
#include "search\\SearchPresetStore.h"
#include "search\\ui\\RegexHelpDialog.h"
#include "search\\RegexQuickReference.h"
#include "search\\ui\\RegexQuickReferencePopup.h"
#include <vector>

inline CString MakePresetPreviewValue(const CString& source, int limit = 112)
{
    CString value;
    for(int index = 0; index < source.GetLength(); ++index) {
        const wchar_t character = source[index];
        if(character == L'\t') value += L"\\t";
        else if(character == L'\r') value += L"\\r";
        else if(character == L'\n') value += L"\\n";
        else value += character;
        if(value.GetLength() > limit) {
            int end = limit;
            if(end > 0 && end < value.GetLength() && value[end - 1] >= 0xD800 && value[end - 1] <= 0xDBFF && value[end] >= 0xDC00 && value[end] <= 0xDFFF) --end;
            value = value.Left(end) + L"\x2026";
            break;
        }
    }
    return value;
}

extern CSettings _Settings;
extern bool VBErr;

class FRBase: public CWinDataExchange<FRBase>
{
public:
	CRegKey		m_fh,m_rh;

	CFBEView*	m_view;
	int			m_whole;
	int			m_case;
	int			m_regexp;
	int			m_dir;
	int			m_unicode;
	int			m_scope;
	CEdit		m_text;
	CSettingsTooltips m_tooltips;
    bool m_templatesExpanded;
    int m_lastRegexTarget;
    int m_compactDialogWidth;
    int m_compactDialogHeight;
    std::vector<FbeSearchPresets::SearchPreset> m_panelPresets;

    static std::vector<FRBase*>& OpenPresetPanels() { static std::vector<FRBase*> panels; return panels; }
    void NotifyOpenPresetPanels()
    {
        std::vector<FRBase*>& panels = OpenPresetPanels();
        for(std::vector<FRBase*>::iterator panel = panels.begin(); panel != panels.end();) {
            if(*panel == NULL || !::IsWindow((*panel)->DialogWindow())) { panel = panels.erase(panel); continue; }
            if(*panel != this && (*panel)->m_templatesExpanded) (*panel)->RefreshPresetPanel();
            ++panel;
        }
    }

    LRESULT OnDestroyPresetPanel(UINT, WPARAM, LPARAM, BOOL&)
    {
        std::vector<FRBase*>& panels = OpenPresetPanels();
        panels.erase(std::remove(panels.begin(), panels.end(), this), panels.end());
        return 0;
    }

    FRBase(CFBEView* view) : m_view(view), m_whole(0), m_case(0), m_regexp(0), m_dir(1), m_unicode(0), m_scope(0), m_templatesExpanded(false), m_lastRegexTarget(IDC_TEXT), m_compactDialogWidth(0), m_compactDialogHeight(0) { }

  HWND	GetDlgItem(int id) { return X_GetDlgItem(id); }
  virtual HWND X_GetDlgItem(int id) = 0;
  BOOL	SetDlgItemText(int id,const TCHAR *str) { return ::SetWindowText(GetDlgItem(id),str); }

  void SetRuntimeText(int id, LPCWSTR key, LPCWSTR fallback)
  {
    const CString text = FbeLoadRuntimeStringByKey(key, fallback);
    if (!text.IsEmpty() && GetDlgItem(id))
      SetDlgItemText(id, text);
  }

  void SetRuntimeDialogTitle(LPCWSTR key, LPCWSTR fallback)
  {
    const CString text = FbeLoadRuntimeStringByKey(key, fallback);
    HWND probe = GetDlgItem(IDC_TEXT);
    HWND dialog = probe ? ::GetParent(probe) : NULL;
    if (!text.IsEmpty() && dialog)
      ::SetWindowText(dialog, text);
  }

    virtual FbeSearchPresets::SearchUiContext SearchContext() const
    {
        return FbeSearchPresets::SearchUiContext::Design;
    }

    virtual void InvalidateSearchSelectionState() {}

    HWND DialogWindow() const
    {
        const HWND text = const_cast<FRBase*>(this)->GetDlgItem(IDC_TEXT);
        return text ? ::GetParent(text) : NULL;
    }

    bool IsReplaceDialog() const { return const_cast<FRBase*>(this)->GetDlgItem(IDC_REPLACE) != NULL; }

    static HTREEITEM InsertPresetTreeItem(HWND tree, HTREEITEM parent, const CString& text, LPARAM data)
    {
        TVINSERTSTRUCTW item = {};
        item.hParent = parent;
        item.hInsertAfter = TVI_LAST;
        item.item.mask = TVIF_TEXT | TVIF_PARAM;
        item.item.pszText = const_cast<LPWSTR>(static_cast<LPCWSTR>(text));
        item.item.lParam = data;
        return reinterpret_cast<HTREEITEM>(::SendMessage(tree, TVM_INSERTITEMW, 0, reinterpret_cast<LPARAM>(&item)));
    }

    const FbeSearchPresets::SearchPreset* SelectedPreset() const
    {
        const HWND tree = const_cast<FRBase*>(this)->GetDlgItem(IDC_FIND_PRESETS_TREE);
        const HTREEITEM selected = tree ? TreeView_GetSelection(tree) : NULL;
        if (!selected) return NULL;
        TVITEM item = {}; item.mask = TVIF_PARAM; item.hItem = selected;
        if (!TreeView_GetItem(tree, &item) || item.lParam < 0 ||
            static_cast<size_t>(item.lParam) >= m_panelPresets.size()) return NULL;
        return &m_panelPresets[static_cast<size_t>(item.lParam)];
    }

    void UpdatePresetActions()
    {
        const FbeSearchPresets::SearchPreset* preset = SelectedPreset();
        const bool custom = preset != NULL && !preset->builtIn;
        const HWND dialog = DialogWindow();
        if (!dialog) return;
        ::EnableWindow(GetDlgItem(IDC_FIND_PRESET_APPLY), preset != NULL);
        const HWND textControl = GetDlgItem(IDC_TEXT);
        const int findTextLength = textControl ? ::GetWindowTextLength(textControl) : 0;
        ::EnableWindow(GetDlgItem(IDC_FIND_PRESET_SAVE), findTextLength > 0);
        ::EnableWindow(GetDlgItem(IDC_FIND_PRESET_UPDATE), custom);
        ::EnableWindow(GetDlgItem(IDC_FIND_PRESET_RENAME), custom);
        ::EnableWindow(GetDlgItem(IDC_FIND_PRESET_DELETE), custom);
        CString description;
        if (preset)
        {
            description = preset->description;
            const CString findLabel = FbeLoadRuntimeStringByKey(L"fbe.search_preset.preview.find", L"Find: %s");
            CString operation; operation.Format(findLabel, static_cast<LPCWSTR>(MakePresetPreviewValue(preset->findText)));
            if(!description.IsEmpty()) description += L"\r\n\r\n";
            description += operation;
            if(preset->hasReplacement) {
                const CString replacement = preset->replacementText.IsEmpty()
                    ? FbeLoadRuntimeStringByKey(L"fbe.search_preset.preview.empty", L"<empty>")
                    : MakePresetPreviewValue(preset->replacementText);
                CString line; line.Format(FbeLoadRuntimeStringByKey(L"fbe.search_preset.preview.replace", L"Replace: %s"), static_cast<LPCWSTR>(replacement));
                description += L"\r\n" + line;
            }
        }
        ::SetWindowText(GetDlgItem(IDC_FIND_PRESET_DESCRIPTION), description);
    }

    void RefreshPresetPanel(const CString& wantedId = CString(), bool selectUserRoot = false)
    {
        const HWND tree = GetDlgItem(IDC_FIND_PRESETS_TREE);
        if (!tree) return;
        m_panelPresets.clear();
        std::vector<FbeSearchPresets::SearchPreset> builtIns;
        FbeSearchPresets::GetBuiltInPresets(SearchContext(), IsReplaceDialog(), builtIns);
        std::vector<FbeSearchPresets::SearchPreset> users;
        FbeSearchPresets::SearchPresetStore store;
        store.Load(users); // a damaged file never hides the built-in catalog
        for (size_t index = 0; index < users.size(); ++index)
            if (users[index].context == SearchContext() && (!IsReplaceDialog() || users[index].hasReplacement))
                m_panelPresets.push_back(users[index]);

        ::SendMessage(tree, TVM_DELETEITEM, 0, reinterpret_cast<LPARAM>(TVI_ROOT));
        const HTREEITEM builtInRoot = InsertPresetTreeItem(tree, TVI_ROOT,
            FbeLoadRuntimeStringByKey(L"fbe.search_preset.built_in", L"Built-in"), -1);
        for (size_t index = 0; index < builtIns.size(); ++index)
        {
            m_panelPresets.insert(m_panelPresets.begin() + index, builtIns[index]);
            InsertPresetTreeItem(tree, builtInRoot, builtIns[index].name, static_cast<LPARAM>(index));
        }
        const size_t builtInCount = builtIns.size();
        const HTREEITEM userRoot = InsertPresetTreeItem(tree, TVI_ROOT,
            FbeLoadRuntimeStringByKey(L"fbe.search_preset.user", L"User"), -2);
        HTREEITEM desired = NULL;
        for (size_t index = builtInCount; index < m_panelPresets.size(); ++index) {
            HTREEITEM item = InsertPresetTreeItem(tree, userRoot, m_panelPresets[index].name, static_cast<LPARAM>(index));
            if(!wantedId.IsEmpty() && m_panelPresets[index].id == wantedId) desired = item;
        }
        TreeView_Expand(tree, builtInRoot, TVE_EXPAND);
        TreeView_Expand(tree, userRoot, TVE_EXPAND);
        if(!desired && selectUserRoot) desired = userRoot;
        if(desired) { TreeView_SelectItem(tree, desired); TreeView_EnsureVisible(tree, desired); }
        UpdatePresetActions();
    }

    int PresetPanelWidth() const
    {
        RECT units = { 0, 0, 212, 0 };
        const HWND dialog = DialogWindow();
        if (dialog) ::MapDialogRect(dialog, &units);
        return units.right;
    }

    int PresetPanelHeight() const
    {
        RECT units = { 0, 0, 0, 72 };
        const HWND dialog = DialogWindow();
        if (dialog) ::MapDialogRect(dialog, &units);
        return units.bottom;
    }

    void UpdatePresetToggleCaption()
    {
        SetRuntimeText(IDC_FIND_TEMPLATES,
            m_templatesExpanded ? L"fbe.search_preset.collapse" : L"fbe.search_preset.expand",
            m_templatesExpanded ? L"Templates <" : L"Templates >");
    }

    void SetPresetPanelVisible(bool visible)
    {
        const int controls[] = { IDC_FIND_PRESETS_LABEL, IDC_FIND_PRESETS_TREE, IDC_FIND_PRESET_DESCRIPTION,
            IDC_FIND_PRESET_APPLY, IDC_FIND_PRESET_SAVE, IDC_FIND_PRESET_UPDATE, IDC_FIND_PRESET_RENAME, IDC_FIND_PRESET_DELETE };
        for (size_t index = 0; index < _countof(controls); ++index)
            ::ShowWindow(GetDlgItem(controls[index]), visible ? SW_SHOW : SW_HIDE);
        const HWND dialog = DialogWindow();
        if (!dialog) return;
        if (m_compactDialogWidth == 0)
        {
            RECT rectangle = {}; ::GetWindowRect(dialog, &rectangle);
            m_compactDialogWidth = rectangle.right - rectangle.left;
            m_compactDialogHeight = rectangle.bottom - rectangle.top;
        }
        RECT rectangle = {};
        ::GetWindowRect(dialog, &rectangle);
        const int width = visible ? m_compactDialogWidth + PresetPanelWidth() : m_compactDialogWidth;
        const int height = visible ? m_compactDialogHeight + PresetPanelHeight() : m_compactDialogHeight;
        int left = rectangle.left;
        int top = rectangle.top;
        if (visible)
        {
            const HMONITOR monitor = ::MonitorFromWindow(dialog, MONITOR_DEFAULTTONEAREST);
            MONITORINFO monitorInfo = {}; monitorInfo.cbSize = sizeof(monitorInfo);
            if (monitor && ::GetMonitorInfo(monitor, &monitorInfo))
            {
                left = max(monitorInfo.rcWork.left, min(left, monitorInfo.rcWork.right - width));
                top = max(monitorInfo.rcWork.top, min(top, monitorInfo.rcWork.bottom - height));
            }
        }
        ::SetWindowPos(dialog, NULL, left, top, width, height, SWP_NOZORDER | SWP_NOACTIVATE);
        m_templatesExpanded = visible;
        UpdatePresetToggleCaption();
        if (visible) RefreshPresetPanel();
    }

    FbeSearchPresets::SearchPreset CurrentPreset(const CString& name) const
    {
        FbeSearchPresets::SearchPreset preset;
        preset.name = name;
        GUID guid = {}; ::CoCreateGuid(&guid);
        wchar_t id[40] = {}; ::StringFromGUID2(guid, id, _countof(id));
        preset.id = id;
        preset.id.Trim(L"{}");
        preset.findText = m_view->m_fo.pattern;
        preset.hasReplacement = IsReplaceDialog();
        preset.replacementText = preset.hasReplacement ? m_view->m_fo.replacement : CString();
        preset.regexp = m_view->m_fo.fRegexp;
        preset.matchCase = (m_view->m_fo.flags & CFBEView::FRF_CASE) != 0;
        preset.wholeWord = (m_view->m_fo.flags & CFBEView::FRF_WHOLE) != 0;
        preset.unicodeProperties = SearchContext() == FbeSearchPresets::SearchUiContext::Design && m_view->m_fo.unicodeProperties;
        preset.context = SearchContext();
        return preset;
    }

    bool SaveUserPresets(const std::vector<FbeSearchPresets::SearchPreset>& presets)
    {
        FbeSearchPresets::SearchPresetStore store;
        if (store.Save(presets)) return true;
        ThemeManager::MessageBox(DialogWindow(),
            FbeLoadRuntimeStringByKey(L"fbe.search_preset.save_failed", L"Could not save search templates."),
            FbeLoadRuntimeStringByKey(L"fbe.search_preset.caption", L"Templates"), MB_OK | MB_ICONEXCLAMATION);
        return false;
    }

    bool LoadUserPresetsForMutation(std::vector<FbeSearchPresets::SearchPreset>& presets)
    {
        FbeSearchPresets::SearchPresetStore store;
        if (store.Load(presets)) return true;
        ThemeManager::MessageBox(DialogWindow(),
            FbeLoadRuntimeStringByKey(L"fbe.search_preset.load_failed", L"Search templates could not be loaded. The file was not changed."),
            FbeLoadRuntimeStringByKey(L"fbe.search_preset.caption", L"Templates"), MB_OK | MB_ICONEXCLAMATION);
        return false;
    }

    FbeSearchPresets::SearchPreset BuildUpdatedPreset(const FbeSearchPresets::SearchPreset& existing) const
    {
        FbeSearchPresets::SearchPreset preset = CurrentPreset(existing.name);
        preset.id = existing.id;
        if (!IsReplaceDialog())
        {
            preset.hasReplacement = existing.hasReplacement;
            preset.replacementText = existing.replacementText;
        }
        return preset;
    }

    void ApplySelectedPreset()
    {
        const FbeSearchPresets::SearchPreset* preset = SelectedPreset();
        if (!preset) return;
        m_view->m_fo.pattern = preset->findText;
        if (IsReplaceDialog() && preset->hasReplacement) m_view->m_fo.replacement = preset->replacementText;
        m_view->m_fo.fRegexp = preset->regexp;
        m_view->m_fo.flags = (m_view->m_fo.flags & CFBEView::FRF_REVERSE) |
            (preset->matchCase ? CFBEView::FRF_CASE : 0) | (preset->wholeWord ? CFBEView::FRF_WHOLE : 0);
        if (SearchContext() == FbeSearchPresets::SearchUiContext::Design)
            m_view->m_fo.unicodeProperties = preset->unicodeProperties;
        InvalidateSearchSelectionState();
        m_view->m_startMatch = m_view->m_endMatch = 0;
        m_view->m_fo.ClearMatch();
        m_view->m_design_search.ClearReplacePreview();
        PutData();
        UpdateUnicodeControl();
        UpdatePresetActions();
        m_view->SyncSearchOptionsToOpenDialogs(this);
        if (GetDlgItem(IDC_FIND_STATUS)) ::SetTimer(DialogWindow(), 0x4F01, 150, NULL);
        ::SetFocus(GetDlgItem(IDC_TEXT));
    }

    LRESULT OnTogglePresets(WORD, WORD, HWND, BOOL&) { SetPresetPanelVisible(!m_templatesExpanded); return 0; }
    LRESULT OnApplyPreset(WORD, WORD, HWND, BOOL&) { ApplySelectedPreset(); return 0; }
    LRESULT OnSavePreset(WORD, WORD, HWND, BOOL&)
    {
        GetData();
        if (m_view->m_fo.pattern.IsEmpty()) return 0;
        CString name;
        if (AU::InputBox(name, FbeLoadRuntimeStringByKey(L"fbe.search_preset.save_title", L"Save search template"),
            FbeLoadRuntimeStringByKey(L"fbe.search_preset.name_prompt", L"Template name:")) != IDYES || name.Trim().IsEmpty()) return 0;
        std::vector<FbeSearchPresets::SearchPreset> users;
        if (!LoadUserPresetsForMutation(users)) return 0;
        users.push_back(CurrentPreset(name));
        if (SaveUserPresets(users)) {
            RefreshPresetPanel(users.back().id);
            NotifyOpenPresetPanels();
        }
        return 0;
    }
    LRESULT OnUpdatePreset(WORD, WORD, HWND, BOOL&)
    {
        const FbeSearchPresets::SearchPreset* selected = SelectedPreset();
        if (!selected || selected->builtIn) return 0;
        const CString selectedId = selected->id;
        GetData();
        std::vector<FbeSearchPresets::SearchPreset> users;
        if (!LoadUserPresetsForMutation(users)) return 0;
        for (size_t index = 0; index < users.size(); ++index)
            if (users[index].id == selectedId)
            {
                users[index] = BuildUpdatedPreset(users[index]);
                break;
            }
        if (SaveUserPresets(users)) { RefreshPresetPanel(selectedId); NotifyOpenPresetPanels(); }
        return 0;
    }
    LRESULT OnRenamePreset(WORD, WORD, HWND, BOOL&)
    {
        const FbeSearchPresets::SearchPreset* selected = SelectedPreset();
        if (!selected || selected->builtIn) return 0;
        const CString selectedId = selected->id;
        CString name(selected->name);
        if (AU::InputBox(name, FbeLoadRuntimeStringByKey(L"fbe.search_preset.rename_title", L"Rename search template"),
            FbeLoadRuntimeStringByKey(L"fbe.search_preset.name_prompt", L"Template name:")) != IDYES || name.Trim().IsEmpty()) return 0;
        std::vector<FbeSearchPresets::SearchPreset> users;
        if (!LoadUserPresetsForMutation(users)) return 0;
        for (size_t index = 0; index < users.size(); ++index)
            if (users[index].id == selectedId)
                users[index].name = name;
        if (SaveUserPresets(users)) { RefreshPresetPanel(selectedId); NotifyOpenPresetPanels(); }
        return 0;
    }
    LRESULT OnDeletePreset(WORD, WORD, HWND, BOOL&)
    {
        const FbeSearchPresets::SearchPreset* selected = SelectedPreset();
        if (!selected || selected->builtIn) return 0;
        const CString selectedId = selected->id;
        if (ThemeManager::MessageBox(DialogWindow(), FbeLoadRuntimeStringByKey(L"fbe.search_preset.delete_confirm", L"Delete the selected search template?"), FbeLoadRuntimeStringByKey(L"fbe.search_preset.caption", L"Templates"), MB_YESNO | MB_ICONQUESTION) != IDYES) return 0;
        std::vector<FbeSearchPresets::SearchPreset> users;
        if (!LoadUserPresetsForMutation(users)) return 0;
        CString nextId;
        for (std::vector<FbeSearchPresets::SearchPreset>::iterator item = users.begin(); item != users.end(); ++item)
            if (item->id == selectedId)
            {
                if(item + 1 != users.end()) nextId = (item + 1)->id;
                else if(item != users.begin()) nextId = (item - 1)->id;
                users.erase(item);
                break;
            }
        if (SaveUserPresets(users)) { RefreshPresetPanel(nextId, nextId.IsEmpty()); NotifyOpenPresetPanels(); }
        return 0;
    }
    LRESULT OnPresetChanged(int, LPNMHDR, BOOL&) { UpdatePresetActions(); return 0; }
    LRESULT OnPresetDblClick(int, LPNMHDR, BOOL&) { ApplySelectedPreset(); return 0; }
    LRESULT OnPresetKeyDown(int, LPNMHDR header, BOOL&)
    {
        NMTVKEYDOWN* key = reinterpret_cast<NMTVKEYDOWN*>(header); if (key && key->wVKey == VK_RETURN) ApplySelectedPreset(); return 0;
    }
    LRESULT OnShowRegexHelp(WORD, WORD, HWND, BOOL&)
    {
        HWND target = GetDlgItem(IDC_TEXT);
        FbeSearchPresets::RegexQuickReferenceMode mode = FbeSearchPresets::RegexQuickReferenceMode::Search;
        if(IsReplaceDialog() && (m_lastRegexTarget == IDC_REPLACE || ::GetFocus() == GetDlgItem(IDC_REPLACE))) { target = GetDlgItem(IDC_REPLACE); mode = FbeSearchPresets::RegexQuickReferenceMode::Replacement; }
        RegexQuickReferencePopup* popup = new RegexQuickReferencePopup();
        // Show(false) means no HWND was created; on success the popup self-owns until WM_NCDESTROY.
        if(!popup->Show(DialogWindow(), GetDlgItem(IDC_FIND_REGEX_HELP), SearchContext(), mode,
            [this, target, mode](const FbeSearchPresets::RegexQuickReferenceEntry& entry) {
                int first = 0, last = 0; ::SendMessage(target, EM_GETSEL, reinterpret_cast<WPARAM>(&first), reinterpret_cast<LPARAM>(&last)); const int length = ::GetWindowTextLength(target); CString current; ::GetWindowText(target, current.GetBuffer(length + 1), length + 1); current.ReleaseBuffer();
                const FbeSearchPresets::RegexQuickReferenceInsertion result = FbeSearchPresets::InsertRegexQuickReference(current, first, last, entry);
                ::SetWindowText(target, result.text); ::SendMessage(target, EM_SETSEL, result.selectionStart, result.selectionStart + result.selectionLength); ::SetFocus(target);
                if(!m_view->m_fo.fRegexp) { m_view->m_fo.fRegexp = true; ::CheckDlgButton(DialogWindow(), IDC_REGEXP, BST_CHECKED); UpdateUnicodeControl(); m_view->SyncSearchOptionsToOpenDialogs(this); }
                if(mode == FbeSearchPresets::RegexQuickReferenceMode::Search) m_view->m_fo.pattern = result.text; else m_view->m_fo.replacement = result.text;
                InvalidateSearchSelectionState();
            },
            [this]() { ShowRegexHelpDialog(DialogWindow(), SearchContext()); })) delete popup;
        return 0;
    }
	BEGIN_MSG_MAP(FRBase)
		ALT_MSG_MAP(1)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroyPresetPanel)
		COMMAND_HANDLER(IDC_TEXT,CBN_EDITCHANGE, OnTextChanged)        COMMAND_ID_HANDLER(IDC_FIND_TEMPLATES, OnTogglePresets)
        COMMAND_HANDLER(IDC_TEXT, CBN_SETFOCUS, OnRegexFieldFocus)
        COMMAND_HANDLER(IDC_REPLACE, CBN_SETFOCUS, OnRegexFieldFocus)
        COMMAND_ID_HANDLER(IDC_FIND_REGEX_HELP, OnShowRegexHelp)
        COMMAND_ID_HANDLER(IDC_FIND_PRESET_APPLY, OnApplyPreset)
        COMMAND_ID_HANDLER(IDC_FIND_PRESET_SAVE, OnSavePreset)
        COMMAND_ID_HANDLER(IDC_FIND_PRESET_UPDATE, OnUpdatePreset)
        COMMAND_ID_HANDLER(IDC_FIND_PRESET_RENAME, OnRenamePreset)
        COMMAND_ID_HANDLER(IDC_FIND_PRESET_DELETE, OnDeletePreset)
        NOTIFY_HANDLER(IDC_FIND_PRESETS_TREE, TVN_SELCHANGED, OnPresetChanged)
        NOTIFY_HANDLER(IDC_FIND_PRESETS_TREE, NM_DBLCLK, OnPresetDblClick)
        NOTIFY_HANDLER(IDC_FIND_PRESETS_TREE, TVN_KEYDOWN, OnPresetKeyDown)
	END_MSG_MAP()

  BEGIN_DDX_MAP(FRBase)
    DDX_TEXT(IDC_TEXT, m_view->m_fo.pattern)
    if (GetDlgItem(IDC_REPLACE))
      DDX_TEXT(IDC_REPLACE, m_view->m_fo.replacement);
    DDX_CHECK(IDC_WHOLE, m_whole)
    DDX_CHECK(IDC_MATCHCASE, m_case)
    DDX_CHECK(IDC_REGEXP, m_regexp)
	if (GetDlgItem(IDC_FIND_UNICODE_PROPERTIES))
		DDX_CHECK(IDC_FIND_UNICODE_PROPERTIES, m_unicode)
    if (GetDlgItem(IDC_UP))
      DDX_RADIO(IDC_UP, m_dir);
  END_DDX_MAP()

	void GetData()
	{
		m_text.SetSelNone();

		DoDataExchange(TRUE);

		int flags = 0;
		if(m_case)
			flags |= CFBEView::FRF_CASE;
		if(m_whole)
			flags |= CFBEView::FRF_WHOLE;
		if(m_dir == 0)
			flags |= CFBEView::FRF_REVERSE;

		m_view->m_fo.flags = flags;
		m_view->m_fo.fRegexp = m_regexp != 0;
		if (SearchContext() == FbeSearchPresets::SearchUiContext::Design)
		    m_view->m_fo.unicodeProperties = m_unicode != 0;
		HWND scope = FRBase::GetDlgItem(IDC_FIND_SCOPE);
		if (scope)
		{
			const LRESULT selection = ::SendMessage(scope, CB_GETCURSEL, 0, 0);
			if (selection != CB_ERR)
				m_view->m_fo.scope = static_cast<AU::Search::SearchScope>(::SendMessage(scope, CB_GETITEMDATA, selection, 0));
		}
	}

	void PutData()
	{
		m_case = (m_view->m_fo.flags & CFBEView::FRF_CASE) != 0;
		m_whole = (m_view->m_fo.flags & CFBEView::FRF_WHOLE) != 0;
		m_dir = (m_view->m_fo.flags & CFBEView::FRF_REVERSE) == 0;
		m_regexp = m_view->m_fo.fRegexp;
		m_unicode = SearchContext() == FbeSearchPresets::SearchUiContext::Design && m_view->m_fo.unicodeProperties;
		m_scope = static_cast<int>(m_view->m_fo.scope);
		DoDataExchange(FALSE);
	}

	void PopulateFindScopes()
	{
		HWND scope = GetDlgItem(IDC_FIND_SCOPE);
		if (!scope)
			return;
		::SendMessage(scope, CB_RESETCONTENT, 0, 0);
		const CString wholeDocument = FbeLoadRuntimeStringByKey(L"fbe.dialog.idd_find.scope_whole_document", L"Whole document");
		const CString currentSection = FbeLoadRuntimeStringByKey(L"fbe.dialog.idd_find.scope_current_section", L"Current section");
		const struct { LPCWSTR Text; AU::Search::SearchScope Value; } values[] = {
			{ wholeDocument, AU::Search::SearchScope::WholeDocument },
			{ currentSection, AU::Search::SearchScope::CurrentSection }
		};
		for (int index = 0; index != _countof(values); ++index)
		{
			const LRESULT item = ::SendMessage(scope, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(values[index].Text));
			::SendMessage(scope, CB_SETITEMDATA, item, static_cast<LPARAM>(values[index].Value));
		}
		// A completed Selection search owns a stable source range. Find Next moves
		// MSHTML's visual selection to a hit, so keep this entry while that source
		// range remains valid; otherwise refresh it from the live selection.
		if (m_view->HasTextSelection() ||
			(m_scope == static_cast<int>(AU::Search::SearchScope::Selection) && m_view->HasSavedSearchScope()))
		{
			const CString selectionText = FbeLoadRuntimeStringByKey(L"fbe.dialog.idd_find.scope_selection", L"Selection");
			const LRESULT item = ::SendMessage(scope, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(static_cast<LPCWSTR>(selectionText)));
			::SendMessage(scope, CB_SETITEMDATA, item, static_cast<LPARAM>(AU::Search::SearchScope::Selection));
		}
		for (LRESULT index = 0, count = ::SendMessage(scope, CB_GETCOUNT, 0, 0); index < count; ++index)
			if (static_cast<int>(::SendMessage(scope, CB_GETITEMDATA, index, 0)) == m_scope)
			{
				::SendMessage(scope, CB_SETCURSEL, index, 0);
				return;
			}
		::SendMessage(scope, CB_SETCURSEL, 0, 0);
	}

  void	LoadHistoryImp(const TCHAR *path,CRegKey& rk,HWND hCB,CString& first) {
    if (!hCB)
      return;

    // open history key
    if (rk.Create(_Settings.GetKey(), path)!=ERROR_SUCCESS)
      return;
    
    // get number of entries
    DWORD nfs;
    if (rk.QueryDWORDValue(_T(""),nfs)!=ERROR_SUCCESS)
      return;
    
    // fetch the entries
    //first.Empty();

    CString   ps,str;

    for (DWORD i=0;i<nfs;++i) 
	{
      ps.Format(_T("%d"), static_cast<int>(i));
      str=U::QuerySV(rk,ps);
      if (!str.IsEmpty()) 
	  {
		::SendMessage(hCB,CB_ADDSTRING,0,(LPARAM)(const TCHAR *)str);
		if (first.IsEmpty())
			first=str;
      }
    }
  }

	LRESULT OnInitDialog(UINT, WPARAM, LPARAM, BOOL& bHandled)
	{
		bHandled = FALSE;
		if(std::find(OpenPresetPanels().begin(), OpenPresetPanels().end(), this) == OpenPresetPanels().end()) OpenPresetPanels().push_back(this);

		LoadHistoryImp(_T("SearchHistory"), m_fh, GetDlgItem(IDC_TEXT), m_view->m_fo.pattern);
		LoadHistoryImp(_T("ReplaceHistory"), m_rh, GetDlgItem(IDC_REPLACE), m_view->m_fo.replacement);

		const bool isReplaceDialog = GetDlgItem(IDC_REPLACE) != NULL;
		SetRuntimeDialogTitle(isReplaceDialog ? L"fbe.dialog.idd_replace.caption" : L"fbe.dialog.idd_find.caption", isReplaceDialog ? L"Replace" : L"Find");
		SetRuntimeText(isReplaceDialog ? IDC_REPLACE_LABEL_TEXT : IDC_FIND_LABEL_TEXT,
			isReplaceDialog ? L"fbe.dialog.idd_replace.find_what" : L"fbe.dialog.idd_find.find_what",
			isReplaceDialog ? L"Find:" : L"Find what:");
		SetRuntimeText(ID_FIND_NEXT, isReplaceDialog ? L"fbe.dialog.idd_replace.find_next" : L"fbe.dialog.idd_find.find_next", L"&Find Next");
		if (!isReplaceDialog)
			SetRuntimeText(IDC_FIND_ALL, L"fbe.dialog.idd_find.find_all", L"Find &All");
		SetRuntimeText(IDC_WHOLE, isReplaceDialog ? L"fbe.dialog.idd_replace.whole_word" : L"fbe.dialog.idd_find.whole_word", L"Match &whole words");
		SetRuntimeText(IDC_MATCHCASE, isReplaceDialog ? L"fbe.dialog.idd_replace.match_case" : L"fbe.dialog.idd_find.match_case", L"Match &case");
		SetRuntimeText(IDC_REGEXP, isReplaceDialog ? L"fbe.dialog.idd_replace.regexp" : L"fbe.dialog.idd_find.regexp", L"Regular &expression");
		SetRuntimeText(IDC_FIND_UNICODE_PROPERTIES,
			L"fbe.dialog.idd_find.unicode_properties",
			L"Unicode (&UCP)");
		SetRuntimeText(isReplaceDialog ? IDC_REPLACE_DIRECTION_GROUP : IDC_FIND_DIRECTION_GROUP,
			isReplaceDialog ? L"fbe.dialog.idd_replace.direction" : L"fbe.dialog.idd_find.direction",
			L"Direction");
		SetRuntimeText(IDC_UP, isReplaceDialog ? L"fbe.dialog.idd_replace.up" : L"fbe.dialog.idd_find.up", L"&Up");
		SetRuntimeText(IDC_DOWN, isReplaceDialog ? L"fbe.dialog.idd_replace.down" : L"fbe.dialog.idd_find.down", L"&Down");
		SetRuntimeText(IDC_FIND_FROM_START,
			L"fbe.dialog.idd_find.from_start",
			L"&From start");
		SetRuntimeText(IDCANCEL, isReplaceDialog ? L"fbe.dialog.idd_replace.cancel" : L"fbe.dialog.idd_find.cancel", L"Cancel");
		if(isReplaceDialog)
		{
			SetRuntimeText(IDC_REPLACE_LABEL_REPLACE, L"fbe.dialog.idd_replace.replace_with", L"Replace:");
			SetRuntimeText(IDC_REPLACE_ONE, L"fbe.dialog.idd_replace.replace_one", L"&Replace");
			SetRuntimeText(IDC_REPLACE_ALL, L"fbe.dialog.idd_replace.replace_all", L"Replace &All");
		}

		// One open dialog is the in-memory source of truth for the other one.
		// Otherwise opening Replace while Find is modeless would reload registry
		// defaults and visibly discard its unsaved common options.
		const bool otherDialogOpen = isReplaceDialog
			? m_view->IsFindDialogOpen()
			: m_view->IsReplaceDialogOpen();
		if (!otherDialogOpen)
		{
			DWORD flags = _Settings.GetSearchOptions();
			m_view->m_fo.fRegexp = (flags & CFBEView::FRF_REGEX) != 0;
			m_view->m_fo.unicodeProperties = (flags & CFBEView::FRF_UNICODE_PROPERTIES) != 0;
			m_view->m_fo.flags = flags & ~(CFBEView::FRF_REGEX | CFBEView::FRF_UNICODE_PROPERTIES);
		}

		m_view->m_startMatch = m_view->m_endMatch = 0;

		m_text = GetDlgItem(IDC_TEXT);

		// Set fields
		PutData();
		UpdateUnicodeControl();        SetPresetPanelVisible(false);
        SetRuntimeText(IDC_FIND_REGEX_HELP, L"fbe.search_preset.regex_help", L"?");
        SetRuntimeText(IDC_FIND_PRESETS_LABEL, L"fbe.search_preset.caption", L"Templates");
        SetRuntimeText(IDC_FIND_PRESET_APPLY, L"fbe.search_preset.apply", L"Apply");
        SetRuntimeText(IDC_FIND_PRESET_SAVE, L"fbe.search_preset.save_current", L"Save current...");
        SetRuntimeText(IDC_FIND_PRESET_UPDATE, L"fbe.search_preset.update", L"Update");
        SetRuntimeText(IDC_FIND_PRESET_RENAME, L"fbe.search_preset.rename", L"Rename...");
        SetRuntimeText(IDC_FIND_PRESET_DELETE, L"fbe.search_preset.delete", L"Delete");
		if (GetDlgItem(IDC_FIND_SCOPE) != NULL)
		{
			SetRuntimeText(IDC_FIND_SCOPE_LABEL,
				L"fbe.dialog.idd_find.scope", L"Scope:");
			PopulateFindScopes();
			const HWND dialog = ::GetParent(GetDlgItem(IDC_TEXT));
			if (dialog)
			{
				m_tooltips.Initialize(dialog);
				m_tooltips.Add(GetDlgItem(IDC_TEXT), L"fbe.tooltip.find.text", L"Text to find. Results update after a short pause while typing.");
				m_tooltips.Add(GetDlgItem(ID_FIND_NEXT), L"fbe.tooltip.find.next", L"Select the next match in the chosen direction.");
				if (!isReplaceDialog) m_tooltips.Add(GetDlgItem(IDC_FIND_ALL), L"fbe.tooltip.find.all", L"Show every match in the Results pane.");
				m_tooltips.Add(GetDlgItem(IDC_WHOLE), L"fbe.tooltip.find.whole_word", L"Match complete words only.");
				m_tooltips.Add(GetDlgItem(IDC_MATCHCASE), L"fbe.tooltip.find.match_case", L"Distinguish uppercase and lowercase letters.");
				m_tooltips.Add(GetDlgItem(IDC_REGEXP), L"fbe.tooltip.find.regexp", L"Interpret the query as a regular expression.");
                m_tooltips.Add(GetDlgItem(IDC_FIND_TEMPLATES), L"fbe.tooltip.find.templates", L"Open built-in and saved search templates.");
                const bool designContext = SearchContext() == FbeSearchPresets::SearchUiContext::Design;
                m_tooltips.Add(GetDlgItem(IDC_FIND_REGEX_HELP),
                    designContext ? L"fbe.tooltip.find.regex_help_design" : L"fbe.tooltip.find.regex_help_source",
                    designContext ? L"PCRE2 regular-expression help" : L"Scintilla regular-expression help");
				m_tooltips.Add(GetDlgItem(IDC_FIND_SCOPE), L"fbe.tooltip.find.scope", L"Choose where to search.");
                const LPCWSTR ucpKey = designContext ? L"fbe.tooltip.find.unicode_properties" : L"fbe.tooltip.find.unicode_properties_source";
                const LPCWSTR ucpFallback = designContext
                    ? L"Use Unicode properties for \\w, \\d, \\s and word boundaries \\b/\\B (for example with Cyrillic text). Available only when Regular expression is enabled."
                    : L"Unicode UCP applies to PCRE2 in Design mode and is unavailable for the current Source regex engine.";
                m_tooltips.Add(GetDlgItem(IDC_FIND_UNICODE_PROPERTIES), ucpKey, ucpFallback);
                m_tooltips.AddDisabledControlArea(GetDlgItem(IDC_FIND_UNICODE_PROPERTIES), ucpKey, ucpFallback);
				if (!isReplaceDialog) m_tooltips.Add(GetDlgItem(IDC_FIND_STATUS), L"fbe.tooltip.find.status", L"Search status and complete regular-expression diagnostic.");
				m_tooltips.Add(GetDlgItem(IDC_UP), L"fbe.tooltip.find.up", L"Search toward the beginning of the document.");
				m_tooltips.Add(GetDlgItem(IDC_DOWN), L"fbe.tooltip.find.down", L"Search toward the end of the document.");
				m_tooltips.Add(GetDlgItem(IDC_FIND_FROM_START), L"fbe.tooltip.find.from_start", L"Go to the first match in the selected scope.");
			}
		}

		return 0;
	}

	LRESULT OnTextChanged(WORD, WORD /* unused: wID */, HWND, BOOL&)
	{
		CheckInput();
		UpdatePresetActions();
		// Find All is debounced so editing a query never synchronously invokes
		// PCRE2 on every keystroke. Replace keeps its existing explicit flow.
		if (GetDlgItem(IDC_FIND_STATUS))
			::SetTimer(::GetParent(GetDlgItem(IDC_TEXT)), 0x4F01, 150, NULL);
		return 0;
	}

    LRESULT OnRegexFieldFocus(WORD, WORD id, HWND, BOOL&) { m_lastRegexTarget = id; return 0; }

  void	CheckInput() {
    ::EnableWindow(GetDlgItem(IDOK),::GetWindowTextLength(GetDlgItem(IDC_TEXT))>0);
  }

	void SaveStringImp(HWND hCB)
	{
		if(!hCB)
			return;

		CString cur = U::GetWindowText(hCB);

		if(cur.IsEmpty())
			return;

		LRESULT Idx = ::SendMessage(hCB, CB_FINDSTRINGEXACT, static_cast<WPARAM>(-1), (LPARAM)(const TCHAR*)cur);
		if(Idx == 0)
			return;
		if(Idx != CB_ERR)
			::SendMessage(hCB, CB_DELETESTRING, Idx, 0);

		::SendMessage(hCB, CB_INSERTSTRING, 0,(LPARAM)(const TCHAR*)cur);
		// fix for issue #136
		::SendMessage(hCB, CB_SETCURSEL, 0, 0);
	}

	void SaveString()
	{
		SaveStringImp(GetDlgItem(IDC_TEXT));
		SaveStringImp(GetDlgItem(IDC_REPLACE));
	}

	void SaveHistoryImp(CRegKey& rk,HWND hCB)
	{
		if(!rk && !hCB)
			return;

		LRESULT lCount = ::SendMessage(hCB, CB_GETCOUNT, 0, 0);
		if(lCount > 100)
			lCount = 100;

		CString path;
		for (int i = 0; i < lCount; ++i)
		{
			CString cur(U::GetCBString(hCB, i));
			if(cur.IsEmpty())
				continue;
			path.Format(L"%d", i);
			rk.SetStringValue(path, cur);
		}

		rk.SetDWORDValue(L"", lCount);
	}

	void SaveHistory() 
	{
		SaveSearchOptions();
		SaveHistoryImp(m_fh,GetDlgItem(IDC_TEXT));
		SaveHistoryImp(m_rh,GetDlgItem(IDC_REPLACE));
	}

	void SaveSearchOptions()
	{
		_Settings.SetSearchOptions(m_view->m_fo.flags |
			(m_view->m_fo.fRegexp ? CFBEView::FRF_REGEX : 0) |
			(m_view->m_fo.unicodeProperties ? CFBEView::FRF_UNICODE_PROPERTIES : 0), true);
	}

	void UpdateUnicodeControl()
	{
		HWND unicode = GetDlgItem(IDC_FIND_UNICODE_PROPERTIES);
		if (unicode)
			::EnableWindow(unicode, SearchContext() == FbeSearchPresets::SearchUiContext::Design && ::IsDlgButtonChecked(::GetParent(unicode), IDC_REGEXP) == BST_CHECKED);
	}

	// Find and Replace are modeless views of the same CFBEView options.  Keep
	// their controls in lockstep as soon as a common option changes, rather
	// than waiting for a search command or either dialog to close.
	void SyncSearchOptionsFromView()
	{
		m_case = (m_view->m_fo.flags & CFBEView::FRF_CASE) != 0;
		m_whole = (m_view->m_fo.flags & CFBEView::FRF_WHOLE) != 0;
		m_dir = (m_view->m_fo.flags & CFBEView::FRF_REVERSE) == 0;
		m_regexp = m_view->m_fo.fRegexp;
		m_unicode = SearchContext() == FbeSearchPresets::SearchUiContext::Design && m_view->m_fo.unicodeProperties;
		m_scope = static_cast<int>(m_view->m_fo.scope);
		const HWND dialog = ::GetParent(GetDlgItem(IDC_TEXT));
		if (dialog)
		{
			::CheckDlgButton(dialog, IDC_MATCHCASE, m_case ? BST_CHECKED : BST_UNCHECKED);
			::CheckDlgButton(dialog, IDC_WHOLE, m_whole ? BST_CHECKED : BST_UNCHECKED);
			::CheckDlgButton(dialog, IDC_REGEXP, m_regexp ? BST_CHECKED : BST_UNCHECKED);
			::CheckDlgButton(dialog, IDC_FIND_UNICODE_PROPERTIES, m_unicode ? BST_CHECKED : BST_UNCHECKED);
			::CheckRadioButton(dialog, IDC_UP, IDC_DOWN, m_dir ? IDC_DOWN : IDC_UP);
		}
		const HWND scope = GetDlgItem(IDC_FIND_SCOPE);
		if (scope)
			for (LRESULT index = 0, count = ::SendMessage(scope, CB_GETCOUNT, 0, 0); index < count; ++index)
				if (static_cast<int>(::SendMessage(scope, CB_GETITEMDATA, index, 0)) == m_scope)
				{
					::SendMessage(scope, CB_SETCURSEL, index, 0);
					break;
				}
		UpdateUnicodeControl();
	}
};

class CFindDlgBase: public CModelessDialogImpl<CFindDlgBase>, public FRBase
{
public:
	enum { IDD = IDD_FIND };

	CFindDlgBase(CFBEView *view) : FRBase(view){ }

	BEGIN_MSG_MAP(CFindDlgBase)
		MESSAGE_HANDLER(WM_TIMER, OnTimer)
		MESSAGE_HANDLER(WM_CLOSE, OnClose)
		COMMAND_ID_HANDLER(ID_FIND_NEXT, OnDoFind)
		COMMAND_HANDLER(IDC_FIND_FROM_START, BN_CLICKED, OnFindFromStart)
		COMMAND_ID_HANDLER(IDC_FIND_ALL, OnDoFindAll)
		COMMAND_HANDLER(IDC_FIND_SCOPE, CBN_SELCHANGE, OnScopeChanged)
		COMMAND_HANDLER(IDC_FIND_SCOPE, CBN_DROPDOWN, OnScopeDropDown)
		COMMAND_HANDLER(IDC_MATCHCASE, BN_CLICKED, OnSearchOptionChanged)
		COMMAND_HANDLER(IDC_WHOLE, BN_CLICKED, OnSearchOptionChanged)
		COMMAND_HANDLER(IDC_REGEXP, BN_CLICKED, OnSearchOptionChanged)
		COMMAND_HANDLER(IDC_FIND_UNICODE_PROPERTIES, BN_CLICKED, OnSearchOptionChanged)
		COMMAND_HANDLER(IDC_UP, BN_CLICKED, OnSearchOptionChanged)
		COMMAND_HANDLER(IDC_DOWN, BN_CLICKED, OnSearchOptionChanged)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
		CHAIN_MSG_MAP_ALT(FRBase, 1)
	END_MSG_MAP()


	LRESULT OnCancel(WORD, WORD /* unused: wID */, HWND, BOOL&)
	{
		::KillTimer(m_hWnd, 0x4F01);
		GetData();
		SaveSearchOptions();
		m_view->CloseFindDialog(this);
		return 0;
	}

	LRESULT OnClose(UINT, WPARAM, LPARAM, BOOL&)
	{
		::KillTimer(m_hWnd, 0x4F01);
		GetData();
		SaveSearchOptions();
		m_view->CloseFindDialog(this);
		return 0;
	}

	LRESULT OnDoFind(WORD, WORD, HWND, BOOL&)
	{
		DoFind();
		return 0;
	}

	LRESULT OnFindFromStart(WORD, WORD, HWND, BOOL&)
	{
		GetData();
		VBErr = false;
		if (!m_view->DoSearchFromScopeStart())
		{
			if (!VBErr && !m_view->LastSearchError().IsEmpty())
				SetFindStatus(m_view->LastSearchError());
			else if (!VBErr)
				U::MessageBox(MB_OK | MB_ICONEXCLAMATION, IDR_MAINFRAME, IDS_SEARCH_FAIL_MSG, static_cast<LPCWSTR>(m_view->m_fo.pattern));
		}
		else
		{
			SaveString();
			SaveHistory();
			SetFindStatus(m_view->SearchResultStatus());
		}
		return 0;
	}

	LRESULT OnDoFindAll(WORD, WORD, HWND, BOOL&)
	{
		GetData();
		VBErr = false;
		CString error;
		if (m_view->DoFindAll(true, &error))
		{
			SaveString();
			SaveHistory();
			SetFindStatus(m_view->FindAllResultStatus());
		}
		else
		{
			// Find All is explicit but stays modeless: keep the PCRE2 diagnostic in
			// the wide status row instead of leaving a previous result count visible.
			SetFindStatus(error.IsEmpty()
				? FbeLoadRuntimeStringByKey(L"fbe.search.error.mapping_failed", L"Search could not be mapped to the document.")
				: error);
		}
		return 0;
	}

	LRESULT OnScopeChanged(WORD, WORD, HWND, BOOL&)
	{
		GetData();
		m_view->ResetSearchScope();
		m_view->SyncSearchOptionsToOpenDialogs(this);
		::SetTimer(m_hWnd, 0x4F01, 150, NULL);
		return 0;
	}

	LRESULT OnScopeDropDown(WORD, WORD, HWND, BOOL&)
	{
		HWND scope = FRBase::GetDlgItem(IDC_FIND_SCOPE);
		const LRESULT selected = scope ? ::SendMessage(scope, CB_GETCURSEL, 0, 0) : CB_ERR;
		if (selected != CB_ERR)
			m_scope = static_cast<int>(::SendMessage(scope, CB_GETITEMDATA, selected, 0));
		PopulateFindScopes();
		return 0;
	}

	LRESULT OnSearchOptionChanged(WORD, WORD, HWND, BOOL&)
	{
		GetData();
		UpdateUnicodeControl();
		m_view->SyncSearchOptionsToOpenDialogs(this);
		::SetTimer(m_hWnd, 0x4F01, 150, NULL);
		return 0;
	}

	LRESULT OnTimer(UINT, WPARAM timerId, LPARAM, BOOL& bHandled)
	{
		bHandled = FALSE;
		if (timerId != 0x4F01)
			return 0;
		::KillTimer(m_hWnd, 0x4F01);
		GetData();
		CString error;
		if (m_view->m_fo.pattern.IsEmpty())
			SetFindStatus(L"");
		else if (m_view->DoFindAll(false, &error))
			SetFindStatus(m_view->FindAllResultStatus());
		else
			SetFindStatus(error.IsEmpty()
				? FbeLoadRuntimeStringByKey(L"fbe.search.error.mapping_failed", L"Search could not be mapped to the document.")
				: error);
		return 0;
	}

	virtual void DoFind() = 0;
	void SetFindStatus(const CString& status)
	{
		FRBase::SetDlgItemText(IDC_FIND_STATUS, status);
		const CString tooltip = status.IsEmpty()
			? FbeLoadRuntimeStringByKey(L"fbe.tooltip.find.status", L"Search status and complete regular-expression diagnostic.")
			: status;
		m_tooltips.UpdateText(FRBase::GetDlgItem(IDC_FIND_STATUS), tooltip);
	}
	virtual HWND X_GetDlgItem(int id)
	{
		return CModelessDialogImpl<CFindDlgBase>::GetDlgItem(id);
	}
};

class CReplaceDlgBase: public CModelessDialogImpl<CReplaceDlgBase>,
		       public FRBase
{
public:
  enum { IDD = IDD_REPLACE };
  bool m_selvalid; // true if last search was successful but no replacement was done

  CReplaceDlgBase(CFBEView *view) : FRBase(view), m_selvalid(false) { }

	BEGIN_MSG_MAP(CReplaceDlgBase)
		MESSAGE_HANDLER(WM_CLOSE, OnClose)
    COMMAND_ID_HANDLER(ID_FIND_NEXT, OnDoFind)
    COMMAND_ID_HANDLER(IDC_REPLACE_ONE, OnDoReplace)
    COMMAND_ID_HANDLER(IDC_REPLACE_ALL, OnDoReplaceAll)
	COMMAND_HANDLER(IDC_FIND_FROM_START, BN_CLICKED, OnFindFromStart)
	COMMAND_HANDLER(IDC_FIND_SCOPE, CBN_SELCHANGE, OnScopeChanged)
	COMMAND_HANDLER(IDC_FIND_SCOPE, CBN_DROPDOWN, OnScopeDropDown)
	COMMAND_HANDLER(IDC_MATCHCASE, BN_CLICKED, OnSearchOptionChanged)
	COMMAND_HANDLER(IDC_WHOLE, BN_CLICKED, OnSearchOptionChanged)
	COMMAND_HANDLER(IDC_REGEXP, BN_CLICKED, OnSearchOptionChanged)
	COMMAND_HANDLER(IDC_FIND_UNICODE_PROPERTIES, BN_CLICKED, OnSearchOptionChanged)
	COMMAND_HANDLER(IDC_UP, BN_CLICKED, OnSearchOptionChanged)
	COMMAND_HANDLER(IDC_DOWN, BN_CLICKED, OnSearchOptionChanged)
    COMMAND_ID_HANDLER(IDCANCEL, OnCancel)

    COMMAND_HANDLER(IDC_TEXT,CBN_EDITCHANGE, OnTextChanged)
    COMMAND_HANDLER(IDC_REPLACE,CBN_EDITCHANGE, OnReplChanged)

    CHAIN_MSG_MAP_ALT(FRBase, 1)
  END_MSG_MAP()


  LRESULT OnCancel(WORD, WORD /* unused: wID */, HWND, BOOL&) {
	  GetData();
	  SaveSearchOptions();
	  m_view->CloseFindDialog(this);
    return 0;
  }
	LRESULT OnClose(UINT, WPARAM, LPARAM, BOOL&) {
		GetData();
		SaveSearchOptions();
		m_view->CloseFindDialog(this);
		return 0;
	}
  virtual void DoFind() = 0;
  LRESULT OnDoFind(WORD, WORD, HWND, BOOL&) {
    GetData();
    DoFind();
    return 0;
  }
  virtual void DoReplace() = 0;
  LRESULT OnDoReplace(WORD, WORD, HWND, BOOL&) {
    GetData();
    DoReplace();
    return 0;
  }
  virtual void DoReplaceAll() = 0;
  LRESULT OnDoReplaceAll(WORD, WORD, HWND, BOOL&) {
    GetData();
    DoReplaceAll();
    return 0;
  }
  LRESULT OnFindFromStart(WORD, WORD, HWND, BOOL&) {
	GetData();
	m_selvalid = false;
	if (!m_view->DoSearchFromScopeStart()) {
		if (!m_view->LastSearchError().IsEmpty() && m_view->LastSearchErrorIsRegexp())
			U::MessageBox(m_hWnd, m_view->LastSearchError(), FbeLoadRuntimeStringByKey(L"fbe.dialog.idd_replace.caption", L"Replace"), MB_OK | MB_ICONEXCLAMATION);
		else
			U::MessageBox(MB_OK | MB_ICONEXCLAMATION, IDR_MAINFRAME, IDS_SEARCH_FAIL_MSG, static_cast<LPCWSTR>(m_view->m_fo.pattern));
	} else {
		SaveString(); SaveHistory(); m_selvalid = true; MakeClose();
	}
	return 0;
  }
  LRESULT OnScopeChanged(WORD, WORD, HWND, BOOL&) {
	GetData(); m_view->ResetSearchScope(); m_view->SyncSearchOptionsToOpenDialogs(this); m_selvalid = false; return 0;
  }
  LRESULT OnScopeDropDown(WORD, WORD, HWND, BOOL&) {
	HWND scope = FRBase::GetDlgItem(IDC_FIND_SCOPE);
	const LRESULT selected = scope ? ::SendMessage(scope, CB_GETCURSEL, 0, 0) : CB_ERR;
	if (selected != CB_ERR) m_scope = static_cast<int>(::SendMessage(scope, CB_GETITEMDATA, selected, 0));
	PopulateFindScopes(); return 0;
  }
  LRESULT OnSearchOptionChanged(WORD, WORD, HWND, BOOL&) {
	GetData(); UpdateUnicodeControl(); m_view->SyncSearchOptionsToOpenDialogs(this); m_selvalid = false; return 0;
  }

  LRESULT OnTextChanged(WORD, WORD /* unused: wID */, HWND, BOOL& bHandled) {
    SendMessage(DM_SETDEFID,IDOK);
	m_selvalid=false;
	UpdatePresetActions();
    bHandled=FALSE;
    return 0;
  }
  virtual void InvalidateSearchSelectionState() { m_selvalid = false; }
  LRESULT OnReplChanged(WORD, WORD /* unused: wID */, HWND, BOOL& bHandled) {
    SendMessage(DM_SETDEFID,IDC_REPLACE_ONE);
    bHandled=FALSE;
    return 0;
  }
  void MakeClose() {
    // change cancel button to "Close"
	CString s;
	s = FbeLoadCString(IDS_MB_CLOSE);
	::SetWindowText(CModelessDialogImpl<CReplaceDlgBase>::GetDlgItem(IDCANCEL),s);
    SendMessage(DM_SETDEFID,IDC_REPLACE_ONE);
  }
  virtual HWND	X_GetDlgItem(int id) { return CModelessDialogImpl<CReplaceDlgBase>::GetDlgItem(id); }
};

class CViewFindDlg: public CFindDlgBase
{
public:
	CViewFindDlg(CFBEView* view) : CFindDlgBase(view) { }

	virtual FbeSearchPresets::SearchUiContext SearchContext() const { return FbeSearchPresets::SearchUiContext::Design; }

	virtual void DoFind()
	{
		GetData();
		VBErr = false;
		if(!m_view->DoSearch())
		{
			if (!VBErr)
			{
				if (!m_view->LastSearchError().IsEmpty() && m_view->LastSearchErrorIsRegexp())
					U::MessageBox(m_hWnd, m_view->LastSearchError(), FbeLoadRuntimeStringByKey(L"fbe.dialog.idd_find.caption", L"Find"), MB_OK | MB_ICONEXCLAMATION);
				else if (m_view->LastSearchError().IsEmpty())
					U::MessageBox(MB_OK | MB_ICONEXCLAMATION, IDR_MAINFRAME, IDS_SEARCH_FAIL_MSG, static_cast<LPCWSTR>(m_view->m_fo.pattern));
				else
					SetFindStatus(m_view->LastSearchError());
			}
		}
		else
		{
			SaveString();
			SaveHistory();
			SetFindStatus(m_view->SearchResultStatus());
		}
	}
};

#endif
