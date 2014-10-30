#pragma once

#include "GEKMath.h"
#include <assert.h>
#include <windows.h>
#include <atlbase.h>
#include <atlstr.h>
#include <functional>
#include <vector>
#include <list>
#include <map>

#pragma warning(disable:4251)

#define E_INVALID                       _HRESULT_TYPEDEF_(0x8007A001L)
#define E_FILENOTFOUND                  _HRESULT_TYPEDEF_(0x8007A001L)
#define E_ARCHIVENOTFOUND               _HRESULT_TYPEDEF_(0x8007A002L)

#define GEKDELETE(X)                    do { if (X) delete [] (X); (X) = nullptr; } while (false)
#define GEKRELEASE(X)                   do { if (X) delete (X); (X) = nullptr; } while (false)
#define COMRELEASE(X)                   do { if (X) (X)->Release(); (X) = nullptr; } while (false)

#define REQUIRE_VOID_RETURN(CHECK)      do { if ((CHECK) == 0) { _ASSERTE(CHECK); return; } } while (0)
#define REQUIRE_RETURN(CHECK, RETURN)   do { if ((CHECK) == 0) { _ASSERTE(CHECK); return (RETURN); } } while (0)

enum GEKMODEASPECT
{
    _ASPECT_INVALID = -1,
    _ASPECT_4x3 = 0,
    _ASPECT_16x9 = 1,
    _ASPECT_16x10 = 2,
};

struct GEKMODE
{
public:
    UINT32 xsize;
    UINT32 ysize;

public:
    GEKMODEASPECT GetAspect(void);
};

std::map<UINT32, std::vector<GEKMODE>> GEKGetDisplayModes(void);

namespace std
{
    template<typename CharType, typename TraitsType>
    struct hash<ATL::CStringT<CharType, TraitsType>> : public unary_function<ATL::CStringT<CharType, TraitsType>, size_t>
    {
        size_t operator()(const ATL::CStringT<CharType, TraitsType> &strString) const
        {
            return CStringElementTraits<typename TraitsType>::Hash(strString);
        }
    };

    template<typename CharType, typename TraitsType>
    struct equal_to<ATL::CStringT<CharType, TraitsType>> : public unary_function<ATL::CStringT<CharType, TraitsType>, bool>
    {
        bool operator()(const ATL::CStringT<CharType, TraitsType> &strStringA, const ATL::CStringT<CharType, TraitsType> &strStringB) const
        {
            return (strStringA == strStringB);
        }
    };

    template <>
    struct hash<GUID> : public unary_function<GUID, size_t>
    {
        size_t operator()(REFGUID kGUID) const
        {
            DWORD *pLast = (DWORD *)&kGUID.Data4[0];
            return (kGUID.Data1 ^ (kGUID.Data2 << 16 | kGUID.Data3) ^ (pLast[0] | pLast[1]));
        }
    };

    template <>
    struct equal_to<GUID> : public unary_function<GUID, bool>
    {
        bool operator()(REFGUID kGUIDA, REFGUID kGUIDB) const
        {
            return (memcmp(&kGUIDA, &kGUIDB, sizeof(GUID)) == 0);
        }
    };
};

__forceinline
bool operator < (REFGUID kGUIDA, REFGUID kGUIDB)
{
    return (memcmp(&kGUIDA, &kGUIDB, sizeof(GUID)) < 0);
}

#include "Include\CGEKTimer.h"
#include "Include\CGEKParser.h"
#include "Include\CGEKConfig.h"
#include "Include\CGEKLibXML2.h"

bool EvaluateDouble(LPCWSTR pExpression,     double &nValue);
bool EvaluateFloat(LPCWSTR pExpression,      float &nValue);
bool EvaluateFloat2(LPCWSTR pExpression,     float2 &nValue);
bool EvaluateFloat3(LPCWSTR pExpression,     float3 &nValue);
bool EvaluateFloat4(LPCWSTR pExpression,     float4 &nValue);
bool EvaluateQuaternion(LPCWSTR pExpression, quaternion &nValue);
bool EvaluateINT32(LPCWSTR pExpression,      INT32 &nValue);
bool EvaluateUINT32(LPCWSTR pExpression,     UINT32 &nValue);
bool EvaluateINT64(LPCWSTR pExpression,      INT64 &nValue);
bool EvaluateUINT64(LPCWSTR pExpression,     UINT64 &nValue);
bool EvaluateBoolean(LPCWSTR pExpression,    bool &nValue);

double      StrToDouble(LPCWSTR pExpression);
float       StrToFloat(LPCWSTR pExpression);
float2      StrToFloat2(LPCWSTR pExpression);
float3      StrToFloat3(LPCWSTR pExpression);
float4      StrToFloat4(LPCWSTR pExpression);
quaternion  StrToQuaternion(LPCWSTR pExpression);
INT32       StrToINT32(LPCWSTR pExpression);
UINT32      StrToUINT32(LPCWSTR pExpression);
INT64       StrToINT64(LPCWSTR pExpression);
UINT64      StrToUINT64(LPCWSTR pExpression);
bool        StrToBoolean(LPCWSTR pExpression);

CStringA    FormatString(LPCSTR pFormat, ...);
CStringW    FormatString(LPCWSTR pFormat, ...);

CStringW GEKParseFileName(LPCWSTR pFileName);

HRESULT GEKFindFiles(LPCWSTR pBasePath, LPCWSTR pFilter, bool bRecursive, std::function<HRESULT (LPCWSTR pFileName)> Callback);

HMODULE GEKLoadLibrary(LPCWSTR pFileName);

HRESULT GEKLoadFromFile(LPCWSTR pFileName, std::vector<UINT8> &aBuffer, UINT32 nSize = -1);
HRESULT GEKLoadFromFile(LPCWSTR pFileName, CStringA &strString);
HRESULT GEKLoadFromFile(LPCWSTR pFileName, CStringW &strString, bool bConvertFromUTF8 = true);

HRESULT GEKSaveToFile(LPCWSTR pFileName, const std::vector<UINT8> &aBuffer);
HRESULT GEKSaveToFile(LPCWSTR pFileName, LPCSTR pString);
HRESULT GEKSaveToFile(LPCWSTR pFileName, LPCWSTR pString, bool bConvertToUTF8 = true);
