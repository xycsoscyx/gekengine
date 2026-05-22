#include "GEK/Utility/FileSystem.hpp"

#ifdef _WIN32
#include <Windows.h>
#endif

#include <cctype>

namespace Gek
{
    namespace FileSystem
    {
        namespace
        {
            std::string ToLowerAscii(std::string value)
            {
                std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c)
                               { return static_cast<char>(std::tolower(c)); });
                return value;
            }

#ifdef _WIN32
            std::filesystem::path ResolveExistingPath(std::filesystem::path const &path)
            {
                return path;
            }
#else
            std::filesystem::path ResolveExistingPath(std::filesystem::path const &path)
            {
                std::error_code errorCode;
                if (std::filesystem::exists(path, errorCode))
                {
                    return path;
                }

                std::filesystem::path currentPath;
                auto iterator = path.begin();

                if (path.has_root_path())
                {
                    currentPath = path.root_path();
                    while (iterator != path.end() && *iterator == path.root_path())
                    {
                        ++iterator;
                    }
                }
                else
                {
                    currentPath = std::filesystem::current_path(errorCode);
                    if (errorCode)
                    {
                        return path;
                    }
                }

                for (; iterator != path.end(); ++iterator)
                {
                    const std::filesystem::path requestedPart = *iterator;
                    if (requestedPart.empty() || requestedPart == ".")
                    {
                        continue;
                    }

                    std::filesystem::path exactPath = currentPath / requestedPart;
                    if (std::filesystem::exists(exactPath, errorCode))
                    {
                        currentPath = exactPath;
                        continue;
                    }

                    bool foundMatch = false;
                    const std::string requestedLower = ToLowerAscii(requestedPart.string());
                    for (auto const &entry : std::filesystem::directory_iterator(currentPath, errorCode))
                    {
                        if (errorCode)
                        {
                            break;
                        }

                        const std::string candidateLower = ToLowerAscii(entry.path().filename().string());
                        if (candidateLower == requestedLower)
                        {
                            currentPath = entry.path();
                            foundMatch = true;
                            break;
                        }
                    }

                    if (!foundMatch)
                    {
                        return path;
                    }
                }

                return currentPath;
            }
#endif
        } // namespace

        Path operator/(Path const &leftPath, std::string_view rightPath)
        {
            return leftPath.data / std::filesystem::path(rightPath);
        }

        Path operator/(Path const &leftPath, std::string const &rightPath)
        {
            return leftPath.data / std::filesystem::path(rightPath);
        }

        Path operator/(Path const &leftPath, Path const &rightPath)
        {
            return leftPath.data / rightPath.data;
        }

        Path operator/(Path const &leftPath, const char *rightPath)
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

        Path::Path(Path const &path)
            : data(path.data)
        {
            data.make_preferred();
        }

        Path::Path(const char *path)
            : data(path)
        {
            data.make_preferred();
        }

        void Path::operator=(std::string_view path)
        {
            data = path.data();
        }

        void Path::operator=(std::string const &path)
        {
            data = path.data();
        }

        void Path::operator=(Path const &path)
        {
            data = path.data;
        }

        bool Path::operator==(Path const &path)
        {
            return data == path.data;
        }

        bool Path::operator!=(Path const &path)
        {
            return data != path.data;
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

        Path Path::getRootPath(void) const
        {
            return data.root_path();
        }

        Path Path::getParentPath(void) const
        {
            return data.parent_path();
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
            auto resolvedPath = ResolveExistingPath(data);
            std::error_code errorCode;
            return std::filesystem::is_regular_file(resolvedPath, errorCode);
        }

        size_t Path::getFileSize(void) const
        {
            auto resolvedPath = ResolveExistingPath(data);
            std::error_code errorCode;
            return std::filesystem::file_size(resolvedPath, errorCode);
        }

        bool Path::isDirectory(void) const
        {
            auto resolvedPath = ResolveExistingPath(data);
            std::error_code errorCode;
            return std::filesystem::is_directory(resolvedPath, errorCode);
        }

        void Path::createChain(void) const
        {
            std::error_code errorCode;
            std::filesystem::create_directories(data, errorCode);
        }

        void Path::setWorkingDirectory(void) const
        {
            std::error_code errorCode;
            std::filesystem::create_directories(data, errorCode);
            std::filesystem::current_path(data, errorCode);
        }

        void Path::findFiles(std::function<bool(Path const &filePath)> onFileFound, bool recursive) const
        {
            std::error_code errorCode;
            for (auto const &fileSearch : std::filesystem::directory_iterator(data, errorCode))
            {
                Path filePath(fileSearch.path());
                if (recursive && filePath.isDirectory())
                {
                    filePath.findFiles(onFileFound, recursive);
                }
                else if (!onFileFound(Path(fileSearch.path())))
                {
                    return;
                }
            }
        }

        Path Path::lexicallyRelative(Path const &root) const
        {
            return data.lexically_relative(root.data);
        }

        std::string Read(Path const &filePath)
        {
            std::string buffer;
            auto resolvedPath = ResolveExistingPath(filePath.data);
            std::error_code errorCode;
            if (std::filesystem::is_regular_file(resolvedPath, errorCode))
            {
                std::ifstream file;
                file.open(resolvedPath.string().data(), std::ios::in);
                if (file.is_open())
                {
                    std::stringstream stream;
                    stream << file.rdbuf();
                    buffer = stream.str();
                    file.close();
                }
            }

            return buffer;
        }

        std::vector<uint8_t> Load(Path const &filePath, std::uintmax_t limitReadSize)
        {
            std::vector<uint8_t> buffer;
            auto resolvedPath = ResolveExistingPath(filePath.data);
            std::error_code errorCode;
            if (std::filesystem::is_regular_file(resolvedPath, errorCode))
            {
                std::uintmax_t fileSize = std::filesystem::file_size(resolvedPath, errorCode);
                auto size = (limitReadSize == 0 ? fileSize : std::min(fileSize, limitReadSize));
                if (size > 0)
                {
                    std::ifstream file;
                    file.open(resolvedPath.string().data(), std::ios::in | std::ios::binary);
                    if (file.is_open())
                    {
                        buffer.resize(size);
                        file.read(reinterpret_cast<char *>(buffer.data()), size);
                        file.close();
                    }
                }
            }

            return buffer;
        }

        Path GetCanonicalPath(Path const &path)
        {
            std::error_code errorCode;
            return std::filesystem::canonical(path.data, errorCode);
        }

        Path GetExistingPath(Path const &path)
        {
            return ResolveExistingPath(path.data);
        }

        Path GetModuleFilePath(void)
        {
#ifdef _WIN32
            std::string shortPath(1025, L'\0');
            GetModuleFileNameA(nullptr, &shortPath.at(0), 1024);
#else
            std::string shortPath = "/proc/self/exe";
#endif
            std::error_code errorCode;
            auto processName = std::filesystem::canonical(shortPath, errorCode);
            return Path(processName);
        }

        Path GetCacheFromModule(void)
        {
            auto modulePath = GetModuleFilePath().getParentPath();
            auto searchPath = modulePath;

            while (searchPath.isDirectory() && searchPath != modulePath.getRootPath())
            {
                if (searchPath.getFileName() == "bin")
                {
                    return searchPath / "cache";
                }
                else
                {
                    searchPath = searchPath.getParentPath();
                }
            };

            return modulePath / "cache";
        }
    } // namespace FileSystem
}; // namespace Gek
