#pragma once

#include "GEK\Math\Vector2.h"
#include "GEK\Math\Vector3.h"
#include "GEK\Math\Vector4.h"
#include "GEK\Math\Color.h"
#include "GEK\Math\Quaternion.h"
#include <Windows.h>

namespace Gek
{
    namespace Evaluator
    {
        bool get(LPCWSTR expression, INT32 &result);
        bool get(LPCWSTR expression, UINT32 &result);
        bool get(LPCWSTR expression, INT64 &result);
        bool get(LPCWSTR expression, UINT64 &result);
        bool get(LPCWSTR expression, float &result);
        bool get(LPCWSTR expression, Gek::Math::Float2 &result);
        bool get(LPCWSTR expression, Gek::Math::Float3 &result);
        bool get(LPCWSTR expression, Gek::Math::Float4 &result);
        bool get(LPCWSTR expression, Gek::Math::Color &result);
        bool get(LPCWSTR expression, Gek::Math::Quaternion &result);

        template <typename TYPE>
        TYPE get(LPCWSTR expression)
        {
            TYPE value = {};
            get(expression, value);
            return value;
        }
    }; // namespace Evaluator
}; // namespace Gek
