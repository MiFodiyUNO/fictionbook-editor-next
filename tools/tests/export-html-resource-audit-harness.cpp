#include "../../src/export-html/HtmlExportResourceAudit.h"

#include <iostream>
#include <string>
#include <vector>

static int ExpectCount(const std::wstring& html, size_t expected, const char* name)
{
	const std::vector<std::wstring> dependencies = HtmlExportResourceAudit::FindExternalDependencies(html);
	if (dependencies.size() != expected) {
		std::cerr << name << ": expected " << expected << ", got " << dependencies.size() << std::endl;
		return 1;
	}
	return 0;
}

int main()
{
	int failures = 0;
	failures += ExpectCount(L"<img src=\"data:image/png;base64,AA==\"><a href=\"https://example.test\">link</a>", 0, "allowed data and hyperlink");
	failures += ExpectCount(L"<img src=\"cover.png\"><source src=\"https://cdn.test/a.mp4\"><video poster=\"poster.jpg\"><link href=\"theme.css\">", 4, "HTML resource attributes");
	failures += ExpectCount(L"<style>a{background:url(data:image/png;base64,AA==)} b{background:url('image.png')} @import url(https://example.test/theme.css); @import 'print.css';</style>", 3, "CSS urls and imports");
	failures += ExpectCount(L"<img src=\"#embedded\"><source src=\"mailto:author@example.test\"><style>a{background:url(#paint)}</style>", 0, "allowed anchors and mailto");
	return failures ? 1 : 0;
}
