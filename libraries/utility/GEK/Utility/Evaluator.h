#pragma once

#include "GEK\Math\Float2.h"
#include "GEK\Math\Float3.h"
#include "GEK\Math\Float4.h"
#include "GEK\Math\Color.h"
#include "GEK\Math\Quaternion.h"
#include "GEK\Utility\String.h"

namespace Gek
{
    namespace Evaluator
    {
        void get(const wchar_t *expression, INT32 &result, INT32 defaultValue = 0);
        void get(const wchar_t *expression, UINT32 &result, UINT32 defaultValue = 0);
        void get(const wchar_t *expression, INT64 &result, INT64 defaultValue = 0);
        void get(const wchar_t *expression, UINT64 &result, UINT64 defaultValue = 0);
        void get(const wchar_t *expression, float &result, float defaultValue = 0);
        void get(const wchar_t *expression, Gek::Math::Float2 &result, const Gek::Math::Float2 &defaultValue = Gek::Math::Float2(0.0f, 0.0f));
        void get(const wchar_t *expression, Gek::Math::Float3 &result, const Gek::Math::Float3 &defaultValue = Gek::Math::Float3(0.0f, 0.0f, 0.0f));
        void get(const wchar_t *expression, Gek::Math::Float4 &result, const Gek::Math::Float4 &defaultValue = Gek::Math::Float4(0.0f, 0.0f, 0.0f, 0.0f));
        void get(const wchar_t *expression, Gek::Math::Color &result, const Gek::Math::Color &defaultValue = Gek::Math::Color(0.0f, 0.0f, 0.0f, 1.0f));
        void get(const wchar_t *expression, Gek::Math::Quaternion &result, const Gek::Math::Quaternion &defaultValue = Gek::Math::Quaternion::Identity);
        void get(const wchar_t *expression, wstring &result);

        template <typename TYPE>
        TYPE get(const wchar_t *expression)
        {
            TYPE value;
            get(expression, value);
            return value;
        }
    }; // namespace Evaluator
}; // namespace Gek
