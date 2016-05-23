#pragma once

#include "GEK\Context\Observer.h"
#include "GEK\System\VideoSystem.h"
#include "GEK\Engine\Resources.h"
#include "GEK\Shapes\Frustum.h"

namespace Gek
{
    interface Entity;

    interface RenderPipeline
    {
        VideoPipeline *getPipeline(void);
    };

    interface RenderContext
    {
        VideoContext *getContext(void);

        RenderPipeline *computePipeline(void);
        RenderPipeline *vertexPipeline(void);
        RenderPipeline *geometryPipeline(void);
        RenderPipeline *pixelPipeline(void);
    };

    interface Render
    {
        void initialize(IUnknown *initializerContext);

        void render(Entity *cameraEntity, const Math::Float4x4 &projectionMatrix, float minimumDistance, float maximumDistance);
        void queueDrawCall(PluginHandle plugin, MaterialHandle material, std::function<void(RenderContext *renderContext)> draw);
    };

    interface RenderObserver
    {
        void onRenderBackground(void) = default;
        void onRenderScene(Entity *cameraEntity, const Math::Float4x4 *viewMatrix, const Shapes::Frustum *viewFrustum) = default;
        void onRenderForeground(void) = default;
    };

    DECLARE_INTERFACE_IID(RenderRegistration, "97A6A7BC-B739-49D3-808F-3911AE3B8A77");
}; // namespace Gek
