#include "GEK\Utility\FileSystem.h"
#include <Windows.h>
#include <fstream>

namespace Gek
{
	namespace FileSystem
	{
        String getExtension(const wchar_t *fileName)
        {
            std::experimental::filesystem::path filePath(fileName);
            return filePath.extension().wstring();
        }

        String removeFileName(const wchar_t *fileName)
        {
            std::experimental::filesystem::path filePath(fileName);
            return filePath.parent_path().wstring();
        }

        String removeDirectory(const wchar_t *fileName)
        {
            std::experimental::filesystem::path filePath(fileName);
            return filePath.filename().wstring();
        }

        bool isFile(const wchar_t *fileName)
        {
            return std::experimental::filesystem::is_regular_file(fileName);
        }

        bool isDirectory(const wchar_t *fileName)
        {
            return std::experimental::filesystem::is_directory(fileName);
        }

        String replaceRoot(const wchar_t *originalFileName)
        {
            String fileName(originalFileName);
            if (fileName.find(L"$root") != std::string::npos)
            {
                String currentModuleName(L' ', MAX_PATH + 1);
                GetModuleFileName(nullptr, &currentModuleName.at(0), MAX_PATH);
                currentModuleName.trimRight();

                String fullModuleName(L' ', MAX_PATH + 1);
                GetFullPathName(currentModuleName, MAX_PATH, &fullModuleName.at(0), nullptr);
                fullModuleName.trimRight();

                std::experimental::filesystem::path fullModulePath(fullModuleName);
                fullModulePath.remove_filename();
                fullModulePath.remove_filename();

                fileName.replace(L"$root", fullModulePath.wstring());
            }

            fileName.replace(L"/", L"\\");

            String fullPathName(L' ', (MAX_PATH + 1));
            GetFullPathName(fileName, MAX_PATH, &fullPathName.at(0), nullptr);
            fullPathName.trimRight();
            return fullPathName;
        }

		void find(const wchar_t *originalRootDirectory, const wchar_t *filterTypes, bool searchRecursively, std::function<bool(const wchar_t *)> onFileFound)
		{
            String rootDirectory(replaceRoot(originalRootDirectory));
            String filtersPath(getFileName(rootDirectory, filterTypes));

			WIN32_FIND_DATA findData;
			HANDLE findHandle = FindFirstFile(filtersPath, &findData);
			if (findHandle != INVALID_HANDLE_VALUE)
			{
				do
				{
                    String foundFileName(getFileName(rootDirectory, findData.cFileName));
					if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
					{
						if (searchRecursively && findData.cFileName[0] != L'.')
						{
							find(foundFileName, filterTypes, searchRecursively, onFileFound);
						}
					}
					else
					{
						if (!onFileFound(foundFileName))
						{
							break;
						}
					}
				} while (FindNextFile(findHandle, &findData));
				FindClose(findHandle);
			}
		}

		void load(const wchar_t *originalFileName, std::vector<uint8_t> &buffer, std::size_t limitReadSize)
		{
            String fileName(replaceRoot(originalFileName));
			std::basic_ifstream<uint8_t, std::char_traits<uint8_t>> fileStream(fileName, std::ios::in | std::ios::binary | std::ios::ate);
			if (fileStream.is_open())
			{
				std::size_t size = static_cast<std::size_t>(fileStream.tellg());
				fileStream.seekg(0, std::ios::beg);
				buffer.resize(limitReadSize ? std::min(limitReadSize, size) : size);
				fileStream.read(buffer.data(), buffer.size());
				fileStream.close();
			}
			else
			{
				throw FileNotFound();
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

        void save(const wchar_t *originalFileName, const std::vector<uint8_t> &buffer)
        {
            String fileName(getFileName(originalFileName));
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

        void save(const wchar_t *fileName, const StringUTF8 &fileData)
        {
            std::vector<uint8_t> buffer(fileData.length());
            std::copy(fileData.begin(), fileData.end(), buffer.begin());
            save(fileName, buffer);
        }

        void save(const wchar_t *fileName, const wchar_t *fileData)
        {
            save(fileName, StringUTF8(fileData));
        }
    } // namespace FileSystem
}; // namespace Gek
