#include "stdafx.h"
#include "HtmlExportResourceAudit.h"

#include <algorithm>
#include <cwctype>
#include <regex>

namespace {

std::wstring Trim(const std::wstring& value)
{
	std::wstring::size_type first = 0;
	while (first < value.size() && iswspace(value[first])) ++first;
	std::wstring::size_type last = value.size();
	while (last > first && iswspace(value[last - 1])) --last;
	return value.substr(first, last - first);
}

bool IsAllowedUri(const std::wstring& value)
{
	const std::wstring uri = Trim(value);
	if (uri.empty()) return true;
	std::wstring lower(uri);
	std::transform(lower.begin(), lower.end(), lower.begin(), towlower);
	return lower.compare(0, 5, L"data:") == 0 || uri[0] == L'#' ||
		lower.compare(0, 7, L"mailto:") == 0;
}

void AddMatches(const std::wstring& html, const std::wregex& expression,
	std::vector<std::wstring>& dependencies)
{
	for (std::wsregex_iterator it(html.begin(), html.end(), expression), end; it != end; ++it) {
		const std::wstring value = (*it)[1].str();
		if (!IsAllowedUri(value)) dependencies.push_back(Trim(value));
	}
}

}

namespace HtmlExportResourceAudit {

std::vector<std::wstring> FindExternalDependencies(const std::wstring& html)
{
	std::vector<std::wstring> dependencies;
	// The output is HTML, not guaranteed XML, so audit the completed serialized
	// bytes rather than relying on an XML parser accepting browser-valid markup.
	const std::wregex mediaAttribute(
		L"<(?:img|source)\\b[^>]*\\bsrc\\s*=\\s*['\\\"]([^'\\\"]*)['\\\"]",
		std::regex_constants::icase);
	const std::wregex posterAttribute(
		L"<video\\b[^>]*\\bposter\\s*=\\s*['\\\"]([^'\\\"]*)['\\\"]",
		std::regex_constants::icase);
	const std::wregex stylesheetAttribute(
		L"<link\\b[^>]*\\bhref\\s*=\\s*['\\\"]([^'\\\"]*)['\\\"]",
		std::regex_constants::icase);
	const std::wregex cssUrl(L"url\\s*\\(\\s*['\\\"]?([^'\\\")\\s][^'\\\")]*?)['\\\"]?\\s*\\)",
		std::regex_constants::icase);
	const std::wregex cssImport(L"@import\\s+['\\\"]([^'\\\"]*)['\\\"]",
		std::regex_constants::icase);

	AddMatches(html, mediaAttribute, dependencies);
	AddMatches(html, posterAttribute, dependencies);
	AddMatches(html, stylesheetAttribute, dependencies);
	AddMatches(html, cssUrl, dependencies);
	AddMatches(html, cssImport, dependencies);
	return dependencies;
}

}
