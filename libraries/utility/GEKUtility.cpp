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
struct CGEKRandom : public exprtk::ifunction<TYPE>
{
    std::uniform_real_distribution<TYPE> m_kRandom;
    CGEKRandom(TYPE nMin, TYPE nMax) 
        : exprtk::ifunction<TYPE>(1)
        , m_kRandom(nMin, nMax)
    {
    }

    inline TYPE operator()(const TYPE& nRange)
    {
        return (nRange * m_kRandom(kMersine));
    }
};

template <typename TYPE>
struct CGEKLerp : public exprtk::ifunction<TYPE>
{
    CGEKLerp(void) : exprtk::ifunction<TYPE>(3)
    {
    }

    inline TYPE operator()(const TYPE& nMinimum, const TYPE& nMaximum, const TYPE& nFactor)
    {
        return ((nFactor * (nMaximum - nMinimum)) + nMinimum);
    }
};

template <typename TYPE>
class TGEKEvaluateValue
{
private:
    CGEKRandom<TYPE> m_kRand;
    CGEKRandom<TYPE> m_kAbsRand;
    CGEKLerp<TYPE> m_kLerp;
    exprtk::symbol_table<TYPE> m_kSymbolTable;
    exprtk::expression<TYPE> m_kExpression;
    exprtk::parser<TYPE> m_kParser;

public:
    TGEKEvaluateValue(void)
        : m_kRand(TYPE(-1), TYPE(1))
        , m_kAbsRand(TYPE(0), TYPE(1))
    {
        m_kSymbolTable.add_function("rand", m_kRand);
        m_kSymbolTable.add_function("arand", m_kAbsRand);
        m_kSymbolTable.add_function("lerp", m_kLerp);

        m_kSymbolTable.add_constants();
        m_kExpression.register_symbol_table(m_kSymbolTable);
    }

    bool EvaluateValue(LPCWSTR pExpression, TYPE &nValue)
    {
        m_kSymbolTable.remove_vector("value");
        if (m_kParser.compile(CW2A(pExpression).m_psz, m_kExpression))
        {
            nValue = m_kExpression.value();
            return true;
        }
        else
        {
            return false;
        }
    }

    template <std::size_t SIZE>
    bool EvaluateVector(LPCWSTR pExpression, TYPE (&aValue)[SIZE])
    {
        m_kSymbolTable.remove_vector("value");
        m_kSymbolTable.add_vector("value", aValue);
        if (m_kParser.compile(FormatString("var vector[%d] := {%S}; value := vector;", SIZE, pExpression).GetString(), m_kExpression))
        {
            m_kExpression.value();
            return true;
        }
        else
        {
            return false;
        }
    }
};

static TGEKEvaluateValue<float> gs_kEvalulateFloat;
static TGEKEvaluateValue<double> gs_kEvalulateDouble;

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

bool EvaluateDouble(LPCWSTR pExpression, double &nValue)
{
    return gs_kEvalulateDouble.EvaluateValue(pExpression, nValue);
}

bool EvaluateFloat(LPCWSTR pExpression, float &nValue)
{
    return gs_kEvalulateFloat.EvaluateValue(pExpression, nValue);
}

bool EvaluateFloat2(LPCWSTR pExpression, float2 &nValue)
{
    return gs_kEvalulateFloat.EvaluateVector(pExpression, nValue.xy);
}

bool EvaluateFloat3(LPCWSTR pExpression, float3 &nValue)
{
    return gs_kEvalulateFloat.EvaluateVector(pExpression, nValue.xyz);
}

bool EvaluateFloat4(LPCWSTR pExpression, float4 &nValue)
{
    if (gs_kEvalulateFloat.EvaluateVector(pExpression, nValue.xyzw))
    {
        return true;
    }

    float3 nPartValue;
    if (EvaluateFloat3(pExpression, nPartValue))
    {
        nValue = float4(nPartValue, 1.0f);
        return true;
    }

    return false;
}

bool EvaluateQuaternion(LPCWSTR pExpression, quaternion &nValue)
{
    if (EvaluateFloat4(pExpression, *(float4 *)&nValue))
    {
        return true;
    }

    float3 nEuler;
    if (EvaluateFloat3(pExpression, nEuler))
    {
        nValue.SetEuler(nEuler);
        return true;
    }

    return false;
}

bool EvaluateINT32(LPCWSTR pExpression, INT32 &nValue)
{
    float nFloatValue = 0.0f;
    if (EvaluateFloat(pExpression, nFloatValue))
    {
        nValue = INT32(nFloatValue);
        return true;
    }
    else
    {
        return false;
    }
}

bool EvaluateUINT32(LPCWSTR pExpression, UINT32 &nValue)
{
    float nFloatValue = 0.0f;
    if (EvaluateFloat(pExpression, nFloatValue))
    {
        nValue = UINT32(nFloatValue);
        return true;
    }
    else
    {
        return false;
    }
}

bool EvaluateINT64(LPCWSTR pExpression, INT64 &nValue)
{
    double nDoubleValue = 0.0;
    if (EvaluateDouble(pExpression, nDoubleValue))
    {
        nValue = INT64(nDoubleValue);
        return true;
    }
    else
    {
        return false;
    }
}

bool EvaluateUINT64(LPCWSTR pExpression, UINT64 &nValue)
{
    double nDoubleValue = 0.0;
    if (EvaluateDouble(pExpression, nDoubleValue))
    {
        nValue = UINT64(nDoubleValue);
        return true;
    }
    else
    {
        return false;
    }
}

bool EvaluateBoolean(LPCWSTR pExpression, bool &nValue)
{
    if (_wcsicmp(pExpression, L"true") == 0 ||
        _wcsicmp(pExpression, L"yes") == 0)
    {
        return true;
    }

    INT32 nIntValue = 0;
    if (EvaluateINT32(pExpression, nIntValue))
    {
        nValue = (nIntValue == 0 ? false : true);
        return true;
    }
    else
    {
        return false;
    }
}

double StrToDouble(LPCWSTR pExpression)
{
    double nValue = 0.0;
    EvaluateDouble(pExpression, nValue);
    return nValue;
}

float StrToFloat(LPCWSTR pExpression)
{
    float nValue = 0.0f;
    EvaluateFloat(pExpression, nValue);
    return nValue;
}

float2 StrToFloat2(LPCWSTR pExpression)
{
    float2 nValue;
    EvaluateFloat2(pExpression, nValue);
    return nValue;
}

float3 StrToFloat3(LPCWSTR pExpression)
{
    float3 nValue;
    EvaluateFloat3(pExpression, nValue);
    return nValue;
}

float4 StrToFloat4(LPCWSTR pExpression)
{
    float4 nValue;
    EvaluateFloat4(pExpression, nValue);
    return nValue;
}

quaternion StrToQuaternion(LPCWSTR pExpression)
{
    quaternion nValue;
    EvaluateQuaternion(pExpression, nValue);
    return nValue;
}

INT32 StrToINT32(LPCWSTR pExpression)
{
    INT32 nValue = 0;
    EvaluateINT32(pExpression, nValue);
    return nValue;
}

UINT32 StrToUINT32(LPCWSTR pExpression)
{
    UINT32 nValue = 0;
    EvaluateUINT32(pExpression, nValue);
    return nValue;
}

INT64 StrToINT64(LPCWSTR pExpression)
{
    INT64 nValue = 0;
    EvaluateINT64(pExpression, nValue);
    return nValue;
}

UINT64 StrToUINT64(LPCWSTR pExpression)
{
    UINT64 nValue = 0;
    EvaluateUINT64(pExpression, nValue);
    return nValue;
}

bool StrToBoolean(LPCWSTR pExpression)
{
    bool nValue = false;
    EvaluateBoolean(pExpression, nValue);
    return nValue;
}

