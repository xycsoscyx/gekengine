#include "GEKUtility.h"
#include <atlpath.h>
#include <stdio.h>
#include <time.h>
#include <list>
#include <algorithm>
#include <random>

float ClipFloat(float fValue, UINT32 nNumDecimals)
{
    CStringW strFormat;
    strFormat.Format(L"%%.%df", nNumDecimals);

    CStringW strValue;
    strValue.Format(strFormat, fValue);
    return StrToFloat(strValue);
}

GEKMODEASPECT GEKMODE::GetAspect(void)
{
    const float fAspect4x3 = ClipFloat((4.0f / 3.0f), 2);
    const float fAspect16x9 = ClipFloat((16.0f / 9.0f), 2);
    const float fAspect16x10 = ClipFloat((16.0f / 10.0f), 2);
    float nAspect = ClipFloat((float(xsize) / float(ysize)), 2);
    if (nAspect == fAspect4x3)
    {
        return _ASPECT_4x3;
    }
    else if (nAspect == fAspect16x9)
    {
        return _ASPECT_16x9;
    }
    else if (nAspect == fAspect16x10)
    {
        return _ASPECT_16x10;
    }
    else
    {
        return _ASPECT_INVALID;
    }
}

std::map<UINT32, std::vector<GEKMODE>> GEKGetDisplayModes(void)
{
    UINT32 iMode = 0;
    DEVMODE kDisplay = { 0 };
    std::map<UINT32, std::vector<GEKMODE>> aDisplayModes;
    while (EnumDisplaySettings(0, iMode++, &kDisplay))
    {
        bool bExists = false;
        std::vector<GEKMODE> &akModes = aDisplayModes[kDisplay.dmBitsPerPel];
        for (UINT32 nSize = 0; nSize < akModes.size(); nSize++)
        {
            if (akModes[nSize].xsize != kDisplay.dmPelsWidth) continue;
            if (akModes[nSize].ysize != kDisplay.dmPelsHeight) continue;
            bExists = true;
            break;
        }

        if (!bExists)
        {
            GEKMODE kMode;
            kMode.xsize = kDisplay.dmPelsWidth;
            kMode.ysize = kDisplay.dmPelsHeight;
            akModes.push_back(kMode);
        }
    };

    return aDisplayModes;
}

static std::hash<std::wstring> gs_nHash;
GEKHASH::GEKHASH(void)
    : m_nHash(0)
{
}

GEKHASH::GEKHASH(LPCWSTR pString)
: m_nHash(gs_nHash(pString))
{
}

GEKHASH::GEKHASH(const CStringW &strString)
: m_nHash(gs_nHash(strString.GetString()))
{
}

UINT32 GEKHASH::GetHash(void) const
{
    return UINT32(m_nHash);
}

GEKHASH GEKHASH::operator = (LPCWSTR pString)
{
    m_nHash = gs_nHash(pString);
    return (*this);
}

GEKHASH GEKHASH::operator = (const CStringW &strString)
{
    m_nHash = gs_nHash(strString.GetString());
    return (*this);
}

bool GEKHASH::operator<(const GEKHASH &nValue) const
{
    return m_nHash < nValue.m_nHash;
}

bool GEKHASH::operator == (const GEKHASH &nValue) const
{
    return m_nHash == nValue.m_nHash;
}

bool GEKHASH::operator == (LPCWSTR pString)
{
    return m_nHash == gs_nHash(pString);
}

bool GEKHASH::operator == (const CStringW &strString)
{
    return m_nHash == gs_nHash(strString.GetString());
}

CStringA FormatString(LPCSTR pFormat, ...)
{
    CStringA strValue;
    if (pFormat != nullptr)
    {
        va_list pArgs;
        va_start(pArgs, pFormat);
        strValue.FormatV(pFormat, pArgs);
        va_end(pArgs);
    }

    return strValue;
}

CStringW FormatString(LPCWSTR pFormat, ...)
{
    CStringW strValue;
    if (pFormat != nullptr)
    {
        va_list pArgs;
        va_start(pArgs, pFormat);
        strValue.FormatV(pFormat, pArgs);
        va_end(pArgs);
    }

    return strValue;
}

CStringW GEKParseFileName(LPCWSTR pFileName)
{
    CStringW strParsed(pFileName);
    if (strParsed.Find(L"%root%") >= 0)
    {
        CStringW strModule;
        GetModuleFileName(nullptr, strModule.GetBuffer(MAX_PATH + 1), MAX_PATH);
        strModule.ReleaseBuffer();

        CPathW strPath;
        CStringW &strAbsoluteModule = strPath;
        GetFullPathName(strModule, MAX_PATH, strAbsoluteModule.GetBuffer(MAX_PATH + 1), nullptr);
        strAbsoluteModule.ReleaseBuffer();

        strPath.RemoveFileSpec();
        strPath.RemoveFileSpec();
        strParsed.Replace(L"%root%", strPath);
    }

    strParsed.Replace(L"/", L"\\");
    return strParsed;
}

HRESULT GEKFindFiles(LPCWSTR pBasePath, LPCWSTR pFilter, bool bRecursive, std::function<HRESULT (LPCWSTR pFileName)> Callback)
{
    HRESULT hRetVal = S_OK;

    CStringW strFullBasePath(GEKParseFileName(pBasePath));
    PathAddBackslashW(strFullBasePath.GetBuffer(MAX_PATH + 1));
    strFullBasePath.ReleaseBuffer();

    WIN32_FIND_DATA kData;
    CStringW strFullSearchPath(strFullBasePath + pFilter);
    HANDLE hFind = FindFirstFile(strFullSearchPath, &kData);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            if (kData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                if (kData.cFileName[0] == '.')
                {
                    continue;
                }
                else if (bRecursive)
                {
                    hRetVal = GEKFindFiles((strFullBasePath + kData.cFileName), pFilter, bRecursive, Callback);
                }
                else
                {
                    continue;
                }
            }
            else
            {
                hRetVal = Callback(strFullBasePath + kData.cFileName);
            }

            if (FAILED(hRetVal))
            {
                break;
            }
        } while (FindNextFile(hFind, &kData));

        FindClose(hFind);
    }

    return hRetVal;
}

HMODULE GEKLoadLibrary(LPCWSTR pFileName)
{
    return LoadLibraryW(GEKParseFileName(pFileName));
}

HRESULT GEKLoadFromFile(LPCWSTR pFileName, std::vector<UINT8> &aBuffer, UINT32 nSize)
{
    HRESULT hRetVal = E_FAIL;
    CStringW strFullFileName(GEKParseFileName(pFileName));
    HANDLE hFile = CreateFile(strFullFileName, GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        hRetVal = E_FILENOTFOUND;
    }
    else
    {
        DWORD nFileSize = GetFileSize(hFile, nullptr);
        if (nFileSize == 0)
        {
            hRetVal = S_OK;
        }
        else
        {
            UINT32 nReadSize = nFileSize;
            if (nSize != -1)
            {
                nReadSize = nSize;
            }

            aBuffer.resize(nReadSize);
            if (aBuffer.size() == nReadSize)
            {
                DWORD nBytesRead = 0;
                if (ReadFile(hFile, &aBuffer[0], nReadSize, &nBytesRead, nullptr))
                {
                    hRetVal = (nBytesRead == nReadSize ? S_OK : E_FAIL);
                }
                else
                {
                    hRetVal = E_FAIL;
                }
            }
            else
            {
                hRetVal = E_OUTOFMEMORY;
            }
        }

        CloseHandle(hFile);
    }

    return hRetVal;
}

HRESULT GEKLoadFromFile(LPCWSTR pFileName, CStringA &strString)
{
    std::vector<UINT8> aBuffer;
    HRESULT hRetVal = GEKLoadFromFile(pFileName, aBuffer);
    if (SUCCEEDED(hRetVal))
    {
        aBuffer.push_back('\0');
        strString = (const char *)&aBuffer[0];
    }

    return hRetVal;
}

HRESULT GEKLoadFromFile(LPCWSTR pFileName, CStringW &strString, bool bConvertFromUTF8)
{
    CStringA strMultiString;
    HRESULT hRetVal = GEKLoadFromFile(pFileName, strMultiString);
    if (SUCCEEDED(hRetVal))
    {
        strString = CA2W(strMultiString, (bConvertFromUTF8 ? CP_UTF8 : CP_ACP));
    }

    return hRetVal;
}

HRESULT GEKSaveToFile(LPCWSTR pFileName, const std::vector<UINT8> &aBuffer)
{
    CStringW strFullFileName(GEKParseFileName(pFileName));
    HANDLE hFile = CreateFile(strFullFileName, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        DWORD nBytesWritten = 0;
        WriteFile(hFile, &aBuffer[0], aBuffer.size(), &nBytesWritten, nullptr);
        CloseHandle(hFile);
    }

    return E_FAIL;
}

HRESULT GEKSaveToFile(LPCWSTR pFileName, LPCSTR pString)
{
    HRESULT hRetVal = E_FAIL;
    UINT32 nSize = strlen(pString);
    std::vector<UINT8> aBuffer(nSize);
    if (aBuffer.size() == nSize)
    {
        memcpy(&aBuffer[0], pString, nSize);
        hRetVal = GEKSaveToFile(pFileName, aBuffer);
    }

    return hRetVal;
}

HRESULT GEKSaveToFile(LPCWSTR pFileName, LPCWSTR pString, bool bConvertToUTF8)
{
    CStringA strMultiString = CW2A(pString, (bConvertToUTF8 ? CP_UTF8 : CP_ACP));
    return GEKSaveToFile(pFileName, strMultiString);
}

static std::random_device kRandomDevice;
static std::mt19937 kMersine(kRandomDevice());
double StrToDouble(LPCWSTR pValue)
{
    static std::uniform_real_distribution<double> kRandom(0.0, 1.0);
    if (_wcsnicmp(pValue, L"rand(", 5) == 0)
    {
        CStringW strRange = pValue;
        strRange.Replace(L"rand", L"");
        strRange.TrimLeft(L'(');
        strRange.TrimRight(L')');
        if (strRange.Find(L":") >= 0)
        {
            int nPosition = 0;
            CStringW strMinimum = strRange.Tokenize(L":", nPosition);
            CStringW strMaximum = strRange.Tokenize(L":", nPosition);
            double nMinimum = wcstod(strMinimum, nullptr);
            double nMaximum = wcstod(strMaximum, nullptr);
            double nValue = kRandom(kMersine);
            return ((nValue * (nMaximum - nMinimum)) + nMinimum);
        }
        else
        {
            double nRange = wcstod(strRange, nullptr);
            double nValue = kRandom(kMersine);
            return ((nValue * nRange * 2.0) - nRange);
        }
    }
    else
    {
        return wcstod(pValue, nullptr);
    }
}

float StrToFloat(LPCWSTR pValue)
{
    static std::uniform_real_distribution<float> kRandom(0.0f, 1.0f);
    if (_wcsnicmp(pValue, L"rand(", 5) == 0)
    {
        CStringW strRange = pValue;
        strRange.Replace(L"rand", L"");
        strRange.TrimLeft(L'(');
        strRange.TrimRight(L')');
        if (strRange.Find(L":") >= 0)
        {
            int nPosition = 0;
            CStringW strMinimum = strRange.Tokenize(L":", nPosition);
            CStringW strMaximum = strRange.Tokenize(L":", nPosition);
            float nMinimum = wcstof(strMinimum, nullptr);
            float nMaximum = wcstof(strMaximum, nullptr);
            float nValue = kRandom(kMersine);
            return ((nValue * (nMaximum - nMinimum)) + nMinimum);
        }
        else
        {
            float nRange = wcstof(strRange, nullptr);
            float nValue = kRandom(kMersine);
            return ((nValue * nRange * 2.0f) - nRange);
        }
    }
    else
    {
        return wcstof(pValue, nullptr);
    }
}


float2 StrToFloat2(LPCWSTR pValue)
{
    float2 nValue(0.0f);
    if (wcschr(pValue, L',') == nullptr)
    {
        nValue = StrToFloat(pValue);
    }
    else
    {
        int nAxis = 0;
        int nPosition = 0;
        CString strValue(pValue);
        CStringW strToken = strValue.Tokenize(L",", nPosition);
        while (!strToken.IsEmpty() && nAxis < 2)
        {
            strToken.Trim();
            nValue[nAxis++] = StrToFloat(strToken);
            strToken = strValue.Tokenize(L",", nPosition);
        };
    }

    return nValue;
}

float3 StrToFloat3(LPCWSTR pValue)
{
    float3 nValue(0.0f);
    if (wcschr(pValue, L',') == nullptr)
    {
        nValue = StrToFloat(pValue);
    }
    else
    {
        int nAxis = 0;
        int nPosition = 0;
        CString strValue(pValue);
        CStringW strToken = strValue.Tokenize(L",", nPosition);
        while (!strToken.IsEmpty() && nAxis < 3)
        {
            strToken.Trim();
            nValue[nAxis++] = StrToFloat(strToken);
            strToken = strValue.Tokenize(L",", nPosition);
        };
    }

    return nValue;
}

float4 StrToFloat4(LPCWSTR pValue)
{
    float4 nValue(0.0f);
    if (wcschr(pValue, L',') == nullptr)
    {
        nValue = StrToFloat(pValue);
    }
    else
    {
        int nAxis = 0;
        int nPosition = 0;
        CString strValue(pValue);
        CStringW strToken = strValue.Tokenize(L",", nPosition);
        while (!strToken.IsEmpty() && nAxis < 4)
        {
            nValue[nAxis++] = StrToFloat(strToken);
            strToken = strValue.Tokenize(L",", nPosition);
        };
    }

    return nValue;
}

quaternion StrToQuaternion(LPCWSTR pValue)
{
    float4 nValue(0.0f);

    int nAxis = 0;
    int nPosition = 0;
    CString strValue(pValue);
    CStringW strToken = strValue.Tokenize(L",", nPosition);
    while (!strToken.IsEmpty() && nAxis < 4)
    {
        nValue[nAxis++] = StrToFloat(strToken);
        strToken = strValue.Tokenize(L",", nPosition);
    };

    quaternion nQuaternion;
    if (nAxis == 4)
    {
        nQuaternion = nValue;
    }
    else
    {
        nQuaternion.SetEuler(nValue);
    }

    return nQuaternion;
}

INT32 StrToINT32(LPCWSTR pValue)
{
    return wcstol(pValue, nullptr, 0);
}

UINT32 StrToUINT32(LPCWSTR pValue)
{
    return wcstoul(pValue, nullptr, 0);
}

INT64 StrToINT64(LPCWSTR pValue)
{
    return _wcstoi64(pValue, nullptr, 0);
}

UINT64 StrToUINT64(LPCWSTR pValue)
{
    return _wcstoui64(pValue, nullptr, 0);
}

bool StrToBoolean(LPCWSTR pValue)
{
    if (_wcsicmp(pValue, L"true") == 0 ||
        _wcsicmp(pValue, L"yes") == 0)
    {
        return true;
    }

    return (StrToINT32(pValue) ? true : false);
}
