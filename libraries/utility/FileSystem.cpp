#include "GEK\Utility\FileSystem.h"
#include <experimental\filesystem>
#include <fstream>

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
				std::basic_filebuf<uint8_t> fileBuffer;
				fileBuffer.open(fileName, std::ios::in | std::ios::binary);
				if (fileBuffer.is_open())
				{
					size = (limitReadSize > 0 ? std::min(size, limitReadSize) : size);
					if (size >= std::numeric_limits<std::size_t>::max())
					{
						throw FileReadError();
					}

					buffer.resize(static_cast<std::size_t>(size));
					auto bufferData = buffer.data();

					static const uint32_t chunkSize = 65536;
					while (size >= chunkSize)
					{
						fileBuffer.sgetn(bufferData, chunkSize);
						bufferData += chunkSize;
						size -= chunkSize;
					};

					if (size > 0)
					{
						fileBuffer.sgetn(bufferData, size);
					}
				}
				else
				{
					throw FileReadError();
				}
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
			std::basic_filebuf<uint8_t> fileBuffer;
			fileBuffer.open(fileName, std::ios::out | std::ios::binary | std::ios::trunc);
			if (fileBuffer.is_open())
			{
				auto size = buffer.size();
				auto bufferData = buffer.data();
				static const uint32_t chunkSize = 65536;
				while (size >= chunkSize)
				{
					fileBuffer.sputn(bufferData, chunkSize);
					bufferData += chunkSize;
					size -= chunkSize;
				};

				if (size > 0)
				{
					fileBuffer.sputn(bufferData, size);
				}
			}
			else
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
