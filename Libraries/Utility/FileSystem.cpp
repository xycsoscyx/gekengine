#include "GEK\Utility\FileSystem.hpp"
#include <experimental\filesystem>
#include <fstream>
#include <Windows.h>

namespace Gek
{
	namespace FileSystem
	{
		String GetFileName(const wchar_t *rootDirectory, const std::vector<String> &list)
		{
            String fileName(rootDirectory);
            fileName.join(list, std::experimental::filesystem::path::preferred_separator);
            return fileName;
		}

		String ReplaceExtension(const wchar_t *fileName, const wchar_t *extension)
		{
			std::experimental::filesystem::path filePath(fileName);
			filePath.replace_extension(extension ? extension : L"");
			return filePath.wstring();
		}

		String GetExtension(const wchar_t *fileName)
        {
            std::experimental::filesystem::path filePath(fileName);
            return filePath.extension().wstring();
        }

		String GetFileName(const wchar_t *fileName)
		{
			std::experimental::filesystem::path filePath(fileName);
			return filePath.filename().wstring();
		}

		String GetDirectory(const wchar_t *fileName)
        {
            std::experimental::filesystem::path filePath(fileName);
            return filePath.parent_path().wstring();
        }

        bool IsFile(const wchar_t *fileName)
        {
            return std::experimental::filesystem::is_regular_file(fileName);
        }

        bool IsDirectory(const wchar_t *fileName)
        {
            return std::experimental::filesystem::is_directory(fileName);
        }

		bool IsFileNewer(const wchar_t *newFile, const wchar_t *oldFile)
		{
			auto newFileTime = std::experimental::filesystem::last_write_time(newFile);
			auto oldFileTime = std::experimental::filesystem::last_write_time(oldFile);
			return (newFileTime > oldFileTime);
		}

		void Find(const wchar_t *rootDirectory, std::function<bool(const wchar_t *)> onFileFound)
		{
			for (auto &fileSearch : std::experimental::filesystem::directory_iterator(rootDirectory))
			{
				onFileFound(fileSearch.path().wstring().c_str());
			}
		}

		void Load(const wchar_t *fileName, std::vector<uint8_t> &buffer, std::uintmax_t limitReadSize)
		{
			HANDLE file = CreateFile(fileName, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
			if(file == INVALID_HANDLE_VALUE)
            {
                throw FileNotFound("Unable to open file for reading");
            }

			buffer.resize(GetFileSize(file, nullptr));

			DWORD bytesRead = 0;
			ReadFile(file, buffer.data(), DWORD(buffer.size()), &bytesRead, nullptr);
			CloseHandle(file);

			if (bytesRead != buffer.size())
			{
				throw FileReadError("Unable to read file contents");
			}
		}

		void Load(const wchar_t *fileName, StringUTF8 &string)
		{
			std::vector<uint8_t> buffer;
			Load(fileName, buffer);
			buffer.push_back('\0');
			string = reinterpret_cast<char *>(buffer.data());
		}

		void Load(const wchar_t *fileName, String &string)
		{
			std::vector<uint8_t> buffer;
			Load(fileName, buffer);
			buffer.push_back('\0');
			string = reinterpret_cast<char *>(buffer.data());
		}

        void CreateDirectory(const wchar_t *fileName)
        {
            auto directory = GetDirectory(fileName);
            if (!directory.empty())
            {
                CreateDirectory(directory);
                ::CreateDirectory(directory, nullptr);
            }
        }

        void Save(const wchar_t *fileName, const std::vector<uint8_t> &buffer)
        {
            CreateDirectory(fileName);
            HANDLE file = CreateFile(fileName, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
			if (file == INVALID_HANDLE_VALUE)
			{
				throw FileNotFound("Unable to open file for writing");
			}

			DWORD bytesWritten = 0;
			WriteFile(file, buffer.data(), DWORD(buffer.size()), &bytesWritten, nullptr);
			CloseHandle(file);

			if (bytesWritten != buffer.size())
			{
				throw FileWriteError("Unable to write contents to file");
			}
        }

        void Save(const wchar_t *fileName, const StringUTF8 &string)
        {
            std::vector<uint8_t> buffer(string.length());
            std::copy(std::begin(string), std::end(string), std::begin(buffer));
            Save(fileName, buffer);
        }

        void Save(const wchar_t *fileName, const String &string)
        {
            Save(fileName, StringUTF8(string));
        }
    } // namespace FileSystem
}; // namespace Gek
