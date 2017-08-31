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

        Path::Path(std::wstring const &path)
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

        void Path::operator = (std::wstring const &path)
        {
            assign(path);
        }

        void Path::operator = (Path const &path)
        {
            assign(path);
        }

        Path &Path::removeFileName(void)
        {
            remove_filename();
            return (*this);
        }

        Path &Path::removeExtension(void)
        {
            replace_extension(String::Empty);
            return (*this);
        }

        Path &Path::replaceFileName(std::string const &fileName)
        {
            replace_filename(fileName);
            return (*this);
        }

        Path &Path::replaceExtension(std::string const &extension)
        {
            replace_extension(extension);
            return (*this);
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
            std::error_code errorCode;
            return std::experimental::filesystem::is_regular_file(*this, errorCode);
        }

        bool Path::isDirectory(void) const
        {
            std::error_code errorCode;
            return std::experimental::filesystem::is_directory(*this, errorCode);
        }

        bool Path::isNewerThan(Path const &path) const
        {
            std::error_code errorCode;
            auto thisWriteTime = std::experimental::filesystem::last_write_time(*this, errorCode);
            auto thatWriteTime = std::experimental::filesystem::last_write_time(path, errorCode);
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
            std::error_code errorCode;
            std::experimental::filesystem::create_directories(filePath, errorCode);
        }

        void Find(Path const &rootDirectory, std::function<bool(Path const &)> onFileFound)
		{
            std::error_code errorCode;
            for (auto const &fileSearch : std::experimental::filesystem::directory_iterator(rootDirectory, errorCode))
			{
                if (!onFileFound(fileSearch.path().u8string()))
                {
                    return;
                }
			}
		}
    } // namespace FileSystem
}; // namespace Gek
