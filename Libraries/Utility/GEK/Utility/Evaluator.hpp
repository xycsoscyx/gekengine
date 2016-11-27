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
#include "GEK/Utility/ShuntingYard.hpp"

namespace Gek
{
    namespace Evaluator
    {
        GEK_ADD_EXCEPTION();

        void Get(ShuntingYard &shuntingYard, const wchar_t *expression, int32_t &result);
        void Get(ShuntingYard &shuntingYard, const wchar_t *expression, uint32_t &result);
        void Get(ShuntingYard &shuntingYard, const wchar_t *expression, float &result);
        void Get(ShuntingYard &shuntingYard, const wchar_t *expression, Math::Float2 &result);
        void Get(ShuntingYard &shuntingYard, const wchar_t *expression, Math::Float3 &result);
        void Get(ShuntingYard &shuntingYard, const wchar_t *expression, Math::Float4 &result);
        void Get(ShuntingYard &shuntingYard, const wchar_t *expression, Math::Quaternion &result);
        void Get(ShuntingYard &shuntingYard, const wchar_t *expression, String &result);

        template <typename TYPE>
        TYPE Get(ShuntingYard &shuntingYard, const wchar_t *expression)
        {
            TYPE value;
            Get(shuntingYard, expression, value);
            return value;
        }
    }; // namespace Evaluator
}; // namespace Gek
