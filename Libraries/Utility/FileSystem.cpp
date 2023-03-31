#include "GEK/Utility/FileSystem.hpp"
#include <fstream>
#include <Windows.h>

namespace Gek
{
    namespace FileSystem
	{
        Path operator / (Path const& leftPath, std::string_view rightPath)
        {
            return leftPath.data / std::filesystem::path(rightPath);
        }

        Path operator / (Path const& leftPath, std::string const& rightPath)
        {
            return leftPath.data / std::filesystem::path(rightPath);
        }

        Path operator / (Path const& leftPath, Path const& rightPath)
        {
            return leftPath.data / rightPath.data;
        }

        Path operator / (Path const& leftPath, const char *rightPath)
        {
            return leftPath.data / std::filesystem::path(rightPath);
        }

        Path::Path(void)
        {
        }

        Path::Path(std::filesystem::path const &path)
            : data(path)
        {
            data.make_preferred();
        }

        Path::Path(std::string_view path)
            : data(path.data())
        {
            data.make_preferred();
        }

        Path::Path(std::string const &path)
            : data(path.data())
        {
            data.make_preferred();
        }

        Path::Path(Path const& path)
            : data(path.data)
        {
            data.make_preferred();
        }

        Path::Path(const char *path)
            : data(path)
        {
            data.make_preferred();
        }

        void Path::operator = (std::string_view path)
        {
            data = path.data();
        }

        void Path::operator = (std::string const &path)
        {
            data = path.data();
        }

        void Path::operator = (Path const &path)
        {
            data = path.data;
        }

        Path &Path::removeFileName(void)
        {
            data.remove_filename();
            return (*this);
        }

        Path &Path::removeExtension(void)
        {
            data.replace_extension(String::Empty);
            return (*this);
        }

        Path &Path::replaceFileName(std::string_view fileName)
        {
            data.replace_filename(fileName.data());
            return (*this);
        }

        Path &Path::replaceExtension(std::string_view extension)
        {
            data.replace_extension(extension.data());
            return (*this);
        }

        Path Path::withExtension(std::string_view extension) const
        {
            auto replaced(data);
            replaced.replace_extension(extension.data());
            return replaced;
        }

        Path Path::withoutExtension(void) const
        {
            auto replaced(data);
            replaced.replace_extension(String::Empty);
            return replaced;
        }

        Path Path::getParentPath(void) const
        {
            return Path(data.parent_path());
        }

        std::string Path::getFileName(void) const
        {
            return data.filename().string();
        }

        std::string Path::getExtension(void) const
        {
            return data.extension().string();
        }

        std::string Path::getString(void) const
        {
            return data.string();
        }

        std::wstring Path::getWindowsString(void) const
        {
            return data.native();
        }

        void Path::rename(Path const &name) const
        {
            std::filesystem::rename(data, name.data);
        }

        bool Path::isNewerThan(Path const &path) const
        {
            std::error_code errorCode;
            auto thisWriteTime = std::filesystem::last_write_time(data, errorCode);
            auto thatWriteTime = std::filesystem::last_write_time(path.data, errorCode);
            return (thisWriteTime > thatWriteTime);
        }

        bool Path::isFile(void) const
        {
            std::error_code errorCode;
            return std::filesystem::is_regular_file(data, errorCode);
        }

        size_t Path::getFileSize(void) const
        {
            std::error_code errorCode;
            return std::filesystem::file_size(data, errorCode);
        }

        bool Path::isDirectory(void) const
        {
            std::error_code errorCode;
            return std::filesystem::is_directory(data, errorCode);
        }

        void Path::createChain(void) const
        {
            std::error_code errorCode;
            std::filesystem::create_directories(data, errorCode);
        }

        void Path::findFiles(std::function<bool(Path const &filePath)> onFileFound) const
        {
            std::error_code errorCode;
            for (auto const &fileSearch : std::filesystem::directory_iterator(data, errorCode))
            {
                if (!onFileFound(Path(fileSearch.path())))
                {
                    return;
                }
            }
        }

        Path GetModuleFilePath(void)
        {
#ifdef _WIN32
            std::wstring relativeName((MAX_PATH + 1), L'\0');
            GetModuleFileName(nullptr, &relativeName.at(0), MAX_PATH);

            std::wstring absoluteName((MAX_PATH + 1), L'\0');
            GetFullPathName(relativeName.data(), MAX_PATH, &absoluteName.at(0), nullptr);
#else
            std:string processName(std::format("/proc/{}/exe", getpid()));
            std::string absoluteName((MAX_PATH + 1), L'\0');
            readlink(processName, &absoluteName.at(0), MAX_PATH);
            String::TrimRight(absoluteName);
#endif
            return Path(String::Narrow(absoluteName));
        }
    } // namespace FileSystem
}; // namespace Gek
