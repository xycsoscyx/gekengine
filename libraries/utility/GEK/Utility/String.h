#pragma once

#include "GEK\Math\Vector2.h"
#include "GEK\Math\Vector3.h"
#include "GEK\Math\Vector4.h"
#include "GEK\Math\Color.h"
#include "GEK\Math\Quaternion.h"
#include "GEK\Utility\Hash.h"
#include <Windows.h>
#include <atlbase.h>
#include <atlstr.h>

namespace Gek
{
    namespace String
    {
        bool to(LPCWSTR expression, double &value);
        bool to(LPCWSTR expression, float &value);
        bool to(LPCWSTR expression, Gek::Math::Float2 &value);
        bool to(LPCWSTR expression, Gek::Math::Float3 &value);
        bool to(LPCWSTR expression, Gek::Math::Float4 &value);
        bool to(LPCWSTR expression, Gek::Math::Color &value);
        bool to(LPCWSTR expression, Gek::Math::Quaternion &value);
        bool to(LPCWSTR expression, INT32 &value);
        bool to(LPCWSTR expression, UINT32 &value);
        bool to(LPCWSTR expression, INT64 &value);
        bool to(LPCWSTR expression, UINT64 &value);
        bool to(LPCWSTR expression, bool &value);

        template <typename TYPE>
        TYPE to(LPCWSTR expression)
        {
            TYPE value = {};
            to(expression, value);
            return value;
        }

        CStringW from(double value);
        CStringW from(float value);
        CStringW from(const Gek::Math::Float2 &value);
        CStringW from(const Gek::Math::Float3 &value);
        CStringW from(const Gek::Math::Float4 &value);
        CStringW from(const Gek::Math::Color &value);
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
