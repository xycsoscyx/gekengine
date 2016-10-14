/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision$
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date$
#pragma once

#include "GEK\Utility\String.hpp"
#include "GEK\Shapes\Frustum.hpp"
#include "GEK\Utility\Context.hpp"
#include "GEK\Engine\Resources.hpp"
#include "GEK\Engine\Renderer.hpp"
#include <memory>

namespace Gek
{
    namespace Engine
    {
        GEK_INTERFACE(Filter)
        {
            GEK_START_EXCEPTIONS();
            GEK_ADD_EXCEPTION(ResourceAlreadyListed);
            GEK_ADD_EXCEPTION(InvalidParameters);
            GEK_ADD_EXCEPTION(MissingParameters);
            GEK_ADD_EXCEPTION(UnlistedRenderTarget);

            GEK_INTERFACE(Pass)
            {
                enum class Mode : uint8_t
                {
                    None = 0,
                    Deferred,
                    Compute,
                };

                using Iterator = std::unique_ptr<Pass>;

                virtual Iterator next(void) = 0;

                virtual Mode prepare(void) = 0;
                virtual void clear(void) = 0;
            };

            virtual void reload(void) = 0;

            virtual Pass::Iterator begin(Video::Device::Context *videoContext) = 0;
        };
    }; // namespace Engine
}; // namespace Gek
