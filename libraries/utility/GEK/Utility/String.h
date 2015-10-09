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
        double toDouble(LPCWSTR expression);
        float toFloat(LPCWSTR expression);
        Gek::Math::Float2 toFloat2(LPCWSTR expression);
        Gek::Math::Float3 toFloat3(LPCWSTR expression);
        Gek::Math::Float4 toFloat4(LPCWSTR expression);
        Gek::Math::Quaternion toQuaternion(LPCWSTR expression);
        INT32 toINT32(LPCWSTR expression);
        UINT32 toUINT32(LPCWSTR expression);
        INT64 toINT64(LPCWSTR expression);
        UINT64 toUINT64(LPCWSTR expression);
        bool toBoolean(LPCWSTR expression);

        CStringW from(double value);
        CStringW from(float value);
        CStringW from(const Gek::Math::Float2 &value);
        CStringW from(const Gek::Math::Float3 &value);
        CStringW from(const Gek::Math::Float4 &value);
        CStringW from(const Gek::Math::Quaternion &value);
        CStringW from(INT8 value);
        CStringW from(UINT8 value);
        CStringW from(INT16 value);
        CStringW from(UINT16 value);
        CStringW from(INT32 value);
        CStringW from(UINT32 value);
        CStringW from(DWORD value);
        CStringW from(LPCVOID value);
        CStringW from(INT64 value);
        CStringW from(UINT64 value);
        CStringW from(bool value);
        CStringW from(LPCSTR value, bool fromUTF8 = false);
        CStringW from(LPCWSTR value);

        CStringA format(LPCSTR format, ...);
        CStringW format(LPCWSTR format, ...);
    }; // namespace String
}; // namespace Gek
