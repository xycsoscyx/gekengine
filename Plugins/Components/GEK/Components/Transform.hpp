/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision: 68c94ed58445f7f7b11fb87263c60bc483158d4d $
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date:   Fri Oct 21 13:54:12 2016 +0000 $
#pragma once

#include "GEK/Math/Vector3.hpp"
#include "GEK/Math/Matrix4x4.hpp"
#include "GEK/Math/Quaternion.hpp"
#include "GEK/Engine/Component.hpp"

namespace Gek
{
    namespace Components
    {
        GEK_COMPONENT(Transform)
        {
            Math::Float3 position = Math::Float3::Zero;
            Math::Quaternion rotation = Math::Quaternion::Identity;
            Math::Float3 scale = Math::Float3::One;

            inline Math::Float4x4 getMatrix(void) const
            {
                return Math::Float4x4(rotation, position);
            }

            inline Math::Float4x4 getScaledMatrix(void) const
            {
                return (Math::Float4x4::FromScale(scale) * getMatrix());
            }
        };
    }; // namespace Components
}; // namespace Gek
