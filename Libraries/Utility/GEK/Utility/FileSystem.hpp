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
            Path(WString const &path);
            Path(Path const &path);

            void operator = (WString const &path);
            void operator = (Path const &path);

            operator wchar_t const * const (void) const;

            void removeFileName(void);
            void removeExtension(void);

            void replaceFileName(WString const &fileName);
            void replaceExtension(WString const &extension);

            Path withExtension(WString const &extension) const;
            Path withoutExtension() const;

            Path getParentPath(void) const;
            WString getFileName(void) const;
            WString getExtension(void) const;

            bool isFile(void) const;
            bool isDirectory(void) const;
            bool isNewerThan(Path const &path) const;
        };

        Path GetModuleFilePath(void);

        Path GetFileName(Path const &rootDirectory, const std::vector<WString> &list);
        
        void MakeDirectoryChain(Path const &filePath);

        template <typename... PARAMETERS>
        Path GetFileName(Path const &rootDirectory, PARAMETERS... nameList)
        {
            return GetFileName(rootDirectory, { nameList... });
        }

        void Find(Path const &rootDirectory, std::function<bool(Path const &filePath)> onFileFound);

        void Load(Path const &fileName, std::vector<uint8_t> &buffer, std::uintmax_t limitReadSize = 0);
        void Load(Path const &fileName, CString &string);
        void Load(Path const &fileName, WString &string);

        void Save(Path const &fileName, std::vector<uint8_t> const &buffer);
        void Save(Path const &fileName, CString const &string);
        void Save(Path const &fileName, WString const &string);
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