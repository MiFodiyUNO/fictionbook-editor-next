#pragma once

#include <string>
#include <vector>

namespace HtmlExportResourceAudit {

// Returns resource URLs which would require a file or network location beside
// a supposedly self-contained HTML document.  Ordinary hyperlinks are not
// inspected: only elements which load a resource and CSS resource directives.
std::vector<std::wstring> FindExternalDependencies(const std::wstring& html);

}
