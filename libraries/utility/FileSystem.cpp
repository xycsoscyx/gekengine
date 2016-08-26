#include "GEK\Utility\FileSystem.h"
#include <experimental\filesystem>
#include <Windows.h>

namespace Gek
{
	namespace FileSystem
	{
		String getFileName(const wchar_t *rootDirectory, const std::initializer_list<String> &list)
		{
			std::experimental::filesystem::path fileName(rootDirectory);
			for (const auto &name : list)
			{
                fileName.append(name.begin(), name.end());
			}

			return fileName.wstring();
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

		void find(const wchar_t *rootDirectory, bool searchRecursively, std::function<bool(const wchar_t *)> onFileFound)
		{
			for (auto &fileSearch : std::experimental::filesystem::directory_iterator(rootDirectory))
			{
				String fileName(fileSearch.path().wstring());
				if (isFile(fileName))
				{
					onFileFound(fileName);
				}
				else if (searchRecursively && isDirectory(fileName))
				{
					find(fileName, searchRecursively, onFileFound);
				}
			}
		}

		void load(const wchar_t *fileName, std::vector<uint8_t> &buffer, std::uintmax_t limitReadSize)
		{
            std::experimental::filesystem::path filePath(fileName);
            if (!std::experimental::filesystem::is_regular_file(filePath))
            {
                throw FileNotFound();
            }

            auto size = std::experimental::filesystem::file_size(filePath);
            if (size > 0)
            {
                HANDLE fileHandle = CreateFile(fileName, GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
                if (fileHandle == INVALID_HANDLE_VALUE)
                {
                    throw FileNotFound();
                }

                size = (limitReadSize > 0 ? std::min(size, limitReadSize) : size);
                buffer.resize(size);
                auto bufferData = buffer.data();

                static const uint32_t chunkSize = 65536;
                while (size >= chunkSize)
                {
                    DWORD bytesRead = 0;
                    BOOL success = ReadFile(fileHandle, bufferData, chunkSize, &bytesRead, nullptr);
                    if (!success || bytesRead != chunkSize)
                    {
                        throw FileReadError();
                    }

                    bufferData += chunkSize;
                    size -= chunkSize;
                };

                if (size > 0)
                {
                    DWORD bytesRead = 0;
                    BOOL success = ReadFile(fileHandle, bufferData, chunkSize, &bytesRead, nullptr);
                    if (!success || bytesRead != chunkSize)
                    {
                        throw FileReadError();
                    }
                }

                CloseHandle(fileHandle);
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
            HANDLE fileHandle = CreateFile(fileName, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (fileHandle == INVALID_HANDLE_VALUE)
            {
                throw FileNotFound();
            }

            DWORD bytesWritten = 0;
            BOOL success = WriteFile(fileHandle, buffer.data(), buffer.size(), &bytesWritten, nullptr);
            if (!success || bytesWritten != buffer.size())
            {
                throw FileWriteError();
            }

            CloseHandle(fileHandle);
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
