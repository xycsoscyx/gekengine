#pragma once

#include "GEK\Context\Context.h"
#include "GEK\Context\Observable.h"
#include "GEK\Engine\Resources.h"
#include "GEK\Shapes\Frustum.h"

namespace Gek
{
    GEK_PREDECLARE(VideoSystem);
    GEK_PREDECLARE(VideoPipeline);
    GEK_PREDECLARE(VideoContext);
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
        : virtual public Observable
    {
        virtual VideoSystem * getVideoSystem(void) const = 0;

        virtual void render(Entity *cameraEntity, const Math::Float4x4 &projectionMatrix, float minimumDistance, float maximumDistance) = 0;
        virtual void queueDrawCall(PluginHandle plugin, MaterialHandle material, std::function<void(RenderContext *renderContext)> draw) = 0;
    };

    GEK_INTERFACE(RenderObserver)
        : virtual public Observer
    {
        virtual void onRenderBackground(void) { };
        virtual void onRenderScene(Entity *cameraEntity, const Math::Float4x4 *viewMatrix, const Shapes::Frustum *viewFrustum) { };
        virtual void onRenderForeground(void) { };
    };
}; // namespace Gek
