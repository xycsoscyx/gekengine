#pragma once

#include "GEK\Utility\Trace.h"
#include "GEK\Utility\String.h"
#include <experimental\filesystem>
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

        class Path : public std::experimental::filesystem::path
        {
        public:
            Path(void);
            Path(const char *path);
            Path(const wchar_t *path);

            Path &operator = (const char *path);
            Path &operator = (const wchar_t *path);
            Path &operator = (const Path &path);

            operator const wchar_t * () const;
            operator String () const;

            String getExtension(void) const;
            String getFileName(void) const;
            Path getPath(void) const;

            bool isFile(void) const;
            bool isDirectory(void) const;
        };

        String expandPath(const wchar_t *fileName);

        void find(const wchar_t *fileName, const wchar_t *filterTypes, bool searchRecursively, std::function<bool(const wchar_t *fileName)> onFileFound);

        void load(const wchar_t *fileName, std::vector<UINT8> &buffer, size_t limitReadSize = 0);
        void load(const wchar_t *fileName, StringUTF8 &string);
        void load(const wchar_t *fileName, String &string);

        void save(const wchar_t *fileName, const std::vector<UINT8> &buffer);
        void save(const wchar_t *fileName, const StringUTF8 &string);
        void save(const wchar_t *fileName, const String &string);
    }; // namespace File
}; // namespace Gek
