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

        wstring expandPath(const wstring &fileName);

        void find(const wstring &fileName, const wstring &filterTypes, bool searchRecursively, std::function<bool(const wstring &fileName)> onFileFound);

        HMODULE loadLibrary(const wstring &fileName);

        void load(const wstring &fileName, std::vector<UINT8> &buffer, size_t limitReadSize = 0);
        void load(const wstring &fileName, string &string);
        void load(const wstring &fileName, wstring &string, bool convertUTF8 = true);

        void save(const wstring &fileName, const std::vector<UINT8> &buffer);
        void save(const wstring &fileName, const string &string);
        void save(const wstring &fileName, const wstring &string, bool convertUTF8 = true);
    }; // namespace File
}; // namespace Gek
