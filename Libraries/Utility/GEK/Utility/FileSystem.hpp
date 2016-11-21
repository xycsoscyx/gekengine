/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision$
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date$
#pragma once

#include "GEK\Utility\Exceptions.hpp"
#include "GEK\Utility\String.hpp"
#include <functional>
#include <vector>

namespace Gek
{
    namespace FileSystem
    {
        GEK_ADD_EXCEPTION(FileNotFound);
        GEK_ADD_EXCEPTION(FileReadError);
        GEK_ADD_EXCEPTION(FileWriteError);

		String GetFileName(const wchar_t *rootDirectory, const std::vector<String> &list);

        template <typename... PARAMETERS>
        String GetFileName(const wchar_t *rootDirectory, PARAMETERS... nameList)
        {
            return GetFileName(rootDirectory, { nameList... });
        }

		String ReplaceExtension(const wchar_t *fileName, const wchar_t *extension = nullptr);
		String GetExtension(const wchar_t *fileName);
        String GetFileName(const wchar_t *fileName);
        String GetDirectory(const wchar_t *fileName);

        bool IsFile(const wchar_t *fileName);
        bool IsDirectory(const wchar_t *fileName);

		bool IsFileNewer(const wchar_t *newFile, const wchar_t *oldFile);

        void Find(const wchar_t *rootDirectory, std::function<bool(const wchar_t *fileName)> onFileFound);

        void Load(const wchar_t *fileName, std::vector<uint8_t> &buffer, std::uintmax_t limitReadSize = 0);
        void Load(const wchar_t *fileName, StringUTF8 &string);
        void Load(const wchar_t *fileName, String &string);

        void Save(const wchar_t *fileName, const std::vector<uint8_t> &buffer);
        void Save(const wchar_t *fileName, const StringUTF8 &string);
        void Save(const wchar_t *fileName, const String &string);
    }; // namespace File
}; // namespace Gek
