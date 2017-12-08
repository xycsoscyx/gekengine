/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision$
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date$
#pragma once

#include "GEK/Utility/Context.hpp"
#include "GEK/API/Handles.hpp"
#include <unordered_map>

namespace Gek
{
    namespace Engine
    {
        GEK_INTERFACE(Material)
        {
            struct Data
            {
                std::vector<ResourceHandle> resourceList;
            };

            virtual ~Material(void) = default;

            virtual Data const *getData(size_t materialHash) = 0;
            virtual RenderStateHandle getRenderState(void) = 0;
        };
    }; // namespace Engine
}; // namespace Gek
