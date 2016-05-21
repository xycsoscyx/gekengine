#pragma once

#include "GEK\Utility\Trace.h"
#include <functional>
#include <vector>

namespace Gek
{
    namespace FileSystem
    {
        GEK_BASE_EXCEPTION();
        GEK_EXCEPTION(FileNotFound);
        GEK_EXCEPTION(FileReadError);
        GEK_EXCEPTION(FileWriteError);

        CStringW expandPath(LPCWSTR fileName);

        void find(LPCWSTR fileName, LPCWSTR filterTypes, bool searchRecursively, std::function<bool(LPCWSTR fileName)> onFileFound);

        HMODULE loadLibrary(LPCWSTR fileName);

        void load(LPCWSTR fileName, std::vector<UINT8> &buffer, size_t limitReadSize = 0);
        void load(LPCWSTR fileName, CStringA &string);
        void load(LPCWSTR fileName, CStringW &string, bool convertUTF8 = true);

        void save(LPCWSTR fileName, const std::vector<UINT8> &buffer);
        void save(LPCWSTR fileName, LPCSTR pString);
        void save(LPCWSTR fileName, LPCWSTR pString, bool convertUTF8 = true);
    }; // namespace File
}; // namespace Gek
