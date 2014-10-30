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

bool EvaluateDouble(LPCWSTR pValue,     double &nValue);
bool EvaluateFloat(LPCWSTR pValue,      float &nValue);
bool EvaluateFloat2(LPCWSTR pValue,     float2 &nValue);
bool EvaluateFloat3(LPCWSTR pValue,     float3 &nValue);
bool EvaluateFloat4(LPCWSTR pValue,     float4 &nValue);
bool EvaluateQuaternion(LPCWSTR pValue, quaternion &nValue);
bool EvaluateINT32(LPCWSTR pValue,      INT32 &nValue);
bool EvaluateUINT32(LPCWSTR pValue,     UINT32 &nValue);
bool EvaluateINT64(LPCWSTR pValue,      INT64 &nValue);
bool EvaluateUINT64(LPCWSTR pValue,     UINT64 &nValue);
bool EvaluateBoolean(LPCWSTR pValue,    bool &nValue);

__forceinline double      StrToDouble(LPCWSTR pValue)       { double nValue = 0.0;  EvaluateDouble(pValue, nValue);     return nValue; }
__forceinline float       StrToFloat(LPCWSTR pValue)        { float nValue = 0.0f;  EvaluateFloat(pValue, nValue);      return nValue; }
__forceinline float2      StrToFloat2(LPCWSTR pValue)       { float2 nValue;        EvaluateFloat2(pValue, nValue);     return nValue; }
__forceinline float3      StrToFloat3(LPCWSTR pValue)       { float3 nValue;        EvaluateFloat3(pValue, nValue);     return nValue; }
__forceinline float4      StrToFloat4(LPCWSTR pValue)       { float4 nValue;        EvaluateFloat4(pValue, nValue);     return nValue; }
__forceinline quaternion  StrToQuaternion(LPCWSTR pValue)   { quaternion nValue;    float3 nEuler;        if (EvaluateFloat3(pValue, nEuler)) { nValue.SetEuler(nEuler); } else EvaluateQuaternion(pValue, nValue); return nValue; }
__forceinline INT32       StrToINT32(LPCWSTR pValue)        { INT32 nValue = 0;     EvaluateINT32(pValue, nValue);      return nValue; }
__forceinline UINT32      StrToUINT32(LPCWSTR pValue)       { UINT32 nValue = 0;    EvaluateUINT32(pValue, nValue);     return nValue; }
__forceinline INT64       StrToINT64(LPCWSTR pValue)        { INT64 nValue = 0;     EvaluateINT64(pValue, nValue);      return nValue; }
__forceinline UINT64      StrToUINT64(LPCWSTR pValue)       { UINT64 nValue = 0;    EvaluateUINT64(pValue, nValue);     return nValue; }
__forceinline bool        StrToBoolean(LPCWSTR pValue)      { bool nValue = false;  EvaluateBoolean(pValue, nValue);    return nValue; }

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
