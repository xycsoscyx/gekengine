#include "GEK\Utility\FileSystem.h"
#include <Windows.h>

namespace Gek
{
    namespace FileSystem
    {
        Path::Path(void)
        {
        }

        Path::Path(const char *path)
            : path(path)
        {
        }

        Path::Path(const wchar_t *path)
            : path(path)
        {
        }

        Path &Path::operator = (const char *path)
        {
            if (path)
            {
                assign(path);
            }
            else
            {
                empty();
            }

            return (*this);
        }

        Path &Path::operator = (const wchar_t *path)
        {
            if (path)
            {
                assign(path);
            }
            else
            {
                empty();
            }

            return (*this);
        }

        Path &Path::operator = (const Path &path)
        {
            if (path)
            {
                assign(path);
            }
            else
            {
                empty();
            }

            return (*this);
        }

        Path::operator const wchar_t * () const
        {
            return c_str();
        }

        Path::operator String () const
        {
            return string();
        }

        String Path::getExtension(void) const
        {
            return extension().string();
        }

        String Path::getFileName(void) const
        {
            return filename().string();
        }

        Path Path::getPath(void) const
        {
            return parent_path().c_str();
        }

        bool Path::isFile(void) const
        {
            return std::experimental::filesystem::is_regular_file(*this);
        }

        bool Path::isDirectory(void) const
        {
            return std::experimental::filesystem::is_directory(*this);
        }

        String expandPath(const wchar_t *fileName)
        {
            String expandedFileName(fileName);
            if (expandedFileName.find(L"$root") != std::string::npos)
            {
                String currentModuleName(L' ', MAX_PATH + 1);
                GetModuleFileName(nullptr, &currentModuleName.at(0), MAX_PATH);
                currentModuleName.trimRight();

                String expandedModuleName(L' ', MAX_PATH + 1);
                GetFullPathName(currentModuleName, MAX_PATH, &expandedModuleName.at(0), nullptr);
                expandedModuleName.trimRight();

                FileSystem::Path currentModulePath(expandedModuleName);
                currentModulePath.remove_filename();
                currentModulePath.remove_filename();

                expandedFileName.replace(L"$root", currentModulePath);
            }

            expandedFileName.replace(L"/", L"\\");
            return expandedFileName;
        }

        void find(const wchar_t *fileName, const wchar_t *filterTypes, bool searchRecursively, std::function<bool(const wchar_t *)> onFileFound)
        {
            String expandedFileName(expandPath(fileName));

            FileSystem::Path expandedPath(expandedFileName);
            expandedPath.append(filterTypes);

            WIN32_FIND_DATA findData;
            HANDLE findHandle = FindFirstFile(expandedPath, &findData);
            if (findHandle != INVALID_HANDLE_VALUE)
            {
                do
                {
                    FileSystem::Path foundPath(expandedFileName);
                    foundPath.append(findData.cFileName);

                    if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                    {
                        if (searchRecursively && findData.cFileName[0] != L'.')
                        {
                            find(foundPath, filterTypes, searchRecursively, onFileFound);
                        }
                    }
                    else
                    {
                        if (!onFileFound(foundPath))
                        {
                            break;
                        }
                    }
                } while (FindNextFile(findHandle, &findData));
                FindClose(findHandle);
            }
        }

        void load(const wchar_t *fileName, std::vector<uint8_t> &buffer, size_t limitReadSize)
        {
            String expandedFileName(expandPath(fileName));
            HANDLE fileHandle = CreateFile(expandedFileName, GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
            GEK_CHECK_CONDITION(fileHandle == INVALID_HANDLE_VALUE, FileNotFound, "Unable to open file: %v", fileName);

            DWORD fileSize = GetFileSize(fileHandle, nullptr);
            if (fileSize > 0)
            {
                if (limitReadSize > 0)
                {
                    buffer.resize(limitReadSize);
                }
                else
                {
                    buffer.resize(fileSize);
                }

                DWORD bytesRead = 0;
                BOOL success = ReadFile(fileHandle, buffer.data(), buffer.size(), &bytesRead, nullptr);
                GEK_CHECK_CONDITION(!success, FileReadError, "Unable to read %v bytes from file: %v", buffer.size(), fileName);
                GEK_CHECK_CONDITION(bytesRead != buffer.size(), FileReadError, "Unable to read %v bytes from file: %v", buffer.size(), fileName);
            }

            CloseHandle(fileHandle);
        }

        void load(const wchar_t *fileName, StringUTF8 &fileData)
        {
            std::vector<uint8_t> buffer;
            load(fileName, buffer);
            buffer.push_back('\0');
            fileData = reinterpret_cast<const char *>(buffer.data());
        }

        void load(const wchar_t *fileName, String &fileData)
        {
            StringUTF8 rawFileData;
            load(fileName, rawFileData);
            fileData = rawFileData;
        }

        void save(const wchar_t *fileName, const std::vector<uint8_t> &buffer)
        {
            String expandedFileName(expandPath(fileName));
            HANDLE fileHandle = CreateFile(expandedFileName, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
            GEK_CHECK_CONDITION(fileHandle == INVALID_HANDLE_VALUE, FileNotFound, "Unable to create file: %v", fileName);

            DWORD bytesWritten = 0;
            BOOL success = WriteFile(fileHandle, buffer.data(), buffer.size(), &bytesWritten, nullptr);
            GEK_CHECK_CONDITION(!success, FileWriteError, "Unable to write %v bytes from file: %v", buffer.size(), fileName);
            GEK_CHECK_CONDITION(bytesWritten != buffer.size(), FileWriteError, "Unable to write %v bytes from file: %v", buffer.size(), fileName);
            CloseHandle(fileHandle);
        }

        void save(const wchar_t *fileName, const StringUTF8 &fileData)
        {
            std::vector<uint8_t> buffer(fileData.length());
            std::copy(fileData.begin(), fileData.end(), buffer.begin());
            save(fileName, buffer);
        }

        void save(const wchar_t *fileName, const String &fileData)
        {
            save(fileName, StringUTF8(fileData));
        }
    } // namespace FileSystem
}; // namespace Gek
