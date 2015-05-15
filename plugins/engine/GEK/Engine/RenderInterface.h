#pragma once

#include "GEK\Utility\Common.h"
#include "GEK\Context\ObserverInterface.h"
#include "GEK\System\VideoInterface.h"
#include "GEK\Shape\Frustum.h"

namespace Gek
{
    namespace Render
    {
        enum
        {
            Position    = 1 << 0,
            TexCoord    = 1 << 1,
            Normal      = 1 << 2,
        };

        DECLARE_INTERFACE_IID_(ObserverInterface, Gek::ObserverInterface, "16333226-FE0A-427D-A3EF-205486E1AD4D")
        {
            STDMETHOD_(void, onRenderBegin)             (THIS_ Handle viewerHandle) { };
            STDMETHOD_(void, onCullScene)               (THIS_ Handle viewerHandle, const Gek::Shape::Frustum &viewFrustum) { };
            STDMETHOD_(void, onDrawScene)               (THIS_ Handle viewerHandle, Gek::Video3D::ContextInterface *videoContext, UINT32 vertexAttributes) { };
            STDMETHOD_(void, onRenderEnd)               (THIS_ Handle viewerHandle) { };
            STDMETHOD_(void, onRenderOverlay)           (THIS) { };
        };
    }; // namespace Render
}; // namespace Gek
