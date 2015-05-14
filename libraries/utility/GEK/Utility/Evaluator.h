#pragma once

#include "GEK\Math\Vector2.h"
#include "GEK\Math\Vector3.h"
#include "GEK\Math\Vector4.h"
#include "GEK\Math\Quaternion.h"
#include <Windows.h>

namespace Gek
{
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
}; // namespace Gek
