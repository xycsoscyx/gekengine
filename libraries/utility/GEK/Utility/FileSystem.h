#pragma once

#include "GEK\Utility\Exceptions.h"
#include "GEK\Utility\String.h"
#include <experimental\filesystem>
#include <functional>
#include <vector>

namespace Gek
{
    namespace FileSystem
    {
        GEK_START_EXCEPTIONS();
        GEK_ADD_EXCEPTION(FileNotFound);
        GEK_ADD_EXCEPTION(FileReadError);
        GEK_ADD_EXCEPTION(FileWriteError);

        class Path : public std::experimental::filesystem::path
        {
        public:
            Path(void);
            Path(const String &path);
            Path(const Path &path);

            Path &operator = (const String &path);
            Path &operator = (const Path &path);

            operator String () const;

            String getExtension(void) const;
            String getFileName(void) const;
            Path getPath(void) const;

            bool isFile(void) const;
            bool isDirectory(void) const;
        };

        String expandPath(const String &fileName);

        void find(const String &fileName, const String &filterTypes, bool searchRecursively, std::function<bool(const String &fileName)> onFileFound);

        void load(const String &fileName, std::vector<uint8_t> &buffer, std::size_t limitReadSize = 0);
        void load(const String &fileName, StringUTF8 &string);
        void load(const String &fileName, String &string);

        void save(const String &fileName, const std::vector<uint8_t> &buffer);
        void save(const String &fileName, const StringUTF8 &string);
        void save(const String &fileName, const String &string);
    }; // namespace File
}; // namespace Gek
