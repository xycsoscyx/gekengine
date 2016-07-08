#pragma once

#include "GEK\Utility\String.h"
#include "GEK\Shapes\Frustum.h"
#include "GEK\Context\Context.h"
#include "GEK\Engine\Resources.h"
#include <memory>

namespace Gek
{
    namespace Engine
    {
        GEK_INTERFACE(Shader)
        {
            GEK_PREDECLARE(Block);

            GEK_INTERFACE(Pass)
            {
                enum class Mode : uint8_t
                {
                    Forward = 0,
                    Deferred,
                    Compute,
                };

                using Iterator = std::unique_ptr<Pass>;

                virtual Iterator next(void) = 0;
                virtual Mode prepare(void) = 0;
            };

            GEK_INTERFACE(Block)
            {
                using Iterator = std::unique_ptr<Block>;

                virtual Iterator next(void) = 0;
                virtual Pass::Iterator begin(void) = 0;
                virtual bool prepare(void) = 0;
            };

            virtual uint32_t getPriority(void) = 0;

            //virtual ResourceListPtr loadResourceList(const wchar_t *materialName, const ResourcePassMap &resourcePassMap) = 0;
            //virtual bool setResourceList(Video::Device::Context *deviceContext, Block *block, Pass *pass, ResourceList * const resourceList) = 0;

            virtual Block::Iterator begin(Video::Device::Context *deviceContext, const Math::Float4x4 &viewMatrix, const Shapes::Frustum &viewFrustum, ResourceHandle cameraTarget) = 0;
        };
    }; // namespace Engine
}; // namespace Gek
