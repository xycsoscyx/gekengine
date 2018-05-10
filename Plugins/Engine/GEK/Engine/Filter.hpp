/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision$
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date$
#pragma once

#include "GEK/Utility/String.hpp"
#include "GEK/Shapes/Frustum.hpp"
#include "GEK/Utility/Context.hpp"
#include "GEK/API/Resources.hpp"
#include "GEK/API/Renderer.hpp"
#include <memory>

namespace Gek
{
    namespace Engine
    {
        GEK_INTERFACE(Filter)
        {
            GEK_INTERFACE(Pass)
            {
                enum class Mode : uint8_t
                {
                    None = 0,
                    Deferred,
                    Compute,
                };

                using Iterator = std::unique_ptr<Pass>;

                virtual ~Pass(void) = default;

                virtual Iterator next(void) = 0;

                virtual Mode prepare(void) = 0;
                virtual void clear(void) = 0;

                virtual bool isEnabled(void) const = 0;

				virtual Hash getIdentifier(void) const = 0;
				virtual std::string_view getName(void) const = 0;
			};

            virtual ~Filter(void) = default;

            virtual void reload(void) = 0;

			virtual Hash getIdentifier(void) const = 0;
			virtual std::string_view getName(void) const = 0;

            virtual Pass::Iterator begin(Video::Device::Context *videoContext, ResourceHandle input, ResourceHandle output) = 0;
        };
    }; // namespace Engine
}; // namespace Gek
