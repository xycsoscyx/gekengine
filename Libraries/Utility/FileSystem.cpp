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

		Path::Path(std::string const &path)
			: std::experimental::filesystem::path(path)
		{
			make_preferred();
		}

		Path::Path(std::experimental::filesystem::path const &path)
			: std::experimental::filesystem::path(path)
		{
			make_preferred();
		}

        Path::Path(Path const &path)
            : std::experimental::filesystem::path(path)
        {
            make_preferred();
        }

        void Path::operator = (std::string const &path)
        {
            assign(path);
        }

        void Path::operator = (Path const &path)
        {
            assign(path);
        }

        void Path::removeFileName(void)
        {
            remove_filename();
        }

        void Path::removeExtension(void)
        {
            replace_extension(String::Empty);
        }

        void Path::replaceFileName(std::string const &fileName)
        {
            replace_filename(fileName);
        }

        void Path::replaceExtension(std::string const &extension)
        {
            replace_extension(extension);
        }

        Path Path::withExtension(std::string const &extension) const
        {
            Path path(*this);
            path.replace_extension(extension);
            return path;
        }

        Path Path::withoutExtension(void) const
        {
            Path path(*this);
            path.replace_extension(String::Empty);
            return path;
        }

        Path Path::getParentPath(void) const
        {
            return parent_path().u8string();
        }

        std::string Path::getFileName(void) const
        {
            return filename().u8string();
        }

        std::string Path::getExtension(void) const
        {
            return extension().u8string();
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
            std::wstring relativeName((MAX_PATH + 1), L' ');
            GetModuleFileName(nullptr, &relativeName.at(0), MAX_PATH);
            String::TrimRight(relativeName);

            std::wstring absoluteName((MAX_PATH + 1), L' ');
            GetFullPathName(relativeName.c_str(), MAX_PATH, &absoluteName.at(0), nullptr);
            String::TrimRight(absoluteName);
#else
            CString processName(CString::Format("/proc/%v/exe", getpid()));
            std::string absoluteName((MAX_PATH + 1), L'\0');
            readlink(processName, &absoluteName.at(0), MAX_PATH);
            String::TrimRight(absoluteName);
#endif
			return std::experimental::filesystem::path(absoluteName);
        }

        void MakeDirectoryChain(Path const &filePath)
        {
            std::experimental::filesystem::create_directories(filePath);
        }

        void Find(Path const &rootDirectory, std::function<bool(Path const &)> onFileFound)
		{
			for (const auto &fileSearch : std::experimental::filesystem::directory_iterator(rootDirectory))
			{
				onFileFound(fileSearch.path().u8string());
			}
		}
    } // namespace FileSystem
}; // namespace Gek
