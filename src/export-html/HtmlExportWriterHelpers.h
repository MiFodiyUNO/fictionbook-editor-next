#pragma once

#include <ctime>
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

// Builds exactly one safe file path below the external-image directory.  FB2
// binary/@id is a filename here, never a relative path.

bool BuildExternalImagePath(
	const ImagePaths& paths,
	const std::wstring& binaryId,
	std::wstring& imagePath);

struct MimePreamble {
	std::string boundary;
	std::string header;
};

bool BuildMimePreamble(
	time_t timestamp,
	unsigned int randomValue,
	MimePreamble& preamble);
bool IsStandaloneWarningRequired(
	unsigned long long resultSizeBytes,
	unsigned long long standaloneWarningMiB);
}
