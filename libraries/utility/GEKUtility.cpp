#include "GEKUtility.h"
#include <atlpath.h>
#include <stdio.h>
#include <time.h>
#include <list>
#include <algorithm>
#include <random>

#ifdef min
#undef min
#undef max
#endif

#include "exprtk.hpp"

static std::random_device kRandomDevice;
static std::mt19937 kMersine(kRandomDevice());

template <typename TYPE>
struct CGEKRandomValue : public exprtk::ifunction<TYPE>
{
    std::uniform_real_distribution<TYPE> m_kRandom;
    CGEKRandomValue() : exprtk::ifunction<TYPE>(1)
                      , m_kRandom(TYPE(0), TYPE(1))
    {
    }

    inline TYPE operator()(const TYPE& nRange)
    {
        TYPE nValue = m_kRandom(kMersine);
        return ((nValue * nRange * TYPE(2)) - nRange);
    }
};

template <typename TYPE>
struct CGEKRandomRange : public exprtk::ifunction<TYPE>
{
    std::uniform_real_distribution<TYPE> m_kRandom;
    CGEKRandomRange() : exprtk::ifunction<TYPE>(2)
                      , m_kRandom(TYPE(0), TYPE(1))
    {
    }

    inline TYPE operator()(const TYPE& nMinimum, const TYPE& nMaximum)
    {
        TYPE nValue = m_kRandom(kMersine);
        return ((nValue * (nMaximum - nMinimum)) + nMinimum);
    }
};

static CGEKRandomValue<float> gs_kRandomValueFloat;
static CGEKRandomRange<float> gs_kRandomRangeFloat;
static exprtk::symbol_table<float> gs_kSymbolTableFloat;
static CGEKRandomValue<double> gs_kRandomValueDouble;
static CGEKRandomRange<double> gs_kRandomRangeDouble;
static exprtk::symbol_table<double> gs_kSymbolTableDouble;
void InitSymbolTables(void)
{
    static bool bInitialized = false;
    if (!bInitialized)
    {
        bInitialized = true;
        gs_kSymbolTableDouble.add_function("rvalue", gs_kRandomValueDouble);
        gs_kSymbolTableDouble.add_function("rrange", gs_kRandomRangeDouble);
        gs_kSymbolTableDouble.add_constants();

        gs_kSymbolTableFloat.add_function("rvalue", gs_kRandomValueFloat);
        gs_kSymbolTableFloat.add_function("rrange", gs_kRandomRangeFloat);
        gs_kSymbolTableFloat.add_constants();
    }
}

static __forceinline float ClipFloat(float nValue)
{
    INT32 nInteger = INT32(nValue * 100.0f);
    return (float(nInteger) / 100.0f);
}

GEKMODEASPECT GEKMODE::GetAspect(void)
{
    const float fAspect4x3 = ClipFloat(4.0f / 3.0f);
    const float fAspect16x9 = ClipFloat(16.0f / 9.0f);
    const float fAspect16x10 = ClipFloat(16.0f / 10.0f);
    float nAspect = ClipFloat(float(xsize) / float(ysize));
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
        for (UINT32 nSize = 0; nSize < akModes.size(); ++nSize)
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

bool EvaluateDouble(LPCWSTR pValue, double &nValue)
{
    InitSymbolTables();
    exprtk::expression<double> kExpression;
    kExpression.register_symbol_table(gs_kSymbolTableDouble);

    exprtk::parser<double> kParser;
    if (kParser.compile(CW2A(pValue).m_psz, kExpression))
    {
        nValue = kExpression.value();
        return true;
    }
    else
    {
        return false;
    }
}

bool EvaluateFloat(LPCWSTR pValue, float &nValue)
{
    InitSymbolTables();
    exprtk::expression<float> kExpression;
    kExpression.register_symbol_table(gs_kSymbolTableFloat);

    exprtk::parser<float> kParser;
    if (kParser.compile(CW2A(pValue).m_psz, kExpression))
    {
        nValue =  kExpression.value();
        return true;
    }
    else
    {
        return false;
    }
}

bool EvaluateFloat2(LPCWSTR pValue, float2 &nValue)
{
    InitSymbolTables();
    exprtk::expression<float> kExpression;
    gs_kSymbolTableFloat.add_vector("output", nValue.xy);
    kExpression.register_symbol_table(gs_kSymbolTableFloat);

    exprtk::parser<float> kParser;
    if (kParser.compile(FormatString("var result[2] := {%S}; output := result;", pValue).GetString(), kExpression))
    {
        kExpression.value();
        gs_kSymbolTableFloat.remove_vector("output");
        return true;
    }
    else
    {
        gs_kSymbolTableFloat.remove_vector("output");
        return false;
    }
}

bool EvaluateFloat3(LPCWSTR pValue, float3 &nValue)
{
    InitSymbolTables();
    exprtk::expression<float> kExpression;
    gs_kSymbolTableFloat.add_vector("output", nValue.xyz);
    kExpression.register_symbol_table(gs_kSymbolTableFloat);

    exprtk::parser<float> kParser;
    if (kParser.compile(FormatString("var result[3] := {%S}; output := result;", pValue).GetString(), kExpression))
    {
        kExpression.value();
        gs_kSymbolTableFloat.remove_vector("output");
        return true;
    }
    else
    {
        gs_kSymbolTableFloat.remove_vector("output");
        return false;
    }
}

bool EvaluateFloat4(LPCWSTR pValue, float4 &nValue)
{
    InitSymbolTables();
    exprtk::expression<float> kExpression;
    gs_kSymbolTableFloat.add_vector("output", nValue.xyzw);
    kExpression.register_symbol_table(gs_kSymbolTableFloat);

    exprtk::parser<float> kParser;
    if (kParser.compile(FormatString("var result[4] := {%S}; output := result;", pValue).GetString(), kExpression))
    {
        kExpression.value();
        gs_kSymbolTableFloat.remove_vector("output");
        return true;
    }
    else
    {
        gs_kSymbolTableFloat.remove_vector("output");
        return false;
    }
}

bool EvaluateQuaternion(LPCWSTR pValue, quaternion &nValue)
{
    if (EvaluateFloat4(pValue, *(float4 *)&nValue))
    {
        return true;
    }

    float3 nEuler;
    if (EvaluateFloat3(pValue, nEuler))
    {
        nValue.SetEuler(nEuler);
        return true;
    }

    return false;
}

bool EvaluateINT32(LPCWSTR pValue, INT32 &nValue)
{
    float nFloatValue = 0.0f;
    if (EvaluateFloat(pValue, nFloatValue))
    {
        nValue = INT32(nFloatValue);
        return true;
    }
    else
    {
        return false;
    }
}

bool EvaluateUINT32(LPCWSTR pValue, UINT32 &nValue)
{
    float nFloatValue = 0.0f;
    if (EvaluateFloat(pValue, nFloatValue))
    {
        nValue = UINT32(nFloatValue);
        return true;
    }
    else
    {
        return false;
    }
}

bool EvaluateINT64(LPCWSTR pValue, INT64 &nValue)
{
    double nDoubleValue = 0.0;
    if (EvaluateDouble(pValue, nDoubleValue))
    {
        nValue = INT64(nDoubleValue);
        return true;
    }
    else
    {
        return false;
    }
}

bool EvaluateUINT64(LPCWSTR pValue, UINT64 &nValue)
{
    double nDoubleValue = 0.0;
    if (EvaluateDouble(pValue, nDoubleValue))
    {
        nValue = UINT64(nDoubleValue);
        return true;
    }
    else
    {
        return false;
    }
}

bool EvaluateBoolean(LPCWSTR pValue, bool &nValue)
{
    if (_wcsicmp(pValue, L"true") == 0 ||
        _wcsicmp(pValue, L"yes") == 0)
    {
        return true;
    }

    INT32 nIntValue = 0;
    if (EvaluateINT32(pValue, nIntValue))
    {
        nValue = (nIntValue == 0 ? false : true);
        return true;
    }
    else
    {
        return false;
    }
}

double StrToDouble(LPCWSTR pValue)
{
    double nValue = 0.0;
    EvaluateDouble(pValue, nValue);
    return nValue;
}

float StrToFloat(LPCWSTR pValue)
{
    float nValue = 0.0f;
    EvaluateFloat(pValue, nValue);
    return nValue;
}

float2 StrToFloat2(LPCWSTR pValue)
{
    float2 nValue;
    EvaluateFloat2(pValue, nValue);
    return nValue;
}

float3 StrToFloat3(LPCWSTR pValue)
{
    float3 nValue;
    EvaluateFloat3(pValue, nValue);
    return nValue;
}

float4 StrToFloat4(LPCWSTR pValue)
{
    float4 nValue;
    EvaluateFloat4(pValue, nValue);
    return nValue;
}

quaternion StrToQuaternion(LPCWSTR pValue)
{
    quaternion nValue;
    EvaluateQuaternion(pValue, nValue);
    return nValue;
}

INT32 StrToINT32(LPCWSTR pValue)
{
    INT32 nValue = 0;
    EvaluateINT32(pValue, nValue);
    return nValue;
}

UINT32 StrToUINT32(LPCWSTR pValue)
{
    UINT32 nValue = 0;
    EvaluateUINT32(pValue, nValue);
    return nValue;
}

INT64 StrToINT64(LPCWSTR pValue)
{
    INT64 nValue = 0;
    EvaluateINT64(pValue, nValue);
    return nValue;
}

UINT64 StrToUINT64(LPCWSTR pValue)
{
    UINT64 nValue = 0;
    EvaluateUINT64(pValue, nValue);
    return nValue;
}

bool StrToBoolean(LPCWSTR pValue)
{
    bool nValue = false;
    EvaluateBoolean(pValue, nValue);
    return nValue;
}

