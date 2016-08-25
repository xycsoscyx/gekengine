#include "GEK\Utility\FileSystem.h"
#include <experimental\filesystem>
#include <fstream>
#include <Windows.h>

namespace Gek
{
	namespace FileSystem
	{
		String getRoot(void)
		{
            String rootPath;
            String currentModuleName(L' ', MAX_PATH + 1);
			GetModuleFileName(nullptr, &currentModuleName.at(0), MAX_PATH);
			currentModuleName.trimRight();

			String fullModuleName(L' ', MAX_PATH + 1);
			GetFullPathName(currentModuleName, MAX_PATH, &fullModuleName.at(0), nullptr);
			fullModuleName.trimRight();

			std::experimental::filesystem::path fullModulePath(fullModuleName);
			fullModulePath.remove_filename();
			fullModulePath.remove_filename();

			return fullModulePath.wstring();
		}
		
		String getFileName(const wchar_t *rootDirectory, const std::initializer_list<String> &list)
		{
			std::experimental::filesystem::path fileName(rootDirectory);
			for (auto name : list)
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

		void load(const wchar_t *fileName, std::vector<uint8_t> &buffer, std::size_t limitReadSize)
		{
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

        void save(const wchar_t *fileName, const std::vector<uint8_t> &buffer)
        {
			std::basic_ofstream<uint8_t, std::char_traits<uint8_t>> fileStream(fileName, std::ios::out | std::ios::binary | std::ios::trunc);
			if (fileStream.is_open())
			{
				fileStream.write(buffer.data(), buffer.size());
				fileStream.close();
			}
			else
			{
				throw FileNotFound();
			}
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
