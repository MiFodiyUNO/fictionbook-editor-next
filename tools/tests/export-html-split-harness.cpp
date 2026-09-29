#include "HtmlSplitExport.h"
#include <iostream>

int main()
{
    const std::wstring html =
        L"<html><body><h4>Table of contents</h4><a href=\"#_toc_a\">A</a><a href=\"#_toc_b\">B</a>"
        L"<section><a id=\"_toc_a\"></a><h2>Глава / one &amp; # %</h2><p><a href=\"#target-b\">next</a></p><a id=\"target-a\"></a></section>"
        L"<section><a id=\"_toc_b\"></a><h2>Глава / one &amp; # %</h2><p><a id=\"target-b\"></a><a href=\"#target-a\">back</a></p></section>"
        L"<section class=\"notes\"><a id=\"note-end\"></a><a href=\"#target-a\">backlink</a></section></body></html>";
    HtmlSplitExport::Plan plan;
    if (!HtmlSplitExport::BuildPlan(html, plan) || plan.sections.size() != 2) return 1;
    if (plan.sections[0].fileName.find(L"/") != std::wstring::npos || plan.sections[0].fileName.find(L"\\") != std::wstring::npos) return 2;
    if (plan.sections[0].fileName == plan.sections[1].fileName) return 3;
    if (plan.sections[0].fileName.find(L"section-1-") != 0 || plan.sections[1].fileName.find(L"section-2-") != 0) return 11;
    if (plan.sections[0].fileName.find(L"Глава") == std::wstring::npos) return 4;
    if (plan.sections[0].fileName.find(L"#") != std::wstring::npos || plan.sections[0].fileName.find(L"%") != std::wstring::npos || plan.sections[0].fileName.find(L"&") != std::wstring::npos || plan.sections[0].fileName.find(L"amp;") != std::wstring::npos) return 10;
    if (plan.indexHtml.find(plan.sections[0].fileName + L"#_toc_a") == std::wstring::npos) return 5;
    if (plan.sections[0].html.find(plan.sections[1].fileName + L"#target-b") == std::wstring::npos) return 6;
    if (plan.sections[1].html.find(plan.sections[0].fileName + L"#target-a") == std::wstring::npos) return 7;
    if (plan.indexHtml.find(L"class=\"notes\"") == std::wstring::npos || plan.sections[0].html.find(L"class=\"notes\"") != std::wstring::npos) return 8;
    if (plan.anchorFiles.find(L"target-b") == plan.anchorFiles.end()) return 9;
    std::wcout << L"ExportHTML split harness passed.\n";
    return 0;
}