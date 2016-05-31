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
        GEK_BASE_EXCEPTION();

        void get(const wstring &expression, INT32 &result, INT32 defaultValue = 0);
        void get(const wstring &expression, UINT32 &result, UINT32 defaultValue = 0);
        void get(const wstring &expression, INT64 &result, INT64 defaultValue = 0);
        void get(const wstring &expression, UINT64 &result, UINT64 defaultValue = 0);
        void get(const wstring &expression, float &result, float defaultValue = 0);
        void get(const wstring &expression, Math::Float2 &result, const Math::Float2 &defaultValue = Math::Float2(0.0f, 0.0f));
        void get(const wstring &expression, Math::Float3 &result, const Math::Float3 &defaultValue = Math::Float3(0.0f, 0.0f, 0.0f));
        void get(const wstring &expression, Math::Float4 &result, const Math::Float4 &defaultValue = Math::Float4(0.0f, 0.0f, 0.0f, 0.0f));
        void get(const wstring &expression, Math::Color &result, const Math::Color &defaultValue = Math::Color(0.0f, 0.0f, 0.0f, 1.0f));
        void get(const wstring &expression, Math::Quaternion &result, const Math::Quaternion &defaultValue = Math::Quaternion::Identity);
        void get(const wstring &expression, wstring &result);

        template <typename TYPE>
        TYPE get(const wstring &expression)
        {
            TYPE value;
            get(expression, value);
            return value;
        }
    }; // namespace Evaluator
}; // namespace Gek
