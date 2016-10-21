/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision$
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date$
#pragma once

#include "GEK\Math\Vector4.hpp"
#include "GEK\Math\Matrix4x4.hpp"
#include "GEK\Math\Quaternion.hpp"

namespace Gek
{
    namespace Math
    {
        Float4x4 convert(const Quaternion &quaternion);
        Quaternion convert(const Float4x4 &matrix);
    }; // namespace Math
}; // namespace Gek
