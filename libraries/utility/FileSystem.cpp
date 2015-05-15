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

        HRESULT find(LPCWSTR fileName, LPCWSTR filterTypes, bool searchRecursively, std::function<HRESULT(LPCWSTR)> onFileFound)
        {
            HRESULT resultValue = S_OK;

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
                            resultValue = find((expandedFileName + findData.cFileName), filterTypes, searchRecursively, onFileFound);
                        }
                    }
                    else
                    {
                        resultValue = onFileFound(expandedFileName + findData.cFileName);
                    }

                    if (FAILED(resultValue))
                    {
                        break;
                    }
                } while (FindNextFile(findHandle, &findData));

                FindClose(findHandle);
            }

            return resultValue;
        }

        HMODULE loadLibrary(LPCWSTR fileName)
        {
            return LoadLibraryW(expandPath(fileName));
        }

        HRESULT load(LPCWSTR fileName, std::vector<UINT8> &buffer, size_t limitReadSize)
        {
            HRESULT resultValue = E_FAIL;
            CStringW expandedFileName(expandPath(fileName));
            HANDLE fileHandle = CreateFile(expandedFileName, GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (fileHandle == INVALID_HANDLE_VALUE)
            {
                resultValue = E_FAIL;
            }
            else
            {
                DWORD fileSize = GetFileSize(fileHandle, nullptr);
                if (fileSize == 0)
                {
                    resultValue = S_OK;
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
                            resultValue = (bytesRead == buffer.size() ? S_OK : E_FAIL);
                        }
                        else
                        {
                            resultValue = E_FAIL;
                        }
                    }
                    else
                    {
                        resultValue = E_OUTOFMEMORY;
                    }
                }

                CloseHandle(fileHandle);
            }

            return resultValue;
        }

        HRESULT load(LPCWSTR fileName, CStringA &string)
        {
            std::vector<UINT8> buffer;
            HRESULT resultValue = load(fileName, buffer);
            if (SUCCEEDED(resultValue))
            {
                buffer.push_back('\0');
                string = LPCSTR(buffer.data());
            }

            return resultValue;
        }

        HRESULT load(LPCWSTR fileName, CStringW &string, bool convertUTF8)
        {
            CStringA readString;
            HRESULT resultValue = load(fileName, readString);
            if (SUCCEEDED(resultValue))
            {
                string = CA2W(readString, (convertUTF8 ? CP_UTF8 : CP_ACP));
            }

            return resultValue;
        }

        HRESULT save(LPCWSTR fileName, const std::vector<UINT8> &buffer)
        {
            HRESULT resultValue = E_FAIL;
            CStringW expandedFileName(expandPath(fileName));
            HANDLE fileHandle = CreateFile(expandedFileName, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (fileHandle != INVALID_HANDLE_VALUE)
            {
                DWORD bytesWritten = 0;
                WriteFile(fileHandle, buffer.data(), buffer.size(), &bytesWritten, nullptr);
                resultValue = (bytesWritten == buffer.size() ? S_OK : E_FAIL);
                CloseHandle(fileHandle);
            }

            return resultValue;
        }

        HRESULT save(LPCWSTR fileName, LPCSTR string)
        {
            HRESULT resultValue = E_FAIL;
            UINT32 stringLength = strlen(string);
            std::vector<UINT8> buffer(stringLength);
            if (buffer.size() == stringLength)
            {
                memcpy(buffer.data(), string, stringLength);
                resultValue = save(fileName, buffer);
            }

            return resultValue;
        }

        HRESULT save(LPCWSTR fileName, LPCWSTR string, bool convertUTF8)
        {
            CStringA writeString = CW2A(string, (convertUTF8 ? CP_UTF8 : CP_ACP));
            return save(fileName, writeString);
        }
    } // namespace FileSystem
}; // namespace Gek
