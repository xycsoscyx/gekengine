#include "GEK\Utility\FileSystem.h"
#include <atlbase.h>
#include <atlpath.h>

namespace Gek
{
    namespace FileSystem
    {
        CStringW expandPath(LPCWSTR fileName)
        {
            CStringW expandedFileName(fileName);
            if (expandedFileName.Find(L"%root%") >= 0)
            {
                CStringW currentModuleName;
                GetModuleFileName(nullptr, currentModuleName.GetBuffer(MAX_PATH + 1), MAX_PATH);
                currentModuleName.ReleaseBuffer();

                CPathW currentModulePath;
                CStringW &currentModulePathString = currentModulePath;
                GetFullPathName(currentModuleName, MAX_PATH, currentModulePathString.GetBuffer(MAX_PATH + 1), nullptr);
                currentModulePathString.ReleaseBuffer();

                // Remove filename from path
                currentModulePath.RemoveFileSpec();

                // Remove debug/release form path
                currentModulePath.RemoveFileSpec();

                expandedFileName.Replace(L"%root%", currentModulePath);
            }

            expandedFileName.Replace(L"/", L"\\");
            return expandedFileName;
        }

        void find(LPCWSTR fileName, LPCWSTR filterTypes, bool searchRecursively, std::function<bool(LPCWSTR)> onFileFound)
        {
            CStringW expandedFileName(expandPath(fileName));
            PathAddBackslashW(expandedFileName.GetBuffer(MAX_PATH + 1));
            expandedFileName.ReleaseBuffer();

            WIN32_FIND_DATA findData;
            CStringW expandedFileNameFilter(expandedFileName + filterTypes);
            HANDLE findHandle = FindFirstFile(expandedFileNameFilter, &findData);
            if (findHandle != INVALID_HANDLE_VALUE)
            {
                do
                {
                    if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                    {
                        if (searchRecursively && findData.cFileName[0] != L'.')
                        {
                            find((expandedFileName + findData.cFileName), filterTypes, searchRecursively, onFileFound);
                        }
                    }
                    else
                    {
                        if (!onFileFound(expandedFileName + findData.cFileName))
                        {
                            break;
                        }
                    }
                } while (FindNextFile(findHandle, &findData));
                FindClose(findHandle);
            }
        }

        HMODULE loadLibrary(LPCWSTR fileName)
        {
            return LoadLibraryW(expandPath(fileName));
        }

        void load(LPCWSTR fileName, std::vector<UINT8> &buffer, size_t limitReadSize)
        {
            CStringW expandedFileName(expandPath(fileName));
            HANDLE fileHandle = CreateFile(expandedFileName, GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
            GEK_CHECK_EXCEPTION(fileHandle == INVALID_HANDLE_VALUE, FileNotFound, "Unable to open file: %S", fileName);

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
                GEK_CHECK_EXCEPTION(!success, FileReadError, "Unable to read %d bytes from file: %S", buffer.size(), fileName);
                GEK_CHECK_EXCEPTION(bytesRead != buffer.size(), FileReadError, "Unable to read %d bytes from file: %S", buffer.size(), fileName);
            }

            CloseHandle(fileHandle);
        }

        void load(LPCWSTR fileName, CStringA &string)
        {
            std::vector<UINT8> buffer;
            load(fileName, buffer);
            buffer.push_back('\0');
            string = LPCSTR(buffer.data());
        }

        void load(LPCWSTR fileName, CStringW &string, bool convertUTF8)
        {
            CStringA readString;
            load(fileName, readString);
            string = CA2W(readString, (convertUTF8 ? CP_UTF8 : CP_ACP));
        }

        void save(LPCWSTR fileName, const std::vector<UINT8> &buffer)
        {
            CStringW expandedFileName(expandPath(fileName));
            HANDLE fileHandle = CreateFile(expandedFileName, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
            GEK_CHECK_EXCEPTION(fileHandle == INVALID_HANDLE_VALUE, FileNotFound, "Unable to create file: %S", fileName);

            DWORD bytesWritten = 0;
            BOOL success = WriteFile(fileHandle, buffer.data(), buffer.size(), &bytesWritten, nullptr);
            GEK_CHECK_EXCEPTION(!success, FileWriteError, "Unable to write %d bytes from file: %S", buffer.size(), fileName);
            GEK_CHECK_EXCEPTION(bytesWritten != buffer.size(), FileWriteError, "Unable to write %d bytes from file: %S", buffer.size(), fileName);
            CloseHandle(fileHandle);
        }

        void save(LPCWSTR fileName, LPCSTR string)
        {
            UINT32 stringLength = strlen(string);
            std::vector<UINT8> buffer(stringLength);
            std::copy_n(string, stringLength, buffer.begin());
            save(fileName, buffer);
        }

        void save(LPCWSTR fileName, LPCWSTR string, bool convertUTF8)
        {
            CW2A writeString(string, (convertUTF8 ? CP_UTF8 : CP_ACP));
            save(fileName, writeString);
        }
    } // namespace FileSystem
}; // namespace Gek
