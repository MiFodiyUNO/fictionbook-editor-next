#include <windows.h>
#include <atlstr.h>
#include <cstdio>

#include "BinaryId.h"

int wmain(int argc, wchar_t* argv[])
{
    if (argc != 2)
        return 2;
    ::wprintf(L"%d\n", FbeBinary::IsValidXmlId(CString(argv[1])) ? 1 : 0);
    return 0;
}