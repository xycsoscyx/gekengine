#pragma once

#include "GEK\Math\Vector2.h"
#include "GEK\Math\Vector3.h"
#include "GEK\Math\Vector4.h"
#include "GEK\Math\Quaternion.h"
#include <Windows.h>
#include <atlbase.h>
#include <atlstr.h>

namespace Gek
{
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
}; // namespace Gek
