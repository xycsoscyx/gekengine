#include "GEK\Utility\FileSystem.h"
#include <experimental\filesystem>
#include <atlbase.h>
#include <atlpath.h>

namespace Gek
{
    namespace FileSystem
    {
        wstring expandPath(const wstring &fileName)
        {
            wstring expandedFileName(fileName);
            if (expandedFileName.find(L"$root") != std::string::npos)
            {
                wstring currentModuleName(L' ', MAX_PATH + 1);
                GetModuleFileName(nullptr, &currentModuleName.at(0), MAX_PATH);
                currentModuleName.trimRight();

                wstring expandedModuleName(L' ', MAX_PATH + 1);
                GetFullPathName(currentModuleName, MAX_PATH, &expandedModuleName.at(0), nullptr);
                expandedModuleName.trimRight();

                std::experimental::filesystem::path currentModulePath(expandedModuleName);
                currentModulePath.remove_filename();
                currentModulePath.remove_filename();

                expandedFileName.replace(L"$root", currentModulePath.c_str());
            }

            expandedFileName.replace(L"/", L"\\");
            return expandedFileName;
        }

        void find(const wstring &fileName, const wstring &filterTypes, bool searchRecursively, std::function<bool(const wstring &)> onFileFound)
        {
            wstring expandedFileName(expandPath(fileName));

            std::experimental::filesystem::path expandedPath(expandedFileName);
            expandedPath.append(filterTypes);

            WIN32_FIND_DATA findData;
            HANDLE findHandle = FindFirstFile(expandedPath.c_str(), &findData);
            if (findHandle != INVALID_HANDLE_VALUE)
            {
                do
                {
                    std::experimental::filesystem::path foundPath(expandedFileName);
                    foundPath.append(findData.cFileName);

                    if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                    {
                        if (searchRecursively && findData.cFileName[0] != L'.')
                        {
                            find(foundPath.c_str(), filterTypes, searchRecursively, onFileFound);
                        }
                    }
                    else
                    {
                        if (!onFileFound(foundPath.c_str()))
                        {
                            break;
                        }
                    }
                } while (FindNextFile(findHandle, &findData));
                FindClose(findHandle);
            }
        }

        HMODULE loadLibrary(const wstring &fileName)
        {
            return LoadLibraryW(expandPath(fileName));
        }

        void load(const wstring &fileName, std::vector<UINT8> &buffer, size_t limitReadSize)
        {
            CStringW expandedFileName(expandPath(fileName));
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

        void load(const wstring &fileName, string &fileData)
        {
            std::vector<UINT8> buffer;
            load(fileName, buffer);
            buffer.push_back('\0');
            fileData = reinterpret_cast<const char *>(buffer.data());
        }

        void load(const wstring &fileName, wstring &fileData, bool convertUTF8)
        {
            string rawFileData;
            load(fileName, rawFileData);
            fileData = rawFileData;
        }

        void save(const wstring &fileName, const std::vector<UINT8> &buffer)
        {
            wstring expandedFileName(expandPath(fileName));
            HANDLE fileHandle = CreateFile(expandedFileName, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
            GEK_CHECK_CONDITION(fileHandle == INVALID_HANDLE_VALUE, FileNotFound, "Unable to create file: %v", fileName);

            DWORD bytesWritten = 0;
            BOOL success = WriteFile(fileHandle, buffer.data(), buffer.size(), &bytesWritten, nullptr);
            GEK_CHECK_CONDITION(!success, FileWriteError, "Unable to write %v bytes from file: %v", buffer.size(), fileName);
            GEK_CHECK_CONDITION(bytesWritten != buffer.size(), FileWriteError, "Unable to write %v bytes from file: %v", buffer.size(), fileName);
            CloseHandle(fileHandle);
        }

        void save(const wstring &fileName, const string &fileData)
        {
            std::vector<UINT8> buffer(fileData.length());
            std::copy(fileData.begin(), fileData.end(), buffer.begin());
            save(fileName, buffer);
        }

        void save(const wstring &fileName, const wstring &fileData, bool convertUTF8)
        {
            save(fileName, string(fileData));
        }
    } // namespace FileSystem
}; // namespace Gek
