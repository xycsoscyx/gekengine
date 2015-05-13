#pragma once

#include "GEKMath.h"
#include "GEKShape.h"
#include <assert.h>
#include <windows.h>
#include <atlbase.h>
#include <atlstr.h>
#include <functional>
#include <vector>
#include <list>
#include <map>

#pragma warning(disable:4251)
#pragma warning(disable:4996)

#define REQUIRE_VOID_RETURN(CHECK)      do { if ((CHECK) == 0) { _ASSERTE(CHECK); return; } } while (false)
#define REQUIRE_RETURN(CHECK, RETURN)   do { if ((CHECK) == 0) { _ASSERTE(CHECK); return (RETURN); } } while (false)

namespace std
{
    template<typename CHARTYPE, typename TRAITSTYPE>
    struct hash<ATL::CStringT<CHARTYPE, TRAITSTYPE>> : public unary_function<ATL::CStringT<CHARTYPE, TRAITSTYPE>, size_t>
    {
        size_t operator()(const ATL::CStringT<CHARTYPE, TRAITSTYPE> &string) const
        {
            return CStringElementTraits<typename TRAITSTYPE>::Hash(string);
        }
    };

    template<typename CHARTYPE, typename TRAITSTYPE>
    struct equal_to<ATL::CStringT<CHARTYPE, TRAITSTYPE>> : public unary_function<ATL::CStringT<CHARTYPE, TRAITSTYPE>, bool>
    {
        bool operator()(const ATL::CStringT<CHARTYPE, TRAITSTYPE> &leftString, const ATL::CStringT<CHARTYPE, TRAITSTYPE> &rightString) const
        {
            return (leftString == rightString);
        }
    };

    template <>
    struct hash<GUID> : public unary_function<GUID, size_t>
    {
        size_t operator()(REFGUID guid) const
        {
            DWORD *last = (DWORD *)&guid.Data4[0];
            return (guid.Data1 ^ (guid.Data2 << 16 | guid.Data3) ^ (last[0] | last[1]));
        }
    };

    template <>
    struct equal_to<GUID> : public unary_function<GUID, bool>
    {
        bool operator()(REFGUID leftGuid, REFGUID rightGuid) const
        {
            return (memcmp(&leftGuid, &rightGuid, sizeof(GUID)) == 0);
        }
    };
};

__forceinline
bool operator < (REFGUID leftGuid, REFGUID rightGuid)
{
    return (memcmp(&leftGuid, &rightGuid, sizeof(GUID)) < 0);
}

namespace Gek
{
    typedef UINT32 Handle;
    const Handle InvalidHandle = 0;

    template <typename TYPE>
    struct Rectangle
    {
        TYPE left;
        TYPE top;
        TYPE right;
        TYPE bottom;
    };

    namespace Display
    {
        enum AspectRatios
        {
            AspectRatioUnknown = -1,
            AspectRatio4x3 = 0,
            AspectRatio16x9 = 1,
            AspectRatio16x10 = 2,
        };

        struct Mode
        {
            UINT32 width;
            UINT32 height;
            AspectRatios aspectRatio;

            Mode(UINT32 width, UINT32 height, AspectRatios aspectRatio)
                : width(width)
                , height(height)
                , aspectRatio(aspectRatio)
            {
            }
        };

        std::map<UINT32, std::vector<Mode>> getModes(void);
    }; // namespace Display

    namespace Evaluator
    {
        bool getDouble(LPCWSTR expression, double &result);
        bool getFloat(LPCWSTR expression, float &result);
        bool getFloat2(LPCWSTR expression, Gek::Math::Float2 &result);
        bool getFloat3(LPCWSTR expression, Gek::Math::Float3 &result);
        bool getFloat4(LPCWSTR expression, Gek::Math::Float4 &result);
        bool getQuaternion(LPCWSTR expression, Gek::Math::Quaternion &result);
        bool getINT32(LPCWSTR expression, INT32 &result);
        bool getUINT32(LPCWSTR expression, UINT32 &result);
        bool getINT64(LPCWSTR expression, INT64 &result);
        bool getUINT64(LPCWSTR expression, UINT64 &result);
        bool getBoolean(LPCWSTR expression, bool &result);
    }; // namespace Evaluator

    namespace String
    {
        double getDouble(LPCWSTR expression);
        float getFloat(LPCWSTR expression);
        Gek::Math::Float2 getFloat2(LPCWSTR expression);
        Gek::Math::Float3 getFloat3(LPCWSTR expression);
        Gek::Math::Float4 getFloat4(LPCWSTR expression);
        Gek::Math::Quaternion getQuaternion(LPCWSTR expression);
        INT32 getINT32(LPCWSTR expression);
        UINT32 getUINT32(LPCWSTR expression);
        INT64 getINT64(LPCWSTR expression);
        UINT64 getUINT64(LPCWSTR expression);
        bool getBoolean(LPCWSTR expression);

        CStringW setDouble(const double &value);
        CStringW setFloat(const float &value);
        CStringW setFloat2(const Gek::Math::Float2 &value);
        CStringW setFloat3(const Gek::Math::Float3 &value);
        CStringW setFloat4(const Gek::Math::Float4 &value);
        CStringW setQuaternion(const Gek::Math::Quaternion &value);
        CStringW setINT32(const INT32 &value);
        CStringW setUINT32(const UINT32 &value);
        CStringW setINT64(const INT64 &value);
        CStringW setUINT64(const UINT64 &value);
        CStringW setBoolean(const bool &value);

        CStringA format(LPCSTR format, ...);
        CStringW format(LPCWSTR format, ...);
    }; // namespace String

    namespace FileSystem
    {
        CStringW expandPath(LPCWSTR basePath);

        HRESULT find(LPCWSTR basePath, LPCWSTR filterTypes, bool searchRecursively, std::function<HRESULT(LPCWSTR fileName)> onFileFound);

        HMODULE loadLibrary(LPCWSTR fileName);

        HRESULT load(LPCWSTR fileName, std::vector<UINT8> &buffer, size_t limitReadSize = -1);
        HRESULT load(LPCWSTR fileName, CStringA &string);
        HRESULT load(LPCWSTR fileName, CStringW &string, bool convertUTF8 = true);

        HRESULT save(LPCWSTR fileName, const std::vector<UINT8> &buffer);
        HRESULT save(LPCWSTR fileName, LPCSTR pString);
        HRESULT save(LPCWSTR fileName, LPCWSTR pString, bool convertUTF8 = true);
    }; // namespace File
}; // namespace Gek

#include "Include\CGEKTimer.h"
#include "Include\CGEKPerformance.h"
#include "Include\CGEKLibXML2.h"
