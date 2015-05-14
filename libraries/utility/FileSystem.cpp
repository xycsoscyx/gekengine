#include "Utility\FileSystem.h"
#include <atlbase.h>
#include <atlpath.h>

namespace Gek
{
    namespace FileSystem
    {
        CStringW expandPath(LPCWSTR basePath)
        {
            CStringW fullPath(basePath);
            if (fullPath.Find(L"%root%") >= 0)
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

                fullPath.Replace(L"%root%", currentModulePath);
            }

            fullPath.Replace(L"/", L"\\");
            return fullPath;
        }

        HRESULT find(LPCWSTR basePath, LPCWSTR filterTypes, bool searchRecursively, std::function<HRESULT(LPCWSTR)> onFileFound)
        {
            HRESULT returnValue = S_OK;

            CStringW fullPath(expandPath(basePath));
            PathAddBackslashW(fullPath.GetBuffer(MAX_PATH + 1));
            fullPath.ReleaseBuffer();

            WIN32_FIND_DATA findData;
            CStringW fullPathFilter(fullPath + filterTypes);
            HANDLE findHandle = FindFirstFile(fullPathFilter, &findData);
            if (findHandle != INVALID_HANDLE_VALUE)
            {
                do
                {
                    if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                    {
                        if (searchRecursively && findData.cFileName[0] != L'.')
                        {
                            returnValue = find((fullPath + findData.cFileName), filterTypes, searchRecursively, onFileFound);
                        }
                    }
                    else
                    {
                        returnValue = onFileFound(fullPath + findData.cFileName);
                    }

                    if (FAILED(returnValue))
                    {
                        break;
                    }
                } while (FindNextFile(findHandle, &findData));

                FindClose(findHandle);
            }

            return returnValue;
        }

        HMODULE loadLibrary(LPCWSTR basePath)
        {
            return LoadLibraryW(expandPath(basePath));
        }

        HRESULT load(LPCWSTR basePath, std::vector<UINT8> &buffer, size_t limitReadSize)
        {
            HRESULT returnValue = E_FAIL;
            CStringW fullPath(expandPath(basePath));
            HANDLE fileHandle = CreateFile(fullPath, GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (fileHandle == INVALID_HANDLE_VALUE)
            {
                returnValue = E_FAIL;
            }
            else
            {
                DWORD fileSize = GetFileSize(fileHandle, nullptr);
                if (fileSize == 0)
                {
                    returnValue = S_OK;
                }
                else
                {
                    if (limitReadSize > 0)
                    {
                        buffer.resize(limitReadSize);
                    }
                    else
                    {
                        buffer.resize(fileSize);
                    }

                    if (!buffer.empty())
                    {
                        DWORD bytesRead = 0;
                        if (ReadFile(fileHandle, buffer.data(), buffer.size(), &bytesRead, nullptr))
                        {
                            returnValue = (bytesRead == buffer.size() ? S_OK : E_FAIL);
                        }
                        else
                        {
                            returnValue = E_FAIL;
                        }
                    }
                    else
                    {
                        returnValue = E_OUTOFMEMORY;
                    }
                }

                CloseHandle(fileHandle);
            }

            return returnValue;
        }

        HRESULT load(LPCWSTR basePath, CStringA &string)
        {
            std::vector<UINT8> buffer;
            HRESULT returnValue = load(basePath, buffer);
            if (SUCCEEDED(returnValue))
            {
                buffer.push_back('\0');
                string = LPCSTR(buffer.data());
            }

            return returnValue;
        }

        HRESULT load(LPCWSTR basePath, CStringW &string, bool convertUTF8)
        {
            CStringA readString;
            HRESULT returnValue = load(basePath, readString);
            if (SUCCEEDED(returnValue))
            {
                string = CA2W(readString, (convertUTF8 ? CP_UTF8 : CP_ACP));
            }

            return returnValue;
        }

        HRESULT GEKSaveToFile(LPCWSTR basePath, const std::vector<UINT8> &buffer)
        {
            HRESULT returnValue = E_FAIL;
            CStringW fullPath(expandPath(basePath));
            HANDLE fileHandle = CreateFile(fullPath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (fileHandle != INVALID_HANDLE_VALUE)
            {
                DWORD bytesWritten = 0;
                WriteFile(fileHandle, buffer.data(), buffer.size(), &bytesWritten, nullptr);
                returnValue = (bytesWritten == buffer.size() ? S_OK : E_FAIL);
                CloseHandle(fileHandle);
            }

            return returnValue;
        }

        HRESULT GEKSaveToFile(LPCWSTR basePath, LPCSTR string)
        {
            HRESULT returnValue = E_FAIL;
            UINT32 stringLength = strlen(string);
            std::vector<UINT8> buffer(stringLength);
            if (buffer.size() == stringLength)
            {
                memcpy(buffer.data(), string, stringLength);
                returnValue = save(basePath, buffer);
            }

            return returnValue;
        }

        HRESULT GEKSaveToFile(LPCWSTR basePath, LPCWSTR string, bool convertUTF8)
        {
            CStringA writeString = CW2A(string, (convertUTF8 ? CP_UTF8 : CP_ACP));
            return save(basePath, writeString);
        }
    } // namespace FileSystem
}; // namespace Gek
