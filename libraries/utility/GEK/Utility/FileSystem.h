#pragma once

#include <Windows.h>
#include <atlbase.h>
#include <atlstr.h>
#include <functional>
#include <vector>

namespace Gek
{
    namespace FileSystem
    {
        CStringW expandPath(LPCWSTR basePath);

        HRESULT find(LPCWSTR basePath, LPCWSTR filterTypes, bool searchRecursively, std::function<HRESULT(LPCWSTR fileName)> onFileFound);

        HMODULE loadLibrary(LPCWSTR fileName);

        HRESULT load(LPCWSTR fileName, std::vector<UINT8> &buffer, size_t limitReadSize = -1);
        HRESULT load(LPCWSTR fileName, CStringA &string);
        HRESULT load(LPCWSTR fileName, CStringW &string, bool convertUTF8 = true);

        HRESULT save(LPCWSTR fileName, const std::vector<UINT8> &buffer);
        HRESULT save(LPCWSTR fileName, LPCSTR pString);
        HRESULT save(LPCWSTR fileName, LPCWSTR pString, bool convertUTF8 = true);
    }; // namespace File
}; // namespace Gek
