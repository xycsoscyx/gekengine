#pragma once

#include "GEK\Math\Vector2.h"
#include "GEK\Math\Vector3.h"
#include "GEK\Math\Vector4.h"
#include "GEK\Math\Color.h"
#include "GEK\Math\Quaternion.h"
#include <Windows.h>
#include <atlbase.h>
#include <atlstr.h>

namespace Gek
{
    namespace Evaluator
    {
        bool get(LPCWSTR expression, INT32 &result, INT32 defaultValue = 0);
        bool get(LPCWSTR expression, UINT32 &result, UINT32 defaultValue = 0);
        bool get(LPCWSTR expression, INT64 &result, INT64 defaultValue = 0);
        bool get(LPCWSTR expression, UINT64 &result, UINT64 defaultValue = 0);
        bool get(LPCWSTR expression, float &result, float defaultValue = 0);
        bool get(LPCWSTR expression, Gek::Math::Float2 &result, const Gek::Math::Float2 &defaultValue = Gek::Math::Float2(0.0f, 0.0f));
        bool get(LPCWSTR expression, Gek::Math::Float3 &result, const Gek::Math::Float3 &defaultValue = Gek::Math::Float3(0.0f, 0.0f, 0.0f));
        bool get(LPCWSTR expression, Gek::Math::Float4 &result, const Gek::Math::Float4 &defaultValue = Gek::Math::Float4(0.0f, 0.0f, 0.0f, 0.0f));
        bool get(LPCWSTR expression, Gek::Math::Color &result, const Gek::Math::Color &defaultValue = Gek::Math::Color(0.0f, 0.0f, 0.0f, 1.0f));
        bool get(LPCWSTR expression, Gek::Math::Quaternion &result, const Gek::Math::Quaternion &defaultValue = Gek::Math::Quaternion::Identity);
        bool get(LPCWSTR expression, CStringW &result);

        template <typename TYPE>
        TYPE get(LPCWSTR expression)
        {
            TYPE value;
            get(expression, value);
            return value;
        }
    }; // namespace Evaluator
}; // namespace Gek
