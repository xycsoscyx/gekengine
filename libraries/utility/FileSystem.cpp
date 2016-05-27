#include "GEK\Utility\FileSystem.h"
#include <experimental\filesystem>
#include <atlbase.h>
#include <atlpath.h>

namespace Gek
{
    namespace FileSystem
    {
        wstring expandPath(const wchar_t *fileName)
        {
            wstring expandedFileName(fileName);
            if (expandedFileName.find(L"$root") != std::string::npos)
            {
                wstring currentModuleName(MAX_PATH + 1, L' ');
                GetModuleFileName(nullptr, &currentModuleName.at(0), MAX_PATH);
                currentModuleName.trimRight();

                wstring expandedModuleName(MAX_PATH + 1, L' ');
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

        void find(const wchar_t *fileName, const wchar_t *filterTypes, bool searchRecursively, std::function<bool(const wchar_t *)> onFileFound)
        {
            wstring expandedFileName(expandPath(fileName));
            //PathAddBackslashW(expandedFileName.GetBuffer(MAX_PATH + 1));

            WIN32_FIND_DATA findData;
            wstring expandedFileNameFilter(expandedFileName.getAppended(filterTypes).c_str());
            HANDLE findHandle = FindFirstFile(expandedFileNameFilter, &findData);
            if (findHandle != INVALID_HANDLE_VALUE)
            {
                do
                {
                    if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                    {
                        if (searchRecursively && findData.cFileName[0] != L'.')
                        {
                            find(expandedFileName.getAppended(findData.cFileName).c_str(), filterTypes, searchRecursively, onFileFound);
                        }
                    }
                    else
                    {
                        if (!onFileFound(expandedFileName.getAppended(findData.cFileName).c_str()))
                        {
                            break;
                        }
                    }
                } while (FindNextFile(findHandle, &findData));
                FindClose(findHandle);
            }
        }

        HMODULE loadLibrary(const wchar_t *fileName)
        {
            return LoadLibraryW(expandPath(fileName));
        }

        void load(const wchar_t *fileName, std::vector<UINT8> &buffer, size_t limitReadSize)
        {
            CStringW expandedFileName(expandPath(fileName));
            HANDLE fileHandle = CreateFile(expandedFileName, GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
            GEK_THROW_ERROR(fileHandle == INVALID_HANDLE_VALUE, FileNotFound, "Unable to open file: %", fileName);

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
                GEK_THROW_ERROR(!success, FileReadError, "Unable to read % bytes from file: %", buffer.size(), fileName);
                GEK_THROW_ERROR(bytesRead != buffer.size(), FileReadError, "Unable to read % bytes from file: %", buffer.size(), fileName);
            }

            CloseHandle(fileHandle);
        }

        void load(const wchar_t *fileName, string &fileData)
        {
            std::vector<UINT8> buffer;
            load(fileName, buffer);
            buffer.push_back('\0');
            fileData = reinterpret_cast<const char *>(buffer.data());
        }

        void load(const wchar_t *fileName, wstring &fileData, bool convertUTF8)
        {
            string rawFileData;
            load(fileName, rawFileData);
            fileData = String::from<wchar_t>(rawFileData);
        }

        void save(const wchar_t *fileName, const std::vector<UINT8> &buffer)
        {
            wstring expandedFileName(expandPath(fileName));
            HANDLE fileHandle = CreateFile(expandedFileName, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
            GEK_THROW_ERROR(fileHandle == INVALID_HANDLE_VALUE, FileNotFound, "Unable to create file: %", fileName);

            DWORD bytesWritten = 0;
            BOOL success = WriteFile(fileHandle, buffer.data(), buffer.size(), &bytesWritten, nullptr);
            GEK_THROW_ERROR(!success, FileWriteError, "Unable to write % bytes from file: %", buffer.size(), fileName);
            GEK_THROW_ERROR(bytesWritten != buffer.size(), FileWriteError, "Unable to write % bytes from file: %", buffer.size(), fileName);
            CloseHandle(fileHandle);
        }

        void save(const wchar_t *fileName, const char *fileData)
        {
            UINT32 stringLength = strlen(fileData);
            std::vector<UINT8> buffer(stringLength);
            std::copy_n(fileData, stringLength, buffer.begin());
            save(fileName, buffer);
        }

        void save(const wchar_t *fileName, const wchar_t *fileData, bool convertUTF8)
        {
            string rawString(String::from<char>(fileData));
            save(fileName, rawString.c_str());
        }
    } // namespace FileSystem
}; // namespace Gek
