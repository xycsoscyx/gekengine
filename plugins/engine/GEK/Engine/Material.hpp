/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision$
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date$
#pragma once

#include "GEK\Utility\Context.hpp"
#include "GEK\Engine\Handles.hpp"
#include <unordered_map>

namespace Gek
{
    namespace Engine
    {
        GEK_INTERFACE(Material)
        {
            struct PassData
            {
                RenderStateHandle renderState;
                std::vector<ResourceHandle> resourceList;
            };

            GEK_START_EXCEPTIONS();
            GEK_ADD_EXCEPTION(MissingParameters);

            virtual ~Material(void) = default;

            virtual const PassData *getPassData(uint32_t passIdentifier) = 0;
        };
    }; // namespace Engine
}; // namespace Gek
