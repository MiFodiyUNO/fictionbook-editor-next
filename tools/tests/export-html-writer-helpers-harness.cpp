#include "../../src/export-html/HtmlExportWriterHelpers.h"

#include <iostream>
#include <string>

namespace {

int ExpectPaths(const std::wstring& outputPath, int mode, const std::wstring& folderName,
	const std::wstring& expectedDirectory, const std::wstring& expectedPrefix, const char* name)
{
	HtmlExportWriterHelpers::ImagePaths paths;
	if (!HtmlExportWriterHelpers::BuildImagePaths(outputPath, mode, folderName, paths)) {
		std::cerr << name << ": rejected" << std::endl;
		return 1;
	}
	if (paths.directory != expectedDirectory || paths.imgPrefix != expectedPrefix) {
		std::cerr << name << ": unexpected paths" << std::endl;
		return 1;
	}
	return 0;
}

int ExpectRejected(const std::wstring& folderName, const char* name)
{
	HtmlExportWriterHelpers::ImagePaths paths;
	if (HtmlExportWriterHelpers::BuildImagePaths(L"C:\\export\\book.html", 1, folderName, paths)) {
		std::cerr << name << ": accepted unsafe path" << std::endl;
		return 1;
	}
	return 0;
}

}

int main()
{
	int failures = 0;
	failures += ExpectPaths(L"book.html", 0, L"", L"book_files", L"book_files/", "automatic folder");
	failures += ExpectPaths(L"C:\\export\\book.html", 1, L"images", L"C:\\export\\images", L"images/", "custom folder");
	failures += ExpectPaths(L"C:\\export\\book.html", 1, L"Иллюстрации", L"C:\\export\\Иллюстрации", L"Иллюстрации/", "unicode folder");
	failures += ExpectPaths(L"C:\\export\\book.html", 1, L"images\\\\", L"C:\\export\\images", L"images/", "duplicate separators");
	failures += ExpectRejected(L"..\\images", "parent traversal");
	failures += ExpectRejected(L"C:\\images", "absolute path");
	return failures == 0 ? 0 : 1;
}
