#pragma once

#include "GEK\Context\Observer.h"
#include "GEK\System\VideoSystem.h"
#include "GEK\Engine\Resources.h"
#include "GEK\Shapes\Frustum.h"

namespace Gek
{
    DECLARE_INTERFACE(Entity);

    DECLARE_INTERFACE_IID(RenderPipeline, "F2BA020B-DBCA-42F5-ABFA-743EE87B298D") : virtual public IUnknown
    {
        STDMETHOD_(VideoPipeline *, getPipeline)             (THIS) PURE;
    };

    DECLARE_INTERFACE_IID(RenderContext, "ABCC7497-93C1-4C40-B92A-2B1ED6D813B9") : virtual public IUnknown
    {
        STDMETHOD_(VideoContext *, getContext)               (THIS) PURE;

        STDMETHOD_(RenderPipeline *, computePipeline)        (THIS) PURE;
        STDMETHOD_(RenderPipeline *, vertexPipeline)         (THIS) PURE;
        STDMETHOD_(RenderPipeline *, geometryPipeline)       (THIS) PURE;
        STDMETHOD_(RenderPipeline *, pixelPipeline)          (THIS) PURE;
    };

    DECLARE_INTERFACE_IID(Render, "C851EC91-8B07-4793-B9F5-B15C54E92014") : virtual public IUnknown
    {
        STDMETHOD(initialize)                           (THIS_ IUnknown *initializerContext) PURE;

        STDMETHOD_(void, render)                        (THIS_ Entity *cameraEntity, const Math::Float4x4 &projectionMatrix, float minimumDistance, float maximumDistance) PURE;
        STDMETHOD_(void, queueDrawCall)                 (THIS_ PluginHandle plugin, MaterialHandle material, std::function<void(RenderContext *renderContext)> draw) PURE;
    };

    DECLARE_INTERFACE_IID(RenderObserver, "16333226-FE0A-427D-A3EF-205486E1AD4D") : virtual public Observer
    {
        STDMETHOD_(void, onRenderScene)                 (THIS_ Entity *cameraEntity, const Math::Float4x4 *viewMatrix, const Shapes::Frustum *viewFrustum) { };
        STDMETHOD_(void, onRenderOverlay)               (THIS) { };
    };

    DECLARE_INTERFACE_IID(RenderRegistration, "97A6A7BC-B739-49D3-808F-3911AE3B8A77");
}; // namespace Gek
