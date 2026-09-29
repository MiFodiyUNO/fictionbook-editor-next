#include "stdafx.h"
#include "HtmlExportWriterHelpers.h"

#include <cwctype>

namespace {

bool IsPathSeparator(wchar_t value)
{
	return value == L'\\' || value == L'/';
}

std::wstring ParentPath(const std::wstring& path)
{
	const std::wstring::size_type separator = path.find_last_of(L"\\/");
	return separator == std::wstring::npos ? std::wstring() : path.substr(0, separator + 1);
}

std::wstring FileStem(const std::wstring& path)
{
	const std::wstring::size_type separator = path.find_last_of(L"\\/");
	const std::wstring fileName = separator == std::wstring::npos ? path : path.substr(separator + 1);
	const std::wstring::size_type extension = fileName.find_last_of(L'.');
	return extension == std::wstring::npos ? fileName : fileName.substr(0, extension);
}

bool NormalizeRelativeDirectory(const std::wstring& value, std::wstring& normalized)
{
	normalized.clear();

	std::wstring input(value);
	const std::wstring::size_type first = input.find_first_not_of(L" \t\r\n");
	if (first == std::wstring::npos) return false;
	const std::wstring::size_type last = input.find_last_not_of(L" \t\r\n");
	input = input.substr(first, last - first + 1);

	if (IsPathSeparator(input.front()) || input.find(L':') != std::wstring::npos) return false;

	std::wstring component;
	for (std::wstring::size_type index = 0; index <= input.size(); ++index) {
		if (index != input.size() && !IsPathSeparator(input[index])) {
			component += input[index];
			continue;
		}

		if (!component.empty()) {
			if (component == L".." || component.find_first_of(L"*?\"<>|") != std::wstring::npos) return false;
			if (component != L".") {
				if (!normalized.empty()) normalized += L'\\';
				normalized += component;
			}
			component.clear();
		}
	}

	return !normalized.empty();
}

}

namespace HtmlExportWriterHelpers {

bool BuildImagePaths(
	const std::wstring& outputHtmlPath,
	int externalImagesFolderMode,
	const std::wstring& externalImagesFolderName,
	ImagePaths& paths)
{
	paths = ImagePaths();

	std::wstring folderName;
	if (externalImagesFolderMode == 1 && !externalImagesFolderName.empty()) {
		if (!NormalizeRelativeDirectory(externalImagesFolderName, folderName)) return false;
	} else {
		folderName = FileStem(outputHtmlPath) + L"_files";
	}

	paths.directory = ParentPath(outputHtmlPath) + folderName;
	paths.imgPrefix = folderName;
	for (wchar_t& value : paths.imgPrefix) {
		if (value == L'\\') value = L'/';
	}
	if (paths.imgPrefix.empty() || paths.imgPrefix.back() != L'/') paths.imgPrefix += L'/';
	return true;
}

bool BuildMimePreamble(
	time_t timestamp,
	unsigned int randomValue,
	MimePreamble& preamble)
{
	preamble = MimePreamble();

	tm utc = {};
	if (gmtime_s(&utc, &timestamp) != 0) return false;

	char date[64] = {};
	if (strftime(date, _countof(date), "%a, %d %b %Y %H:%M:%S +0000", &utc) == 0) return false;

	char boundary[64] = {};
	const int boundaryLength = _snprintf_s(boundary, _countof(boundary), _TRUNCATE,
		"------NextPart---%016llX.%08X",
		static_cast<unsigned long long>(timestamp), randomValue);
	if (boundaryLength < 0) return false;

	char header[2048] = {};
	const int headerLength = _snprintf_s(header, _countof(header), _TRUNCATE,
		"From: <Saved by Haali ExportHTML Plugin>\r\n"
		"Date: %s\r\n"
		"MIME-Version: 1.0\r\n"
		"Content-Type: multipart/related; boundary=\"%s\"; type=\"text/html\"\r\n"
		"\r\n"
		"This is a multi-part message in MIME format.\r\n"
		"\r\n"
		"%s\r\n"
		"Content-Type: text/html; charset=\"utf-8\"\r\n"
		"Content-Transfer-Encoding: 8bit\r\n"
		"\r\n",
		date, boundary + 2, boundary);
	if (headerLength < 0) return false;

	preamble.boundary.assign(boundary, static_cast<size_t>(boundaryLength));
	preamble.header.assign(header, static_cast<size_t>(headerLength));
	return true;
}
}
