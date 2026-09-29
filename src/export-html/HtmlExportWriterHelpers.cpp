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

}
