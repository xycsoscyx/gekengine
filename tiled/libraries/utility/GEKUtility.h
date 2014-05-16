#pragma once

#include "GEKMath.h"
#include <assert.h>
#include <windows.h>
#include <atlbase.h>
#include <atlstr.h>
#include <functional>
#include <vector>
#include <list>

#pragma warning(disable:4251)

#define E_INVALID                       _HRESULT_TYPEDEF_(0x8007A001L)
#define E_FILENOTFOUND                  _HRESULT_TYPEDEF_(0x8007A001L)
#define E_ARCHIVENOTFOUND               _HRESULT_TYPEDEF_(0x8007A002L)

#define GEKDELETE(X)                    do { if (X) delete [] (X); (X) = nullptr; } while (false)
#define GEKRELEASE(X)                   do { if (X) delete (X); (X) = nullptr; } while (false)
#define COMRELEASE(X)                   do { if (X) (X)->Release(); (X) = nullptr; } while (false)

#define REQUIRE_VOID_RETURN(CHECK)      do { if ((CHECK) == 0) { _ASSERTE(CHECK); return; } } while (0)
#define REQUIRE_RETURN(CHECK, RETURN)   do { if ((CHECK) == 0) { _ASSERTE(CHECK); return (RETURN); } } while (0)

#include "Include\CGEKTimer.h"
#include "Include\CGEKParser.h"
#include "Include\CGEKConfig.h"
#include "Include\CGEKLibXML2.h"

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

class GEKHASH
{
private:
    std::size_t m_nHash;

public:
    GEKHASH(void);
    GEKHASH(LPCWSTR pString);
    GEKHASH(const CStringW &strString);

    UINT32 GetHash(void) const;

    GEKHASH operator = (LPCWSTR pString);
    GEKHASH operator = (const CStringW &strString);

    bool operator<(const GEKHASH &nValue) const;
    bool operator == (const GEKHASH &nValue) const;
    bool operator == (LPCWSTR pString);
    bool operator == (const CStringW &strString);
};

double      StrToDouble(LPCWSTR pValue);
float       StrToFloat(LPCWSTR pValue);
float2      StrToFloat2(LPCWSTR pValue);
float3      StrToFloat3(LPCWSTR pValue);
float4      StrToFloat4(LPCWSTR pValue);
quaternion  StrToQuaternion(LPCWSTR pValue);
INT32       StrToINT32(LPCWSTR pValue);
UINT32      StrToUINT32(LPCWSTR pValue);
INT64       StrToINT64(LPCWSTR pValue);
UINT64      StrToUINT64(LPCWSTR pValue);
bool        StrToBoolean(LPCWSTR pValue);

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
