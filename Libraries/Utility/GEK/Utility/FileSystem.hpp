/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision$
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date$
#pragma once

#include "GEK/Utility/Exceptions.hpp"
#include "GEK/Utility/String.hpp"
#include <experimental\filesystem>
#include <functional>
#include <algorithm>
#include <vector>

namespace Gek
{
    namespace FileSystem
    {
        GEK_ADD_EXCEPTION(FileNotFound);
        GEK_ADD_EXCEPTION(FileReadError);
        GEK_ADD_EXCEPTION(FileWriteError);

        struct Path :
            public std::experimental::filesystem::path
        {
            Path(void);
            Path(const wchar_t *path);
            Path(const String &path);
            Path(const Path &path);

            void operator = (const wchar_t *path);
            void operator = (const String &path);
            void operator = (const Path &path);

            operator const wchar_t * (void) const;

            void removeFileName(void);
            void removeExtension(void);

            void replaceFileName(const wchar_t *fileName);
            void replaceExtension(const wchar_t *extension);

            Path withExtension(const wchar_t *extension = nullptr) const;
            Path withoutExtension() const;

            Path getParentPath(void) const;
            String getFileName(void) const;
            String getExtension(void) const;

            bool isFile(void) const;
            bool isDirectory(void) const;
            bool isNewerThan(const Path &path) const;
        };

        Path GetModuleFilePath(void);

        Path GetFileName(const Path &rootDirectory, const std::vector<String> &list);
        
        void MakeDirectoryChain(const Path &filePath);

        template <typename... PARAMETERS>
        Path GetFileName(const Path &rootDirectory, PARAMETERS... nameList)
        {
            return GetFileName(rootDirectory, { nameList... });
        }

        void Find(const Path &rootDirectory, std::function<bool(const Path &filePath)> onFileFound);

        void Load(const Path &fileName, std::vector<uint8_t> &buffer, std::uintmax_t limitReadSize = 0);
        void Load(const Path &fileName, StringUTF8 &string);
        void Load(const Path &fileName, String &string);

        void Save(const Path &fileName, const std::vector<uint8_t> &buffer);
        void Save(const Path &fileName, const StringUTF8 &string);
        void Save(const Path &fileName, const String &string);
    }; // namespace File
}; // namespace Gek

namespace std
{
    template<>
    struct hash<Gek::FileSystem::Path>
    {
        size_t operator()(const Gek::FileSystem::Path &value) const
        {
            return hash<wstring>()(value.native());
        }
    };
}; // namespace std