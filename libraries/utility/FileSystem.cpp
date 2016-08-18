#include "GEK\Utility\FileSystem.h"
#include <Windows.h>

namespace Gek
{
    namespace FileSystem
    {
        Path::Path(void)
        {
        }

        Path::Path(const String &path)
            : path(path)
        {
        }

        Path::Path(const Path &path)
            : path(path)
        {
        }

        Path &Path::operator = (const String &path)
        {
            assign(path);
            return (*this);
        }

        Path &Path::operator = (const Path &path)
        {
            assign(path);
            return (*this);
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
            return String(parent_path().string());
        }

        bool Path::isFile(void) const
        {
            return std::experimental::filesystem::is_regular_file(*this);
        }

        bool Path::isDirectory(void) const
        {
            return std::experimental::filesystem::is_directory(*this);
        }

        String expandPath(const String &fileName)
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

        void find(const String &fileName, const String &filterTypes, bool searchRecursively, std::function<bool(const String &)> onFileFound)
        {
            String expandedFileName(expandPath(fileName));

            FileSystem::Path expandedPath(expandedFileName);
            expandedPath.append(filterTypes);

            WIN32_FIND_DATA findData;
            HANDLE findHandle = FindFirstFile(expandedPath.c_str(), &findData);
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

        void load(const String &fileName, std::vector<uint8_t> &buffer, size_t limitReadSize)
        {
            String expandedFileName(expandPath(fileName));
            HANDLE fileHandle = CreateFile(expandedFileName, GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (fileHandle == INVALID_HANDLE_VALUE)
            {
                throw FileNotFound();
            }

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
                if (!success || bytesRead != buffer.size())
                {
                    throw FileReadError();
                }
            }

            CloseHandle(fileHandle);
        }

        void load(const String &fileName, StringUTF8 &fileData)
        {
            std::vector<uint8_t> buffer;
            load(fileName, buffer);
            buffer.push_back('\0');
            fileData = reinterpret_cast<const char *>(buffer.data());
        }

        void load(const String &fileName, String &fileData)
        {
            StringUTF8 rawFileData;
            load(fileName, rawFileData);
            fileData = rawFileData;
        }

        void save(const String &fileName, const std::vector<uint8_t> &buffer)
        {
            String expandedFileName(expandPath(fileName));
            HANDLE fileHandle = CreateFile(expandedFileName, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
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

        void save(const String &fileName, const StringUTF8 &fileData)
        {
            std::vector<uint8_t> buffer(fileData.length());
            std::copy(fileData.begin(), fileData.end(), buffer.begin());
            save(fileName, buffer);
        }

        void save(const String &fileName, const String &fileData)
        {
            save(fileName, StringUTF8(fileData));
        }
    } // namespace FileSystem
}; // namespace Gek
