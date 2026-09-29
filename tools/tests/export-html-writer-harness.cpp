#include "../../src/export-html/stdafx.h"
#include "../../src/export-html/HtmlExportWriter.h"
#include "../../src/export-html/utils.h"

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

CComModule _Module;
CRegKey _Settings;
CString _SettingsPath;

namespace U {
HandleStreamPtr NewStream(HANDLE& handle, bool closeHandle)
{
	HandleStream* stream = NULL;
	CheckError(HandleStream::CreateInstance(&stream));
	stream->SetHandle(handle, closeHandle);
	if (closeHandle) handle = INVALID_HANDLE_VALUE;
	return stream;
}

void NormalizeInplace(CString&) {}
CString QuerySV(HKEY, const TCHAR*, const TCHAR*) { return CString(); }
DWORD QueryIV(HKEY, const TCHAR*, DWORD value) { return value; }
}

namespace {

std::wstring TemporaryPath(const wchar_t* name)
{
	wchar_t directory[MAX_PATH] = {};
	::GetTempPath(_countof(directory), directory);
	std::wstring path(directory);
	path += L"fbe-export-html-writer-";
	path += std::to_wstring(::GetCurrentProcessId());
	path += L"-";
	path += std::to_wstring(::GetTickCount64());
	path += L"-";
	path += name;
	return path;
}

bool ReadFileText(const std::wstring& path, std::string& text)
{
	std::ifstream file(path, std::ios::binary);
	if (!file) return false;
	text.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
	return true;
}

int WriteText(IStream* stream, const char* text)
{
	ULONG written = 0;
	const ULONG length = static_cast<ULONG>(strlen(text));
	return SUCCEEDED(stream->Write(text, length, &written)) && written == length ? 0 : 1;
}

HtmlExportWriter::Callbacks Callbacks(int& failures)
{
	HtmlExportWriter::Callbacks callbacks;
	callbacks.reportFailure = [&failures](HtmlExportWriter::Failure, const std::wstring&, DWORD) { ++failures; };
	callbacks.confirmImageOverwrite = [](const std::wstring&) { return false; };
	return callbacks;
}

int ExpectNormalWrite()
{
	const std::wstring path = TemporaryPath(L"normal.html");
	int reported = 0;
	{
		HtmlExportWriter::Options options;
		options.targetPath = path;
		HtmlExportWriter::Writer writer(options, Callbacks(reported));
		if (FAILED(writer.Prepare())) return 1;
		CComPtr<IStream> output;
		if (FAILED(writer.GetTransformOutput(&output)) || WriteText(output, "normal")) return 1;
		if (FAILED(writer.Finalize())) return 1;
		writer.Commit();
	}
	std::string text;
	const int failure = reported != 0 || !ReadFileText(path, text) || text != "normal";
	::DeleteFile(path.c_str());
	return failure;
}

int ExpectOpenFailure()
{
	HtmlExportWriter::Options options;
	options.targetPath = TemporaryPath(L"missing\\target.html");
	int reported = 0;
	HtmlExportWriter::Writer writer(options, Callbacks(reported));
	return FAILED(writer.Prepare()) && reported == 1 ? 0 : 1;
}

int ExpectStandaloneWrite()
{
	const std::wstring path = TemporaryPath(L"standalone.html");
	int reported = 0;
	{
		HtmlExportWriter::Options options;
		options.targetPath = path;
		options.standalone = true;
		HtmlExportWriter::Writer writer(options, Callbacks(reported));
		if (FAILED(writer.Prepare())) return 1;
		CComPtr<IStream> output;
		if (FAILED(writer.GetTransformOutput(&output)) || WriteText(output, "standalone")) return 1;
		if (FAILED(writer.WriteStandaloneToTarget()) || FAILED(writer.Finalize())) return 1;
		writer.Commit();
	}
	std::string text;
	const int failure = reported != 0 || !ReadFileText(path, text) || text != "standalone";
	::DeleteFile(path.c_str());
	return failure;
}

int ExpectMimeFinalBoundary()
{
	const std::wstring path = TemporaryPath(L"document.mht");
	int reported = 0;
	{
		HtmlExportWriter::Options options;
		options.targetPath = path;
		options.mime = true;
		HtmlExportWriter::Writer writer(options, Callbacks(reported));
		if (FAILED(writer.Prepare())) return 1;
		CComPtr<IStream> output;
		if (FAILED(writer.GetTransformOutput(&output)) || WriteText(output, "mime-body")) return 1;
		if (FAILED(writer.Finalize())) return 1;
		writer.Commit();
	}
	std::string text;
	const int failure = reported != 0 || !ReadFileText(path, text) ||
		text.find("Content-Type: multipart/related") == std::string::npos ||
		text.find("mime-body") == std::string::npos ||
		text.size() < 4 || text.compare(text.size() - 2, 2, "\r\n") != 0;
	::DeleteFile(path.c_str());
	return failure;
}

int ExpectExternalImageWrite()
{
	const std::wstring path = TemporaryPath(L"images.html");
	const std::wstring images = path.substr(0, path.size() - 5) + L"_files";
	const std::wstring imagePath = images + L"\\pixel.bin";
	int reported = 0;
	IXMLDOMDocument2Ptr document;
	if (FAILED(document.CreateInstance(CLSID_DOMDocument60))) return 1;
	VARIANT_BOOL loaded = VARIANT_FALSE;
	if (FAILED(document->loadXML(CComBSTR(L"<FictionBook xmlns='http://www.gribuser.ru/xml/fictionbook/2.0'><binary id='pixel.bin' content-type='application/octet-stream'>AA==</binary></FictionBook>"), &loaded)) || loaded != VARIANT_TRUE) return 1;
	if (FAILED(document->setProperty(CComBSTR(L"SelectionLanguage"), _variant_t(L"XPath"))) ||
		FAILED(document->setProperty(CComBSTR(L"SelectionNamespaces"), _variant_t(L"xmlns:fb='http://www.gribuser.ru/xml/fictionbook/2.0'")))) return 1;
	{
		HtmlExportWriter::Options options;
		options.targetPath = path;
		options.externalImages = true;
		HtmlExportWriter::Writer writer(options, Callbacks(reported));
		if (FAILED(writer.Prepare()) || FAILED(writer.WriteImages(document)) || FAILED(writer.Finalize())) return 1;
		writer.Commit();
	}
	HANDLE image = ::CreateFile(imagePath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	DWORD size = image == INVALID_HANDLE_VALUE ? 0 : ::GetFileSize(image, NULL);
	if (image != INVALID_HANDLE_VALUE) ::CloseHandle(image);
	const int failure = reported != 0 || size != 1;
	::DeleteFile(imagePath.c_str());
	::RemoveDirectory(images.c_str());
	::DeleteFile(path.c_str());
	return failure;
}

int ExpectRollback()
{
	const std::wstring path = TemporaryPath(L"rollback.html");
	int reported = 0;
	{
		HtmlExportWriter::Options options;
		options.targetPath = path;
		HtmlExportWriter::Writer writer(options, Callbacks(reported));
		if (FAILED(writer.Prepare())) return 1;
		CComPtr<IStream> output;
		if (FAILED(writer.GetTransformOutput(&output)) || WriteText(output, "partial")) return 1;
		// No Commit(): destruction must close and remove the partial target.
	}
	return ::GetFileAttributes(path.c_str()) == INVALID_FILE_ATTRIBUTES ? 0 : 1;
}

}

int main()
{
	::CoInitialize(NULL);
	const int failures = ExpectNormalWrite() + ExpectOpenFailure() + ExpectStandaloneWrite() +
		ExpectMimeFinalBoundary() + ExpectExternalImageWrite() + ExpectRollback();
	::CoUninitialize();
	if (failures != 0) std::cerr << "writer harness failures: " << failures << std::endl;
	return failures == 0 ? 0 : 1;
}