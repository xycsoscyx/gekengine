#include "GEK\Utility\FileSystem.h"
#include <experimental\filesystem>
#include <fstream>
#include <Windows.h>

namespace Gek
{
	namespace FileSystem
	{
		String getFileName(const wchar_t *rootDirectory, const std::vector<String> &list)
		{
            String fileName(rootDirectory);
            fileName.join(list, std::experimental::filesystem::path::preferred_separator);
            return fileName;
		}

		String replaceExtension(const wchar_t *fileName, const wchar_t *extension)
		{
			std::experimental::filesystem::path filePath(fileName);
			filePath.replace_extension(extension ? extension : L"");
			return filePath.wstring();
		}

		String getExtension(const wchar_t *fileName)
        {
            std::experimental::filesystem::path filePath(fileName);
            return filePath.extension().wstring();
        }

		String getFileName(const wchar_t *fileName)
		{
			std::experimental::filesystem::path filePath(fileName);
			return filePath.filename().wstring();
		}

		String getDirectory(const wchar_t *fileName)
        {
            std::experimental::filesystem::path filePath(fileName);
            return filePath.parent_path().wstring();
        }

        bool isFile(const wchar_t *fileName)
        {
            return std::experimental::filesystem::is_regular_file(fileName);
        }

        bool isDirectory(const wchar_t *fileName)
        {
            return std::experimental::filesystem::is_directory(fileName);
        }

		bool isFileNewer(const wchar_t *newFile, const wchar_t *oldFile)
		{
			auto newFileTime = std::experimental::filesystem::last_write_time(newFile);
			auto oldFileTime = std::experimental::filesystem::last_write_time(oldFile);
			return (newFileTime > oldFileTime);
		}

		void find(const wchar_t *rootDirectory, std::function<bool(const wchar_t *)> onFileFound)
		{
			for (auto &fileSearch : std::experimental::filesystem::directory_iterator(rootDirectory))
			{
				onFileFound(fileSearch.path().wstring().c_str());
			}
		}

		void load(const wchar_t *fileName, std::vector<uint8_t> &buffer, std::uintmax_t limitReadSize)
		{
			HANDLE file = CreateFile(fileName, GENERIC_READ, 0, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
			if(file == INVALID_HANDLE_VALUE)
            {
                throw FileNotFound();
            }


			buffer.resize(GetFileSize(file, nullptr));

			DWORD bytesRead = 0;
			ReadFile(file, buffer.data(), buffer.size(), &bytesRead, nullptr);
			CloseHandle(file);

			if (bytesRead != buffer.size())
			{
				throw FileReadError();
			}
		}

		void load(const wchar_t *fileName, StringUTF8 &string)
		{
			std::vector<uint8_t> buffer;
			load(fileName, buffer);
			buffer.push_back('\0');
			string = reinterpret_cast<char *>(buffer.data());
		}

		void load(const wchar_t *fileName, String &string)
		{
			std::vector<uint8_t> buffer;
			load(fileName, buffer);
			buffer.push_back('\0');
			string = reinterpret_cast<char *>(buffer.data());
		}

        void save(const wchar_t *fileName, const std::vector<uint8_t> &buffer)
        {
			HANDLE file = CreateFile(fileName, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
			if (file == INVALID_HANDLE_VALUE)
			{
				throw FileNotFound();
			}

			DWORD bytesWritten = 0;
			WriteFile(file, buffer.data(), buffer.size(), &bytesWritten, nullptr);
			CloseHandle(file);

			if (bytesWritten != buffer.size())
			{
				throw FileWriteError();
			}
        }

        void save(const wchar_t *fileName, const StringUTF8 &string)
        {
            std::vector<uint8_t> buffer(string.length());
            std::copy(string.begin(), string.end(), buffer.begin());
            save(fileName, buffer);
        }

        void save(const wchar_t *fileName, const String &string)
        {
            save(fileName, StringUTF8(string));
        }
    } // namespace FileSystem
}; // namespace Gek
