#include "stdafx.h"
#include "HtmlSplitExport.h"

#include <algorithm>
#include <cwctype>
#include <set>

namespace {
std::wstring ToLower(const std::wstring& text)
{
    std::wstring result(text);
    std::transform(result.begin(), result.end(), result.begin(), towlower);
    return result;
}
size_t FindInsensitive(const std::wstring& text, const std::wstring& needle, size_t start = 0)
{
    if (needle.empty() || start > text.size()) return std::wstring::npos;
    return ToLower(text).find(ToLower(needle), start);
}
bool TagHasNotesClass(const std::wstring& tag)
{
    const std::wstring lower = ToLower(tag);
    return lower.find(L"class=\"notes\"") != std::wstring::npos || lower.find(L"class='notes'") != std::wstring::npos;
}
std::wstring StripTags(const std::wstring& value)
{
    std::wstring result;
    bool inTag = false;
    for (wchar_t ch : value) {
        if (ch == L'<') inTag = true;
        else if (ch == L'>') inTag = false;
        else if (!inTag) result += ch;
    }
    while (!result.empty() && iswspace(result.front())) result.erase(result.begin());
    while (!result.empty() && iswspace(result.back())) result.pop_back();
    return result;
}
std::wstring SectionTitle(const std::wstring& html)
{
    for (int level = 1; level <= 6; ++level) {
        const std::wstring tag = L"<h" + std::to_wstring(level);
        const size_t open = FindInsensitive(html, tag);
        if (open == std::wstring::npos) continue;
        const size_t start = html.find(L'>', open);
        const size_t close = FindInsensitive(html, L"</h" + std::to_wstring(level) + L">", start);
        if (start != std::wstring::npos && close != std::wstring::npos) return StripTags(html.substr(start + 1, close - start - 1));
    }
    return std::wstring();
}
std::wstring SafeFileStem(const std::wstring& title)
{
    std::wstring stem;
    for (wchar_t ch : title) {
        if (ch < 32 || ch == L'\\' || ch == L'/' || ch == L':' || ch == L'*' || ch == L'?' || ch == L'\"' || ch == L'<' || ch == L'>' || ch == L'|') stem += L'_';
        else stem += ch;
    }
    while (!stem.empty() && (iswspace(stem.front()) || stem.front() == L'.')) stem.erase(stem.begin());
    while (!stem.empty() && (iswspace(stem.back()) || stem.back() == L'.')) stem.pop_back();
    if (stem.empty() || stem == L"." || stem == L"..") stem = L"section";
    if (stem.size() > 72) stem.resize(72);
    return stem;
}
void AddAnchors(const std::wstring& html, const std::wstring& fileName, std::map<std::wstring, std::wstring>& anchors)
{
    for (size_t pos = 0; pos < html.size();) {
        const size_t id = FindInsensitive(html, L"id=\"", pos);
        if (id == std::wstring::npos) break;
        const size_t first = id + 4;
        const size_t last = html.find(L'\"', first);
        if (last == std::wstring::npos) break;
        const std::wstring anchor = html.substr(first, last - first);
        if (!anchor.empty()) anchors[anchor] = fileName;
        pos = last + 1;
    }
}
void RewriteLocalLinks(std::wstring& html, const std::wstring& currentFile, const std::map<std::wstring, std::wstring>& anchors)
{
    for (size_t pos = 0; pos < html.size();) {
        const size_t href = FindInsensitive(html, L"href=\"#", pos);
        if (href == std::wstring::npos) break;
        const size_t first = href + 7;
        const size_t last = html.find(L'\"', first);
        if (last == std::wstring::npos) break;
        const std::wstring anchor = html.substr(first, last - first);
        const auto found = anchors.find(anchor);
        if (found != anchors.end() && found->second != currentFile) {
            const std::wstring replacement = found->second + L"#" + anchor;
            html.replace(first, last - first, replacement);
            pos = first + replacement.size() + 1;
        } else pos = last + 1;
    }
}
}
namespace HtmlSplitExport {
bool BuildPlan(const std::wstring& fullHtml, Plan& plan)
{
    plan = Plan();
    const size_t bodyStart = FindInsensitive(fullHtml, L"<body");
    if (bodyStart == std::wstring::npos) return false;
    const size_t bodyContent = fullHtml.find(L'>', bodyStart);
    const size_t bodyEnd = FindInsensitive(fullHtml, L"</body>", bodyContent);
    if (bodyContent == std::wstring::npos || bodyEnd == std::wstring::npos) return false;
    struct Range { size_t first; size_t last; };
    std::vector<Range> sections;
    int depth = 0;
    for (size_t pos = bodyContent + 1; pos < bodyEnd;) {
        const size_t open = FindInsensitive(fullHtml, L"<section", pos);
        const size_t close = FindInsensitive(fullHtml, L"</section", pos);
        if (open == std::wstring::npos && close == std::wstring::npos) break;
        if (open != std::wstring::npos && (close == std::wstring::npos || open < close)) {
            const size_t tagEnd = fullHtml.find(L'>', open);
            if (tagEnd == std::wstring::npos || tagEnd > bodyEnd) return false;
            if (depth == 0 && !TagHasNotesClass(fullHtml.substr(open, tagEnd - open + 1))) sections.push_back({ open, 0 });
            ++depth; pos = tagEnd + 1;
        } else {
            const size_t tagEnd = fullHtml.find(L'>', close);
            if (tagEnd == std::wstring::npos || depth <= 0) return false;
            --depth;
            if (depth == 0 && !sections.empty() && sections.back().last == 0) sections.back().last = tagEnd + 1;
            pos = tagEnd + 1;
        }
    }
    if (sections.empty() || depth != 0) return false;
    const std::wstring prefix = fullHtml.substr(0, sections.front().first);
    const std::wstring documentEnd = fullHtml.substr(bodyEnd);
    plan.indexHtml = prefix + fullHtml.substr(sections.back().last, bodyEnd - sections.back().last) + documentEnd;
    std::set<std::wstring> names;
    for (size_t index = 0; index < sections.size(); ++index) {
        const std::wstring fragment = fullHtml.substr(sections[index].first, sections[index].last - sections[index].first);
        const std::wstring stem = SafeFileStem(SectionTitle(fragment));
        std::wstring name = L"section-" + std::to_wstring(index + 1) + L"-" + stem + L".html";
        for (unsigned int suffix = 2; names.find(ToLower(name)) != names.end(); ++suffix) name = L"section-" + std::to_wstring(index + 1) + L"-" + stem + L"-" + std::to_wstring(suffix) + L".html";
        names.insert(ToLower(name));
        SectionDocument output; output.fileName = name; output.html = prefix + fragment + documentEnd;
        AddAnchors(output.html, output.fileName, plan.anchorFiles);
        plan.sections.push_back(output);
    }
    AddAnchors(plan.indexHtml, L"index.html", plan.anchorFiles);
    for (SectionDocument& section : plan.sections) RewriteLocalLinks(section.html, section.fileName, plan.anchorFiles);
    RewriteLocalLinks(plan.indexHtml, L"index.html", plan.anchorFiles);
    return true;
}
}