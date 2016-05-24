#pragma once

#include "GEK\Utility\Trace.h"
#include "GEK\Utility\String.h"
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

        wstring expandPath(const wchar_t *fileName);

        void find(const wchar_t *fileName, const wchar_t *filterTypes, bool searchRecursively, std::function<bool(const wchar_t *fileName)> onFileFound);

        HMODULE loadLibrary(const wchar_t *fileName);

        void load(const wchar_t *fileName, std::vector<UINT8> &buffer, size_t limitReadSize = 0);
        void load(const wchar_t *fileName, string &string);
        void load(const wchar_t *fileName, wstring &string, bool convertUTF8 = true);

        void save(const wchar_t *fileName, const std::vector<UINT8> &buffer);
        void save(const wchar_t *fileName, const char *pString);
        void save(const wchar_t *fileName, const wchar_t *pString, bool convertUTF8 = true);
    }; // namespace File
}; // namespace Gek
