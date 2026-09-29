#include "stdafx.h"
#include "ExportHTMLPlugin.h"

#include "utils.h"
#include "HtmlExportOptionsDialog.h"
#include "HtmlExportResourceAudit.h"
#include "HtmlExportXslParameters.h"
#include "HtmlExportWriterHelpers.h"
#include "..\\common\\ModernFileDialog.h"
#include "RuntimeLocalization.h"
#include "..\\version.h"

#include <vector>

namespace {

bool LoadUtf8TextFile(const CString& filename, CString& text)
{
	text.Empty();
	if (filename.IsEmpty())
		return true;

	HANDLE file = ::CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, NULL,
		OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if (file == INVALID_HANDLE_VALUE)
		return false;

	DWORD sizeHigh = 0;
	DWORD size = ::GetFileSize(file, &sizeHigh);
	if (size == INVALID_FILE_SIZE || sizeHigh != 0 || size > 16 * 1024 * 1024) {
		::CloseHandle(file);
		::SetLastError(ERROR_FILE_TOO_LARGE);
		return false;
	}

	std::vector<char> bytes(size);
	DWORD read = 0;
	BOOL ok = size == 0 || ::ReadFile(file, &bytes[0], size, &read, NULL);
	::CloseHandle(file);
	if (!ok || read != size)
		return false;

	DWORD offset = size >= 3 && (unsigned char)bytes[0] == 0xEF &&
		(unsigned char)bytes[1] == 0xBB && (unsigned char)bytes[2] == 0xBF ? 3 : 0;
	int sourceLength = static_cast<int>(size - offset);
	if (sourceLength == 0)
		return true;

	UINT codePage = CP_UTF8;
	int length = ::MultiByteToWideChar(codePage, MB_ERR_INVALID_CHARS,
		&bytes[offset], sourceLength, NULL, 0);
	if (length == 0) {
		codePage = CP_ACP;
		length = ::MultiByteToWideChar(codePage, 0, &bytes[offset], sourceLength, NULL, 0);
	}
	if (length == 0)
		return false;

	wchar_t* buffer = text.GetBuffer(length);
	if (::MultiByteToWideChar(codePage, 0, &bytes[offset], sourceLength, buffer, length) == 0) {
		text.ReleaseBuffer(0);
		return false;
	}
	text.ReleaseBuffer(length);
	return true;
}

std::wstring ReadUtf8Stream(IStream* stream)
{
	STATSTG stat = {};
	CheckError(stream->Stat(&stat, STATFLAG_NONAME));
	LARGE_INTEGER start = {};
	CheckError(stream->Seek(start, STREAM_SEEK_SET, NULL));
	std::vector<char> bytes(static_cast<size_t>(stat.cbSize.QuadPart));
	ULONG read = 0;
	if (!bytes.empty()) CheckError(stream->Read(&bytes[0], static_cast<ULONG>(bytes.size()), &read));
	if (read != bytes.size()) throw _com_error(E_FAIL);
	const int length = bytes.empty() ? 0 : ::MultiByteToWideChar(CP_UTF8, 0,
		&bytes[0], static_cast<int>(bytes.size()), NULL, 0);
	if (!bytes.empty() && length == 0) throw _com_error(HRESULT_FROM_WIN32(::GetLastError()));
	std::wstring text(static_cast<size_t>(length), L'\0');
	if (length > 0) ::MultiByteToWideChar(CP_UTF8, 0, &bytes[0],
		static_cast<int>(bytes.size()), &text[0], length);
	return text;
}

}

STDMETHODIMP CExportHTMLPlugin::GetPluginId(BSTR* value)
{
	if (!value) return E_POINTER; *value = ::SysAllocString(L"export-html"); return *value ? S_OK : E_OUTOFMEMORY;
}
STDMETHODIMP CExportHTMLPlugin::GetPluginVersion(BSTR* value)
{
	if (!value) return E_POINTER; *value = ::SysAllocString(FBE_VERSION_WSTRING); return *value ? S_OK : E_OUTOFMEMORY;
}
STDMETHODIMP CExportHTMLPlugin::GetApiVersion(ULONG* value) { if (!value) return E_POINTER; *value = 2; return S_OK; }
STDMETHODIMP CExportHTMLPlugin::GetCapabilities(ULONGLONG* value) { if (!value) return E_POINTER; *value = 0; return S_OK; }

STDMETHODIMP CExportHTMLPlugin::Export(IFBEPluginHost* host, BSTR filename, IFBEDocumentSnapshot* document)
{
	if (!host || !document) return E_POINTER;
	LONGLONG ownerValue = 0; HRESULT hr = host->GetOwnerWindow(&ownerValue); if (FAILED(hr)) return hr;
	BSTR hostVersion = NULL, hostLocale = NULL;
	hr = host->GetHostVersion(&hostVersion); if (SUCCEEDED(hr)) hr = host->GetUiLocale(&hostLocale);
	if (hostVersion) ::SysFreeString(hostVersion); if (hostLocale) ::SysFreeString(hostLocale);
	if (FAILED(hr)) return hr;
	CComPtr<IFBECancellationToken> cancellation; hr = host->GetCancellationToken(&cancellation); if (FAILED(hr)) return hr;
	BOOL cancelled = FALSE; hr = cancellation->IsCancellationRequested(&cancelled); if (FAILED(hr) || cancelled) return FAILED(hr) ? hr : HRESULT_FROM_WIN32(ERROR_CANCELLED);
	CComPtr<IStream> stream; hr = document->OpenXmlStream(&stream); if (FAILED(hr)) return hr;
	IXMLDOMDocument2Ptr source(U::CreateDocument(false)); VARIANT_BOOL loaded = VARIANT_FALSE;
	hr = source->load(_variant_t((IUnknown*)stream), &loaded); if (FAILED(hr) || loaded != VARIANT_TRUE) { host->ReportMessage(2, CComBSTR(L"xml-load"), CComBSTR(L"snapshot XML could not be loaded")); return FAILED(hr) ? hr : E_FAIL; }
	CComPtr<IFBEProgressSink> progress; if (SUCCEEDED(host->GetProgressSink(&progress))) progress->Report(0, 1, CComBSTR(L"export-html"));
	hr = ExportCore(static_cast<long>(ownerValue), filename, source);
	if (SUCCEEDED(hr) && progress) progress->Report(1, 1, CComBSTR(L"export-html"));
	if (FAILED(hr) && hr != HRESULT_FROM_WIN32(ERROR_CANCELLED))
		host->ReportMessage(2, CComBSTR(L"export-failed"), CComBSTR(L"ExportHTML failed"));
	return hr;
}

HRESULT CExportHTMLPlugin::Export(long hWnd, BSTR filename, IDispatch* doc)
{
	// v1 historically treats both a dialog cancellation and an export error as
	// a non-exceptional, non-success result.  Keep that observable contract.
	const HRESULT result = ExportCore(hWnd, filename, doc);
	return FAILED(result) ? S_FALSE : result;
}

HRESULT CExportHTMLPlugin::ExportCore(long hWnd, BSTR filename, IDispatch *doc)
{
	wchar_t testModeValue[4] = {}, testCancel[4] = {}, testFail[4] = {}, testScenarioValue[32] = {};
	const bool exportHtmlTest = ::GetEnvironmentVariable(L"FBE_NEXT_TEST_MODE", testModeValue, _countof(testModeValue)) == 1 && testModeValue[0] == L'1' &&
		::GetEnvironmentVariable(L"FBE_NEXT_TEST_SCENARIO", testScenarioValue, _countof(testScenarioValue)) == wcslen(L"export-html") && wcscmp(testScenarioValue, L"export-html") == 0;
	if (exportHtmlTest && ::GetEnvironmentVariable(L"FBE_NEXT_TEST_EXPORT_HTML_FAIL", testFail, _countof(testFail)) == 1 && testFail[0] == L'1')
		return E_FAIL;
	if (exportHtmlTest &&
		::GetEnvironmentVariable(L"FBE_NEXT_TEST_EXPORT_HTML_CANCEL", testCancel, _countof(testCancel)) == 1 && testCancel[0] == L'1')
		return HRESULT_FROM_WIN32(ERROR_CANCELLED);
	InitExportHtmlRuntimeStrings();

	HANDLE  hOut = INVALID_HANDLE_VALUE;
	CString strMessage;
	HtmlExportSettings exportSettings;

	try {
		// * construct doc pointer
		IXMLDOMDocument2Ptr	    source(doc);
		// Work on a private DOM copy: export cleanup must not modify the open book.
		// cloneNode returns a generic node wrapper.  MSXML's XSL processor can
		// then lose the document-owned key() index used by html.xsl for FB2
		// binaries.  Round-trip through a private DOM keeps the editor document
		// untouched while retaining a real document owner for the transform.
		CComBSTR sourceXml;
		CheckError(source->get_xml(&sourceXml));
		IXMLDOMDocument2Ptr sourceCopy(U::CreateDocument(false));
		VARIANT_BOOL sourceLoaded = VARIANT_FALSE;
		CheckError(sourceCopy->loadXML(sourceXml, &sourceLoaded));
		if (sourceLoaded != VARIANT_TRUE)
			return E_FAIL;
		source = sourceCopy;
		CheckError(source->setProperty(bstr_t(L"SelectionLanguage"), variant_t(L"XPath")));
		CheckError(source->setProperty(bstr_t(L"SelectionNamespaces"),
			variant_t(L"xmlns:fb='http://www.gribuser.ru/xml/fictionbook/2.0'")));
		wchar_t testDomPath[MAX_PATH] = {};
		const DWORD testDomPathLength = ::GetEnvironmentVariable(L"FBE_NEXT_TEST_EXPORT_HTML_DOM_PATH", testDomPath, _countof(testDomPath));
		if (testDomPathLength > 0 && testDomPathLength < _countof(testDomPath))
			CheckError(source->save(_variant_t(testDomPath)));

		// * ask the user where he wants his html
		struct HtmlExportDialogState { struct { UINT nFilterIndex; } m_ofn; wchar_t m_szFileName[MAX_PATH]; CString m_template, m_customCss; bool m_usingCustomTemplate, m_includedesc; int m_tocdepth, m_imageMaxWidth, m_imageMaxHeight; } dlg = {};
		// The portable, self-contained document is the safest default: it cannot
		// lose its CSS or images when moved to another folder or machine.
		dlg.m_ofn.nFilterIndex = 4;
		wchar_t testModeEnabled[4] = {}, dialogTestScenario[32] = {}, testOutput[MAX_PATH] = {};
		const bool deterministicTestExport = ::GetEnvironmentVariable(L"FBE_NEXT_TEST_MODE", testModeEnabled, _countof(testModeEnabled)) == 1 &&
			testModeEnabled[0] == L'1' && ::GetEnvironmentVariable(L"FBE_NEXT_TEST_SCENARIO", dialogTestScenario, _countof(dialogTestScenario)) == wcslen(L"export-html") &&
			wcscmp(dialogTestScenario, L"export-html") == 0;
		const DWORD testOutputLength = deterministicTestExport ? ::GetEnvironmentVariable(L"FBE_NEXT_TEST_EXPORT_HTML_PATH", testOutput, _countof(testOutput)) : 0;
		if (testOutputLength > 0 && testOutputLength < _countof(testOutput)) {
			wchar_t testExportMode[8] = {};
			const DWORD testModeLength = ::GetEnvironmentVariable(L"FBE_NEXT_TEST_EXPORT_HTML_MODE", testExportMode, _countof(testExportMode));
			dlg.m_ofn.nFilterIndex = testModeLength ? max(1, min(4, _wtoi(testExportMode))) : 4;
			::wcsncpy_s(dlg.m_szFileName, _countof(dlg.m_szFileName), testOutput, _TRUNCATE);
			dlg.m_template = U::GetProgDirFile(L"html.xsl");
			dlg.m_usingCustomTemplate = false;
			exportSettings.templatePath = dlg.m_template;
		} else {
			CHtmlExportOptionsDialog options;
			options.LoadSettings();
			std::vector<CString> filterLabels, filterPatterns;
			std::vector<COMDLG_FILTERSPEC> filters;
			BuildHtmlModernFileTypes(LoadExportHtmlString(IDS_SAVE_FILE_FILTER), filterLabels, filterPatterns, filters);
			CComObject<CHtmlFileDialogEvents>* rawEvents = nullptr;
			HRESULT eventHr = CComObject<CHtmlFileDialogEvents>::CreateInstance(&rawEvents);
			if (FAILED(eventHr) || !rawEvents) return FAILED(eventHr) ? eventHr : E_FAIL;
			rawEvents->AddRef();
			rawEvents->owner = (HWND)hWnd;
			rawEvents->options = &options;
			CComPtr<IFileDialogEvents> events;
			eventHr = rawEvents->QueryInterface(IID_PPV_ARGS(&events));
			rawEvents->Release();
			if (FAILED(eventHr)) return eventHr;
			ModernFileDialog::Request request;
			request.save = true; request.pathMustExist = true; request.overwritePrompt = true; request.defaultExtension = L"html";
			request.initialFileName = filename ? filename : L""; request.filters = filters.data(); request.filterCount = static_cast<UINT>(filters.size()); request.filterIndex = 4;
			request.events = events;
			request.customize = [](IFileDialogCustomize* customize) {
				CString button = LoadExportHtmlString(IDS_HTML_EXPORT_OPTIONS_TITLE);
				button += L"...";
				return customize->AddPushButton(CHtmlFileDialogEvents::SettingsButtonId,
					button);
			};
			const ModernFileDialog::Result result = ModernFileDialog::Show((HWND)hWnd, request);
			if (result.outcome == ModernFileDialog::Outcome::Cancelled) return HRESULT_FROM_WIN32(ERROR_CANCELLED);
			if (result.outcome == ModernFileDialog::Outcome::Failed) {
				FbeDiagnostic::HResult(L"file-dialog", L"FD203", result.error, L"Export HTML save dialog");
				return FAILED(result.error) ? result.error : E_FAIL;
			}
			exportSettings = options.m_settings;
			dlg.m_template = exportSettings.templatePath; dlg.m_customCss = exportSettings.customCss; dlg.m_usingCustomTemplate = exportSettings.usingCustomTemplate;
			dlg.m_includedesc = exportSettings.includeDescription; dlg.m_tocdepth = exportSettings.tocDepth; dlg.m_imageMaxWidth = exportSettings.imageMaxWidth; dlg.m_imageMaxHeight = exportSettings.imageMaxHeight;
			options.Persist();
			dlg.m_ofn.nFilterIndex = result.filterIndex;
			::wcsncpy_s(dlg.m_szFileName, _countof(dlg.m_szFileName), result.paths.front().c_str(), _TRUNCATE);
		}
		bool    fMIME = dlg.m_ofn.nFilterIndex == 2;
		bool    fExternalImages = dlg.m_ofn.nFilterIndex == 1;
		bool    fEmbeddedImages = dlg.m_ofn.nFilterIndex == 4;
		bool    fImages = fExternalImages || fMIME || fEmbeddedImages;
		CString customCss;
		if (!LoadUtf8TextFile(dlg.m_customCss, customCss)) {
			strMessage = FormatExportHtmlString(IDS_ERROR_OPEN_FILE, (LPCTSTR)dlg.m_customCss,
				(LPCTSTR)U::Win32ErrMsg(::GetLastError()));
			ShowExportHtmlTaskDialog(::GetActiveWindow(), IDR_EXPORTHTML, (LPCTSTR)strMessage,
				(LPCTSTR)NULL, TDCBF_OK_BUTTON, TD_ERROR_ICON);
			return E_FAIL;
		}

		// * load template
		// MSXML does not reliably populate XSL key() indexes for a stylesheet
		// loaded through FreeThreadedDOMDocument.  html.xsl resolves FB2 binary
		// nodes through key('binary-by-id', ...), so use the regular DOM that the
		// processor contract expects.
		IXMLDOMDocument2Ptr	    tdoc(U::CreateDocument(false));
		if (!U::LoadXml(tdoc, dlg.m_template))
			return E_FAIL;
		if (fEmbeddedImages && dlg.m_usingCustomTemplate && !SupportsEmbeddedImages(tdoc)) {
			ShowExportHtmlTaskDialog(::GetActiveWindow(), IDR_EXPORTHTML,
				LoadExportHtmlString(IDS_ERROR_EMBEDDED_IMAGES_TEMPLATE),
				NULL, TDCBF_OK_BUTTON, TD_ERROR_ICON);
			return E_FAIL;
		}
		IXSLTemplatePtr	    tmpl(U::CreateTemplate());
		CheckError(tmpl->putref_stylesheet(tdoc));

		// * create processor
		IXSLProcessorPtr	    proc;
		CheckError(tmpl->createProcessor(&proc));

		// * setup input
		// ExportHTML cannot reliably distinguish editor markers from legitimate
		// book text such as "{2026}", so it never strips brace-delimited text.
		CheckError(proc->put_input(variant_t((IDispatch*)source)));

		// Keep XSL parameters behind a value-model adapter; ExportCore remains a writer.
		HtmlExportXslParameters::Apply(proc, exportSettings, customCss);

		// 1 = HTML and an adjacent resource folder, 2 = MHT,
		// 3 = HTML without images, 4 = self-contained HTML with data: URIs.
		HtmlExportWriterHelpers::ImagePaths imagePaths;
		const int imagesFolderMode = fExternalImages ? exportSettings.externalImagesFolderMode : 0;
		const std::wstring imagesFolderName = fExternalImages ? std::wstring((LPCWSTR)exportSettings.externalImagesFolderName) : std::wstring();
		if (!HtmlExportWriterHelpers::BuildImagePaths((LPCWSTR)dlg.m_szFileName, imagesFolderMode, imagesFolderName, imagePaths)) return E_FAIL;
		CString dfile(imagePaths.directory.c_str());

		// Self-contained output is first transformed in memory and audited before
		// touching the selected target.  Other modes preserve the historic writer.
		if (!fEmbeddedImages) {
			hOut = ::CreateFile(dlg.m_szFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
			if (hOut == INVALID_HANDLE_VALUE) {
			CString openErrorMessage;
			openErrorMessage = FormatExportHtmlString(IDS_ERROR_OPEN_FILE, dlg.m_szFileName, (LPCTSTR)U::Win32ErrMsg(::GetLastError()));
			ShowExportHtmlTaskDialog(::GetActiveWindow(), IDR_EXPORTHTML, (LPCTSTR)openErrorMessage, (LPCTSTR)NULL, TDCBF_OK_BUTTON, TD_ERROR_ICON);
			return E_FAIL;
			}
		}

		CString relpath;
		if (fExternalImages) {
			relpath = imagePaths.imgPrefix.c_str();

			if (!fMIME) {
				if (!::CreateDirectory(dfile, NULL) && ::GetLastError() != ERROR_ALREADY_EXISTS) {
					DWORD	de = ::GetLastError();
					CloseHandle(hOut);
					::DeleteFile(dlg.m_szFileName);
					strMessage = FormatExportHtmlString(IDS_ERROR_CREATE_DIRECTORY, (LPCTSTR)dfile, (LPCTSTR)U::Win32ErrMsg(de));
					ShowExportHtmlTaskDialog(::GetActiveWindow(), IDR_EXPORTHTML, (LPCTSTR)strMessage, (LPCTSTR)NULL, TDCBF_OK_BUTTON, TD_ERROR_ICON);
					return E_FAIL;
				}
			}
			else
				dfile.Empty();
		}
		HtmlExportXslParameters::ApplyImageMode(proc, fImages, fEmbeddedImages, relpath);

		std::string boundary;

		// * write relevant MIME headers
		if (fMIME) {
			HtmlExportWriterHelpers::MimePreamble mimePreamble;
			const time_t timestamp = time(NULL);
			if (!HtmlExportWriterHelpers::BuildMimePreamble(timestamp, static_cast<unsigned int>(rand()), mimePreamble)) return E_FAIL;
			boundary = mimePreamble.boundary;

			DWORD   len = static_cast<DWORD>(mimePreamble.header.size());
			DWORD   nw;
			BOOL    fWr = WriteFile(hOut, mimePreamble.header.data(), len, &nw, NULL);
			if (!fWr || nw != len)
			{
				if (!fWr)
				{
					strMessage = FormatExportHtmlString(IDS_ERROR_WRITE_FILE, dlg.m_szFileName, (LPCTSTR)U::Win32ErrMsg(::GetLastError()));
					ShowExportHtmlTaskDialog(::GetActiveWindow(), IDR_EXPORTHTML, (LPCTSTR)strMessage, (LPCTSTR)NULL, TDCBF_OK_BUTTON, TD_ERROR_ICON);
				}
				else
				{
					strMessage = FormatExportHtmlString(IDS_ERROR_WRITE_FILE2, dlg.m_szFileName);
					ShowExportHtmlTaskDialog(::GetActiveWindow(), IDR_EXPORTHTML, (LPCTSTR)strMessage, (LPCTSTR)NULL, TDCBF_OK_BUTTON, TD_ERROR_ICON);
				}
				::CloseHandle(hOut);
				::DeleteFile(dlg.m_szFileName);
						return E_FAIL;
			}
		}

		// * transform
		CComPtr<IStream> standaloneOutput;
		if (fEmbeddedImages) {
			CheckError(::CreateStreamOnHGlobal(NULL, TRUE, &standaloneOutput));
			CheckError(proc->put_output(variant_t((IUnknown*)standaloneOutput)));
		} else {
			CheckError(proc->put_output(variant_t((IUnknown*)U::NewStream(hOut, !fMIME))));
		}
		VARIANT_BOOL Done = VARIANT_FALSE;
		CheckError(proc->transform(&Done));
		if (fEmbeddedImages) {
			STATSTG standaloneStat = {};
			CheckError(standaloneOutput->Stat(&standaloneStat, STATFLAG_NONAME));
			if (HtmlExportWriterHelpers::IsStandaloneWarningRequired(
				static_cast<unsigned long long>(standaloneStat.cbSize.QuadPart),
				static_cast<unsigned long long>(exportSettings.standaloneWarningMiB)) &&
				ShowExportHtmlTaskDialog(::GetActiveWindow(), IDR_EXPORTHTML,
					FormatExportHtmlString(IDS_WARNING_STANDALONE_SIZE, exportSettings.standaloneWarningMiB), NULL,
					TDCBF_YES_BUTTON | TDCBF_NO_BUTTON, TD_WARNING_ICON) != IDYES)
				return HRESULT_FROM_WIN32(ERROR_CANCELLED);
			const std::vector<std::wstring> dependencies = HtmlExportResourceAudit::FindExternalDependencies(ReadUtf8Stream(standaloneOutput));
			if (!dependencies.empty() && ShowExportHtmlTaskDialog(::GetActiveWindow(), IDR_EXPORTHTML,
				FormatExportHtmlString(IDS_WARNING_EXTERNAL_RESOURCES, static_cast<int>(dependencies.size())), NULL,
				TDCBF_YES_BUTTON | TDCBF_NO_BUTTON, TD_WARNING_ICON) != IDYES)
				return HRESULT_FROM_WIN32(ERROR_CANCELLED);
			hOut = ::CreateFile(dlg.m_szFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
			if (hOut == INVALID_HANDLE_VALUE) {
				CString openErrorMessage = FormatExportHtmlString(IDS_ERROR_OPEN_FILE, dlg.m_szFileName, (LPCTSTR)U::Win32ErrMsg(::GetLastError()));
				ShowExportHtmlTaskDialog(::GetActiveWindow(), IDR_EXPORTHTML, (LPCTSTR)openErrorMessage, NULL, TDCBF_OK_BUTTON, TD_ERROR_ICON);
				return E_FAIL;
			}
			LARGE_INTEGER start = {};
			CheckError(standaloneOutput->Seek(start, STREAM_SEEK_SET, NULL));
			ULARGE_INTEGER written = {};
			CheckError(standaloneOutput->CopyTo(U::NewStream(hOut, true), ULARGE_INTEGER{ 0xFFFFFFFF, 0x7FFFFFFF }, NULL, &written));
		}

		// * save images
		if (fExternalImages || fMIME) {
			if (dfile.IsEmpty() || dfile[dfile.GetLength() - 1] != _T('\\'))
				dfile += _T('\\');
			IXMLDOMNodeListPtr      bins;
			CheckError(source->selectNodes(bstr_t(L"/fb:FictionBook/fb:binary"), &bins));
			long listLength = 0;
			CheckError(bins->get_length(&listLength));
			for (long l = 0; l < listLength; ++l) {
				try {
					IXMLDOMNodePtr   be;
					CheckError(bins->get_item(l, &be));
					IXMLDOMElementPtr element;
					CheckError(be->QueryInterface(IID_PPV_ARGS(&element)));
					_variant_t	id;
					CheckError(element->getAttribute(bstr_t(L"id"), &id));
					_variant_t	ct;
					CheckError(element->getAttribute(bstr_t(L"content-type"), &ct));
					if (V_VT(&id) != VT_BSTR || V_VT(&ct) != VT_BSTR)
						continue;

					if (fMIME) {
						// get base64 data
						CComBSTR   data;
						CheckError(be->get_text(&data));

						// allocate buffer
						char      *buffer = (char*)malloc(data.Length() + 1024);
						if (buffer == NULL)
							continue;

						// construct a MIME header
						_snprintf_s(buffer, 1024, _TRUNCATE,
							"\r\n"
							"%s\r\n"
							"Content-Type: %S\r\n"
							"Content-Transfer-Encoding: base64\r\n"
							"Content-Location: %S\r\n"
							"\r\n",
							boundary.c_str(), V_BSTR(&ct), V_BSTR(&id));
						DWORD     hlen = strlen(buffer);

						// convert data to ascii
						DWORD     mlen = WideCharToMultiByte(CP_ACP, 0,
							data, data.Length(),
							buffer + hlen, data.Length(),
							NULL, NULL);

						// write a new mime header+data
						DWORD   nw;
						BOOL    fWr = WriteFile(hOut, buffer, hlen + mlen, &nw, NULL);
						DWORD   de = ::GetLastError();
						free(buffer);

						if (!fWr || nw != hlen + mlen)
						{
							if (!fWr)
							{
								strMessage = FormatExportHtmlString(IDS_ERROR_WRITE_FILE, dlg.m_szFileName, (LPCTSTR)U::Win32ErrMsg(de));
								ShowExportHtmlTaskDialog(::GetActiveWindow(), IDR_EXPORTHTML, (LPCTSTR)strMessage, (LPCTSTR)NULL, TDCBF_OK_BUTTON, TD_ERROR_ICON);
							}
							else
							{
								strMessage = FormatExportHtmlString(IDS_ERROR_WRITE_FILE2, dlg.m_szFileName);
								ShowExportHtmlTaskDialog(::GetActiveWindow(), IDR_EXPORTHTML, (LPCTSTR)strMessage, (LPCTSTR)NULL, TDCBF_OK_BUTTON, TD_ERROR_ICON);
							}
							::CloseHandle(hOut);
							::DeleteFile(dlg.m_szFileName);
							return E_FAIL;
						}
					}
					else
					{
						CheckError(be->put_dataType(bstr_t(L"bin.base64")));
						_variant_t	data;
						CheckError(be->get_nodeTypedValue(&data));
						if (V_VT(&data) != (VT_ARRAY | VT_UI1) || ::SafeArrayGetDim(V_ARRAY(&data)) != 1)
							continue;
						DWORD len = V_ARRAY(&data)->rgsabound[0].cElements;
						void	*buffer;
						::SafeArrayAccessData(V_ARRAY(&data), &buffer);
						CString fname(dfile);
						fname += V_BSTR(&id);
						HANDLE hFile = ::CreateFile(fname, GENERIC_WRITE, 0, NULL, CREATE_NEW, 0, NULL);
						if (hFile == INVALID_HANDLE_VALUE && ::GetLastError() == ERROR_FILE_EXISTS)
						{
							strMessage = FormatExportHtmlString(IDS_WARNING_FILE_ALREADY_EXISTS, (LPCTSTR)fname);
							if (ShowExportHtmlTaskDialog(::GetActiveWindow(), IDR_EXPORTHTML, (LPCTSTR)strMessage, (LPCTSTR)NULL, TDCBF_YES_BUTTON | TDCBF_NO_BUTTON, TD_WARNING_ICON) != IDYES)
								goto skip;
							hFile = ::CreateFile(fname, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
						}
						if (hFile != INVALID_HANDLE_VALUE)
						{
							DWORD wr;
							BOOL fWr = ::WriteFile(hFile, buffer, len, &wr, NULL);
							DWORD de = ::GetLastError();
							::CloseHandle(hFile);
							if (!fWr || wr != len)
							{
								if (!fWr)
								{
									strMessage = FormatExportHtmlString(IDS_ERROR_WRITE_FILE, (LPCTSTR)fname, (LPCTSTR)U::Win32ErrMsg(de));
									ShowExportHtmlTaskDialog(::GetActiveWindow(), IDR_EXPORTHTML, (LPCTSTR)strMessage, (LPCTSTR)NULL, TDCBF_OK_BUTTON, TD_ERROR_ICON);
								}
								else
								{
									strMessage = FormatExportHtmlString(IDS_ERROR_WRITE_FILE2, (LPCTSTR)fname);
									ShowExportHtmlTaskDialog(::GetActiveWindow(), IDR_EXPORTHTML, (LPCTSTR)strMessage, (LPCTSTR)NULL, TDCBF_OK_BUTTON, TD_ERROR_ICON);
								}
								::DeleteFile(fname);
							}
						}
						else
						{
							strMessage = FormatExportHtmlString(IDS_ERROR_OPEN_FILE, (LPCTSTR)fname, (LPCTSTR)U::Win32ErrMsg(::GetLastError()));
							ShowExportHtmlTaskDialog(::GetActiveWindow(), IDR_EXPORTHTML, (LPCTSTR)strMessage, (LPCTSTR)NULL, TDCBF_OK_BUTTON, TD_ERROR_ICON);
						}
					skip:
						::SafeArrayUnaccessData(V_ARRAY(&data));
					}
				}
				catch (const _com_error&)
				{
					// Ошибка отдельного изображения не должна прерывать экспорт остальных.
					continue;
				}
			}
		}

		// * write a final mime boundary
		if (fMIME) {
			char    mime_tmp[256];
			_snprintf_s(mime_tmp, sizeof(mime_tmp), "\r\n%s\r\n", boundary.c_str());
			DWORD   len = strlen(mime_tmp);
			DWORD   nw;
			BOOL    fWr = WriteFile(hOut, mime_tmp, len, &nw, NULL);
			if (!fWr || nw != len)
			{
				if (!fWr)
				{
					strMessage = FormatExportHtmlString(IDS_ERROR_WRITE_FILE, dlg.m_szFileName, (LPCTSTR)U::Win32ErrMsg(::GetLastError()));
					ShowExportHtmlTaskDialog(::GetActiveWindow(), IDR_EXPORTHTML, (LPCTSTR)strMessage, (LPCTSTR)NULL, TDCBF_OK_BUTTON, TD_ERROR_ICON);
				}
				else
				{
					strMessage = FormatExportHtmlString(IDS_ERROR_WRITE_FILE2, dlg.m_szFileName);
					ShowExportHtmlTaskDialog(::GetActiveWindow(), IDR_EXPORTHTML, (LPCTSTR)strMessage, (LPCTSTR)NULL, TDCBF_OK_BUTTON, TD_ERROR_ICON);
				}
				::CloseHandle(hOut);
				::DeleteFile(dlg.m_szFileName);
				return E_FAIL;
			}
			::CloseHandle(hOut);
		}
	}
	catch (_com_error& e)
	{
		if (hOut != INVALID_HANDLE_VALUE)
			CloseHandle(hOut);
		U::ReportError(e);
		return e.Error();
	}
	return S_OK;
}
