/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision: fc6dba5a2aba4d25dcd872ccbb719e3619e40901 $
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date:   Wed Oct 19 17:38:40 2016 +0000 $
#pragma once

#include "GEK/Utility/String.hpp"
#include "GEK/Shapes/Frustum.hpp"
#include "GEK/Utility/Context.hpp"
#include "GEK/System/VideoDevice.hpp"
#include "GEK/Engine/Material.hpp"
#include <memory>

namespace Gek
{
    namespace Engine
    {
        GEK_INTERFACE(Shader)
        {
            GEK_INTERFACE(Pass)
            {
                enum class Mode : uint8_t
                {
                    None = 0,
                    Forward,
                    Deferred,
                    Compute,
                };

                using Iterator = std::unique_ptr<Pass>;

                virtual ~Pass(void) = default;

                virtual Iterator next(void) = 0;

                virtual Mode prepare(void) = 0;
                virtual void clear(void) = 0;

                virtual std::string_view getName(void) const = 0;

                virtual size_t getMaterialHash(void) const = 0;
                virtual uint32_t getFirstResourceStage(void) const = 0;
                virtual bool isLightingRequired(void) const = 0;
            };

            GEK_INTERFACE(Material)
            {
                struct Initializer
                {
                    std::string name;
                    ResourceHandle fallback;
                };

                using Iterator = std::unique_ptr<Material>;

                virtual ~Material(void) = default;

                virtual Iterator next(void) = 0;

                virtual std::string_view getName(void) const = 0;
                virtual std::vector<Initializer> const &getInitializerList(void) const = 0;
                virtual RenderStateHandle getRenderState(void) const = 0;
            };

            virtual ~Shader(void) = default;

            virtual void reload(void) = 0;

			virtual std::string_view getName(void) const = 0;

            virtual uint32_t getDrawOrder(void) const = 0;
            virtual bool isLightingRequired(void) const = 0;
            virtual std::string const &getOutput(void) const = 0;

            virtual Material::Iterator begin(void) = 0;
            virtual Pass::Iterator begin(Video::Device::Context *videoContext, Math::Float4x4 const &viewMatrix, Shapes::Frustum const &viewFrustum) = 0;
        };
    }; // namespace Engine
}; // namespace Gek
