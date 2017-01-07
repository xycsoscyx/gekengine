#include "GEK/Utility/FileSystem.hpp"
#include <fstream>
#include <Windows.h>

namespace Gek
{
	namespace FileSystem
	{
        Path::Path(void)
        {
        }

        Path::Path(const wchar_t *path)
            : std::experimental::filesystem::path(path)
        {
            make_preferred();
        }

        Path::Path(const String &path)
            : std::experimental::filesystem::path(path)
        {
            make_preferred();
        }

        Path::Path(const Path &path)
            : std::experimental::filesystem::path(path)
        {
            make_preferred();
        }

        void Path::operator = (const wchar_t *path)
        {
            assign(path);
        }

        void Path::operator = (const String &path)
        {
            assign(path);
        }

        void Path::operator = (const Path &path)
        {
            assign(path);
        }

        Path::operator const wchar_t * (void) const
        {
            return c_str();
        }

        void Path::removeFileName(void)
        {
            remove_filename();
        }

        void Path::removeExtension(void)
        {
            replace_extension(L"");
        }

        void Path::replaceFileName(const wchar_t *fileName)
        {
            replace_filename(fileName);
        }

        void Path::replaceExtension(const wchar_t *extension)
        {
            replace_extension(extension);
        }

        Path Path::withExtension(const wchar_t *extension) const
        {
            Path path(*this);
            path.replace_extension(extension);
            return path;
        }

        Path Path::withoutExtension(void) const
        {
            Path path(*this);
            path.replace_extension(L"");
            return path;
        }

        Path Path::getParentPath(void) const
        {
            return parent_path().native();
        }

        String Path::getFileName(void) const
        {
            return filename().native();
        }

        String Path::getExtension(void) const
        {
            return extension().native();
        }

        bool Path::isFile(void) const
        {
            return std::experimental::filesystem::is_regular_file(*this);
        }

        bool Path::isDirectory(void) const
        {
            return std::experimental::filesystem::is_directory(*this);
        }

        bool Path::isNewerThan(const Path &path) const
        {
            auto thisWriteTime = std::experimental::filesystem::last_write_time(*this);
            auto thatWriteTime = std::experimental::filesystem::last_write_time(path);
            return (thisWriteTime > thatWriteTime);
        }

        Path GetModuleFilePath(void)
        {
#ifdef _WIN32
            String relativeName((MAX_PATH + 1), L' ');
            GetModuleFileName(nullptr, &relativeName.at(0), MAX_PATH);
            relativeName.trimRight();

            String absoluteName((MAX_PATH + 1), L' ');
            GetFullPathName(relativeName, MAX_PATH, &absoluteName.at(0), nullptr);
            absoluteName.trimRight();
#else
            StringUTF8 processName(StringUTF8::Format("/proc/%v/exe", getpid()));
            String absoluteName((MAX_PATH + 1), L'\0');
            readlink(processName, &absoluteName.at(0), MAX_PATH);
            absoluteName.trimRight();
#endif
            return absoluteName;
        }

        Path GetFileName(const Path &rootDirectory, const std::vector<String> &list)
		{
            String filePath(rootDirectory);
            filePath.join(list, std::experimental::filesystem::path::preferred_separator);
            return filePath;
		}

        void MakeDirectoryChain(const Path &filePath)
        {
            std::experimental::filesystem::create_directories(filePath);
        }

        void Find(const Path &rootDirectory, std::function<bool(const Path &)> onFileFound)
		{
			for (auto &fileSearch : std::experimental::filesystem::directory_iterator(rootDirectory))
			{
				onFileFound(fileSearch.path().wstring().c_str());
			}
		}

		void Load(const Path &filePath, std::vector<uint8_t> &buffer, std::uintmax_t limitReadSize)
		{
            auto size = std::experimental::filesystem::file_size(filePath);
            size = (limitReadSize == 0 ? size : std::min(size, limitReadSize));
            if (size > 0)
            {
                FILE *file = fopen(StringUTF8(filePath.native()), "rb");
                if (file == nullptr)
                {
                    throw FileNotFound("Unable to open file for reading");
                }

                buffer.resize(size);
                auto read = fread(buffer.data(), size, 1, file);
                fclose(file);

                if (read != 1)
                {
                    throw FileReadError("Unable to read file contents");
                }
            }
		}

		void Load(const Path &filePath, StringUTF8 &string)
		{
			std::vector<uint8_t> buffer;
			Load(filePath, buffer);
			buffer.push_back('\0');
			string = reinterpret_cast<char *>(buffer.data());
		}

		void Load(const Path &filePath, String &string)
		{
			std::vector<uint8_t> buffer;
			Load(filePath, buffer);
			buffer.push_back('\0');
			string = reinterpret_cast<char *>(buffer.data());
		}

        void Save(const Path &filePath, const std::vector<uint8_t> &buffer)
        {
            MakeDirectoryChain(filePath.getParentPath());

            FILE *file = fopen(StringUTF8(filePath.native()), "w+b");
			if (file == nullptr)
			{
				throw FileNotFound("Unable to open file for writing");
			}

            auto wrote = fwrite(buffer.data(), buffer.size(), 1, file);
            fclose(file);

			if (wrote != 1)
			{
				throw FileWriteError("Unable to write contents to file");
			}
        }

        void Save(const Path &filePath, const StringUTF8 &string)
        {
            std::vector<uint8_t> buffer(string.length());
            std::copy(std::begin(string), std::end(string), std::begin(buffer));
            Save(filePath, buffer);
        }

        void Save(const Path &filePath, const String &string)
        {
            Save(filePath, StringUTF8(string));
        }
    } // namespace FileSystem
}; // namespace Gek
