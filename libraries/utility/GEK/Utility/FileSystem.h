#pragma once

#include "GEK\Utility\Exceptions.h"
#include "GEK\Utility\String.h"
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

		String getRoot(void);

		String getFileName(const wchar_t *rootDirectory, const std::initializer_list<String> &list);

		template <typename... PARAMETERS>
		String getFileName(const wchar_t *rootDirectory, PARAMETERS... nameList)
		{
			return getFileName(rootDirectory, { nameList... });
		}

		template <typename... PARAMETERS>
		String getRootFileName(PARAMETERS... nameList)
		{
			return getFileName(getRoot(), { nameList... });
		}

		String replaceExtension(const wchar_t *fileName, const wchar_t *extension = nullptr);
		String getExtension(const wchar_t *fileName);
        String getFileName(const wchar_t *fileName);
        String getDirectory(const wchar_t *fileName);

        bool isFile(const wchar_t *fileName);
        bool isDirectory(const wchar_t *fileName);

        void find(const wchar_t *rootDirectory, bool searchRecursively, std::function<bool(const wchar_t *fileName)> onFileFound);

        void load(const wchar_t *fileName, std::vector<uint8_t> &buffer, std::size_t limitReadSize = 0);
        void load(const wchar_t *fileName, StringUTF8 &string);
        void load(const wchar_t *fileName, String &string);

        void save(const wchar_t *fileName, const std::vector<uint8_t> &buffer);
        void save(const wchar_t *fileName, const StringUTF8 &string);
        void save(const wchar_t *fileName, const wchar_t *string);
    }; // namespace File
}; // namespace Gek
