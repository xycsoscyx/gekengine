/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision: 68c94ed58445f7f7b11fb87263c60bc483158d4d $
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date:   Fri Oct 21 13:54:12 2016 +0000 $
#pragma once

#include "GEK/Math/Vector2.hpp"
#include "GEK/Math/Vector3.hpp"
#include "GEK/Math/Vector4.hpp"
#include "GEK/Math/Vector4.hpp"
#include "GEK/Math/Quaternion.hpp"
#include "GEK/Utility/String.hpp"

namespace Gek
{
    namespace Evaluator
    {
        void SetRandomSeed(uint32_t seed);

        void Get(const wchar_t *expression, int32_t &result, int32_t defaultValue = 0);
        void Get(const wchar_t *expression, uint32_t &result, uint32_t defaultValue = 0);
        void Get(const wchar_t *expression, float &result, float defaultValue = 0.0f);
        void Get(const wchar_t *expression, Math::Float2 &result, const Math::Float2 &defaultValue = Math::Float2::Zero);
        void Get(const wchar_t *expression, Math::Float3 &result, const Math::Float3 &defaultValue = Math::Float3::Zero);
        void Get(const wchar_t *expression, Math::Float4 &result, const Math::Float4 &defaultValue = Math::Float4::Zero);
        void Get(const wchar_t *expression, Math::Quaternion &result, const Math::Quaternion &defaultValue = Math::Quaternion::Identity);
        void Get(const wchar_t *expression, String &result);

        template <typename TYPE>
        TYPE Get(const wchar_t *expression)
        {
            TYPE value;
            Get(expression, value);
            return value;
        }
    }; // namespace Evaluator
}; // namespace Gek
