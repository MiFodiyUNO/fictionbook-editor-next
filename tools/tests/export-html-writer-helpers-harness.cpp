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

int ExpectMimePreamble()
{
	HtmlExportWriterHelpers::MimePreamble preamble;
	if (!HtmlExportWriterHelpers::BuildMimePreamble(0, 0x1A2B3C4D, preamble)) {
		std::cerr << "MIME preamble: rejected" << std::endl;
		return 1;
	}

	const std::string expectedBoundary = "------NextPart---0000000000000000.1A2B3C4D";
	if (preamble.boundary != expectedBoundary) {
		std::cerr << "MIME boundary format" << std::endl;
		return 1;
	}

	const std::string& header = preamble.header;
	const std::string expectedDate = "Date: Thu, 01 Jan 1970 00:00:00 +0000\r\n";
	const std::string expectedContentType = "Content-Type: multipart/related; boundary=\"----NextPart---0000000000000000.1A2B3C4D\"; type=\"text/html\"\r\n";
	if (header.find(expectedDate) == std::string::npos ||
		header.find(expectedContentType) == std::string::npos ||
		header.find("Content-Type: text/html; charset=\"utf-8\"\r\n") == std::string::npos) {
		std::cerr << "MIME header fields" << std::endl;
		return 1;
	}
	for (size_t index = 0; index < header.size(); ++index) {
		if (header[index] == '\n' && (index == 0 || header[index - 1] != '\r')) {
			std::cerr << "MIME CRLF" << std::endl;
			return 1;
		}
	}
	return header.size() >= 2 && header.compare(header.size() - 2, 2, "\r\n") == 0 ? 0 : 1;
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
	failures += ExpectMimePreamble();

	return failures == 0 ? 0 : 1;
}
