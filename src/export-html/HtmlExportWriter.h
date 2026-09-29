#pragma once

#include "HtmlExportWriterHelpers.h"

#include <atlbase.h>
#include <functional>
#include <string>
#include <vector>

struct IStream;
struct IXMLDOMDocument2;

namespace HtmlExportWriter {

enum class Failure {
	OpenTarget,
	CreateImagesDirectory,
	WriteTarget,
	ShortWrite,
	OpenImage,
	WriteImage,
	ShortImageWrite
};

struct Callbacks {
	std::function<void(Failure failure, const std::wstring& path, DWORD error)> reportFailure;
	std::function<bool(const std::wstring& path)> confirmImageOverwrite;
};

struct Options {
	std::wstring targetPath;
	bool mime = false;
	bool externalImages = false;
	bool standalone = false;
	int externalImagesFolderMode = 0;
	std::wstring externalImagesFolderName;
};

class Writer {
public:
	Writer(const Options& options, const Callbacks& callbacks);
	~Writer();

	HRESULT Prepare();
	const HtmlExportWriterHelpers::ImagePaths& ImagePaths() const;
	HRESULT GetTransformOutput(IStream** output);
	IStream* StandaloneOutput() const;
	HRESULT WriteStandaloneToTarget();
	HRESULT WriteImages(IXMLDOMDocument2* source);
	HRESULT Finalize();
	void Commit();

private:
	HRESULT OpenTarget();
	HRESULT WriteTargetBytes(const void* bytes, DWORD length);
	HRESULT WriteMimePreamble();
	HRESULT WriteMimeFinalBoundary();
	HRESULT CloseTarget();
	void Abort();
	void Report(Failure failure, const std::wstring& path, DWORD error) const;

	Options m_options;
	Callbacks m_callbacks;
	HtmlExportWriterHelpers::ImagePaths m_imagePaths;
	HANDLE m_target;
	CComPtr<IStream> m_transformOutput;
	CComPtr<IStream> m_standaloneOutput;
	std::string m_mimeBoundary;
	std::vector<std::wstring> m_createdImages;
	bool m_createdImagesDirectory;
	bool m_targetOpened;
	bool m_prepared;
	bool m_committed;
};

}