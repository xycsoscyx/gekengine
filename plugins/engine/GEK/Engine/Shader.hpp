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
#include "GEK\System\VideoDevice.hpp"
#include "GEK\Engine\Material.hpp"
#include <memory>

namespace Gek
{
    namespace Engine
    {
        GEK_INTERFACE(Shader)
        {
            GEK_START_EXCEPTIONS();
            GEK_ADD_EXCEPTION(InvalidParameters);
            GEK_ADD_EXCEPTION(MissingParameters);
            GEK_ADD_EXCEPTION(ResourceAlreadyListed);
            GEK_ADD_EXCEPTION(UnlistedRenderTarget);
            GEK_ADD_EXCEPTION(UnknownMaterialType);
            GEK_ADD_EXCEPTION(InvalidElementType);

            GEK_PREDECLARE(Block);

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

                virtual Iterator next(void) = 0;

                virtual Mode prepare(void) = 0;
                virtual void clear(void) = 0;

                virtual uint32_t getIdentifier(void) const = 0;
                virtual uint32_t getFirstResourceStage(void) const = 0;
            };

            GEK_INTERFACE(Block)
            {
                using Iterator = std::unique_ptr<Block>;

                virtual Iterator next(void) = 0;

                virtual Pass::Iterator begin(void) = 0;

                virtual bool prepare(void) = 0;
            };

            struct Material
            {
                struct Resource
                {
                    String name;
                    String pattern;
                    String parameters;
                };

                uint32_t identifier;
                RenderStateHandle renderState;
                std::vector<Resource> resourceList;

                Material(uint32_t identifier)
                    : identifier(identifier)
                {
                }
            };

            virtual void reload(void) = 0;

            virtual uint32_t getPriority(void) const = 0;
            virtual const Material *getPassMaterial(const wchar_t *materialName) const = 0;

            virtual Block::Iterator begin(Video::Device::Context *videoContext, const Math::Float4x4 &viewMatrix, const Shapes::Frustum &viewFrustum) = 0;
        };
    }; // namespace Engine
}; // namespace Gek
