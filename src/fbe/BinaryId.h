#pragma once

#include <atlstr.h>
#include <windows.h>

// xs:ID is derived from XML NCName.  File-system names are often already
// valid IDs (including Unicode letters), so retain them verbatim.  For names
// that need normalization, replace only invalid characters and ensure an
// XML-name start character without transliterating the rest of the name.
namespace FbeBinary
{
	// Keep this UTF-16 code-unit subset exactly aligned with runtime/main.js.
	// JScript ES3 has no Unicode-category predicate equivalent to IsCharAlphaW;
	// an explicit common subset prevents one layer accepting an ID the other rejects.
	inline bool IsXmlIdUnicodeLetter(TCHAR value)
	{
		const UINT code = static_cast<UINT>(value);
		return (code >= L'A' && code <= L'Z') || (code >= L'a' && code <= L'z') ||
			(code >= 0x00c0 && code <= 0x02ff) || (code >= 0x0370 && code <= 0x1fff) ||
			(code >= 0x2070 && code <= 0x218f) || (code >= 0x2c00 && code <= 0x2fef) ||
			(code >= 0x3001 && code <= 0xd7ff) || (code >= 0xf900 && code <= 0xfdcf) ||
			(code >= 0xfdf0 && code <= 0xfffd);
	}

	inline bool IsXmlIdStart(TCHAR value)
	{
		return value == _T('_') || IsXmlIdUnicodeLetter(value);
	}

	inline bool IsXmlIdCharacter(TCHAR value)
	{
		return IsXmlIdStart(value) || (value >= _T('0') && value <= _T('9')) ||
			value == _T('-') || value == _T('.');
	}

	inline bool IsValidXmlId(const CString& value)
	{
		if (value.IsEmpty() || !IsXmlIdStart(value[0])) return false;
		for (int index = 1; index < value.GetLength(); ++index)
			if (!IsXmlIdCharacter(value[index])) return false;
		return true;
	}

	inline CString NormalizeXmlId(const CString& pathOrId)
	{
		int start = pathOrId.ReverseFind(_T('\\'));
		const int slash = pathOrId.ReverseFind(_T('/'));
		if (slash > start) start = slash;
		CString source = pathOrId.Mid(start < 0 ? 0 : start + 1);
		if (source.IsEmpty()) return CString(L"image");

		bool valid = IsXmlIdStart(source[0]);
		for (int index = 1; valid && index < source.GetLength(); ++index)
			valid = IsXmlIdCharacter(source[index]);
		if (valid) return source;

		CString normalized;
		for (int index = 0; index < source.GetLength(); ++index)
			normalized.AppendChar(IsXmlIdCharacter(source[index]) ? source[index] : _T('_'));
		if (normalized.IsEmpty()) normalized = L"image";
		if (!IsXmlIdStart(normalized[0])) normalized.Insert(0, _T('_'));
		return normalized;
	}
}
