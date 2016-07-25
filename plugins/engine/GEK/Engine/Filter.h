#pragma once

#include "GEK\Utility\String.h"
#include "GEK\Shapes\Frustum.h"
#include "GEK\Context\Context.h"
#include "GEK\Engine\Resources.h"
#include "GEK\Engine\Renderer.h"
#include <memory>

namespace Gek
{
    namespace Engine
    {
        GEK_INTERFACE(Filter)
        {
            GEK_INTERFACE(Pass)
            {
                GEK_START_EXCEPTIONS();

                enum class Mode : uint8_t
                {
                    Exit = 0,
                    Deferred,
                    Compute,
                };

                using Iterator = std::unique_ptr<Pass>;

                virtual Iterator next(void) = 0;

                virtual Mode prepare(void) = 0;
                virtual void clear(void) = 0;
            };

            virtual Pass::Iterator begin(Video::Device::Context *deviceContext, ResourceHandle cameraTarget) = 0;
        };
    }; // namespace Engine
}; // namespace Gek
