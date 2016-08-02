#pragma once

#include "GEK\Utility\String.h"
#include "GEK\Shapes\Frustum.h"
#include "GEK\Context\Context.h"
#include "GEK\Engine\Resources.h"
#include "GEK\Engine\Material.h"
#include <memory>

namespace Gek
{
    namespace Engine
    {
        GEK_INTERFACE(Shader)
        {
            GEK_START_EXCEPTIONS();
            GEK_ADD_EXCEPTION(InvalidDepthParameters);
            GEK_ADD_EXCEPTION(InvalidParameters);
            GEK_ADD_EXCEPTION(MissingRequiredParameters);
            GEK_ADD_EXCEPTION(ResourceAlreadyListed);
            GEK_ADD_EXCEPTION(UnlistedRenderTarget);
            GEK_ADD_EXCEPTION(UnknownMaterialType);

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

                virtual bool enableMaterial(Material *material) = 0;
            };

            GEK_INTERFACE(Block)
            {
                using Iterator = std::unique_ptr<Block>;

                virtual Iterator next(void) = 0;

                virtual Pass::Iterator begin(void) = 0;

                virtual bool prepare(void) = 0;
            };

            virtual uint32_t getPriority(void) = 0;

            virtual Material::DataPtr loadMaterialData(const Material::PassMap &passMap) = 0;

            virtual Block::Iterator begin(Video::Device::Context *deviceContext, const Math::Float4x4 &viewMatrix, const Shapes::Frustum &viewFrustum, ResourceHandle cameraTarget) = 0;
        };
    }; // namespace Engine
}; // namespace Gek
