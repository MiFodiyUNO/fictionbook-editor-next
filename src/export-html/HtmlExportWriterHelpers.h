#pragma once

#include <string>

namespace HtmlExportWriterHelpers {

// Pure path calculation for the external-image export mode.  It deliberately
// does not create or remove files/directories and has no writer ownership.
struct ImagePaths {
	std::wstring directory;
	std::wstring imgPrefix;
};

bool BuildImagePaths(
	const std::wstring& outputHtmlPath,
	int externalImagesFolderMode,
	const std::wstring& externalImagesFolderName,
	ImagePaths& paths);

}
