#pragma once

#include <map>
#include <string>
#include <vector>

namespace HtmlSplitExport {
struct SectionDocument {
    std::wstring fileName;
    std::wstring html;
};
struct Plan {
    std::wstring indexHtml;
    std::vector<SectionDocument> sections;
    std::map<std::wstring, std::wstring> anchorFiles;
};
// Plans split output from the canonical HTML emitted by html.xsl.
bool BuildPlan(const std::wstring& fullHtml, Plan& plan);
}