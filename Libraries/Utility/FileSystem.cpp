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

        Path::Path(WString const &path)
            : std::experimental::filesystem::path(path)
        {
            make_preferred();
        }

        Path::Path(Path const &path)
            : std::experimental::filesystem::path(path)
        {
            make_preferred();
        }

        void Path::operator = (WString const &path)
        {
            assign(path);
        }

        void Path::operator = (Path const &path)
        {
            assign(path);
        }

        Path::operator wchar_t const * const (void) const
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

        void Path::replaceFileName(WString const &fileName)
        {
            replace_filename(fileName);
        }

        void Path::replaceExtension(WString const &extension)
        {
            replace_extension(extension);
        }

        Path Path::withExtension(WString const &extension) const
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

        WString Path::getFileName(void) const
        {
            return filename().native();
        }

        WString Path::getExtension(void) const
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

        bool Path::isNewerThan(Path const &path) const
        {
            auto thisWriteTime = std::experimental::filesystem::last_write_time(*this);
            auto thatWriteTime = std::experimental::filesystem::last_write_time(path);
            return (thisWriteTime > thatWriteTime);
        }

        Path GetModuleFilePath(void)
        {
#ifdef _WIN32
            WString relativeName((MAX_PATH + 1), L' ');
            GetModuleFileName(nullptr, &relativeName.at(0), MAX_PATH);
            relativeName.trimRight();

            WString absoluteName((MAX_PATH + 1), L' ');
            GetFullPathName(relativeName, MAX_PATH, &absoluteName.at(0), nullptr);
            absoluteName.trimRight();
#else
            CString processName(CString::Format("/proc/%v/exe", getpid()));
            WString absoluteName((MAX_PATH + 1), L'\0');
            readlink(processName, &absoluteName.at(0), MAX_PATH);
            absoluteName.trimRight();
#endif
            return absoluteName;
        }

        Path GetFileName(Path const &rootDirectory, const std::vector<WString> &list)
		{
            WString filePath(rootDirectory);
            filePath.join(list, std::experimental::filesystem::path::preferred_separator);
            return filePath;
		}

        void MakeDirectoryChain(Path const &filePath)
        {
            std::experimental::filesystem::create_directories(filePath);
        }

        void Find(Path const &rootDirectory, std::function<bool(Path const &)> onFileFound)
		{
			for (auto &fileSearch : std::experimental::filesystem::directory_iterator(rootDirectory))
			{
                Path filePath(fileSearch.path().wstring());
				onFileFound(filePath);
			}
		}

		void Load(Path const &filePath, std::vector<uint8_t> &buffer, std::uintmax_t limitReadSize)
		{
            auto size = std::experimental::filesystem::file_size(filePath);
            size = (limitReadSize == 0 ? size : std::min(size, limitReadSize));
            if (size > 0)
            {
                FILE *file = fopen(CString(filePath.native()), "rb");
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

		void Load(Path const &filePath, CString &string)
		{
			std::vector<uint8_t> buffer;
			Load(filePath, buffer);
			buffer.push_back('\0');
			string = reinterpret_cast<char *>(buffer.data());
		}

		void Load(Path const &filePath, WString &string)
		{
			std::vector<uint8_t> buffer;
			Load(filePath, buffer);
			buffer.push_back('\0');
			string = reinterpret_cast<char *>(buffer.data());
		}

        void Save(Path const &filePath, std::vector<uint8_t> const &buffer)
        {
            MakeDirectoryChain(filePath.getParentPath());

            FILE *file = fopen(CString(filePath.native()), "w+b");
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

        void Save(Path const &filePath, CString const &string)
        {
            std::vector<uint8_t> buffer(string.length());
            std::copy(std::begin(string), std::end(string), std::begin(buffer));
            Save(filePath, buffer);
        }

        void Save(Path const &filePath, WString const &string)
        {
            Save(filePath, CString(string));
        }
    } // namespace FileSystem
}; // namespace Gek
