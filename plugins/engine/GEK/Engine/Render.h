#pragma once

#include "GEK\Context\Observer.h"
#include "GEK\System\VideoSystem.h"
#include "GEK\Engine\Resources.h"
#include "GEK\Shapes\Frustum.h"

namespace Gek
{
    GEK_PREDECLARE(Entity);

    GEK_INTERFACE(RenderPipeline)
    {
        virtual VideoPipeline * const getPipeline(void) = 0;
    };

    GEK_INTERFACE(RenderContext)
    {
        virtual VideoContext * const getContext(void) = 0;

        virtual RenderPipeline * const computePipeline(void) = 0;
        virtual RenderPipeline * const vertexPipeline(void) = 0;
        virtual RenderPipeline * const geometryPipeline(void) = 0;
        virtual RenderPipeline * const pixelPipeline(void) = 0;
    };

    GEK_INTERFACE(Render)
    {
        virtual void initialize(IUnknown *initializerContext) = 0;

        virtual void render(Entity *cameraEntity, const Math::Float4x4 &projectionMatrix, float minimumDistance, float maximumDistance) = 0;
        virtual void queueDrawCall(PluginHandle plugin, MaterialHandle material, std::function<void(RenderContext *renderContext)> draw) = 0;
    };

    GEK_INTERFACE(RenderObserver)
    {
        virtual void onRenderBackground(void) = 0;
        virtual void onRenderScene(Entity *cameraEntity, const Math::Float4x4 *viewMatrix, const Shapes::Frustum *viewFrustum) = 0;
        virtual void onRenderForeground(void) = 0;
    };
}; // namespace Gek
