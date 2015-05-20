#pragma once

#include "GEK\Utility\Common.h"
#include "GEK\Context\ObserverInterface.h"
#include "GEK\System\VideoInterface.h"
#include "GEK\Shape\Frustum.h"

namespace Gek
{
    namespace Engine
    {
        namespace Render
        {
            namespace Attribute
            {
                enum
                {
                    Position = 1 << 0,
                    TexCoord = 1 << 1,
                    Normal = 1 << 2,
                };
            }; // namespace Attribute

            DECLARE_INTERFACE_IID(Interface, "C851EC91-8B07-4793-B9F5-B15C54E92014") : virtual public IUnknown
            {
                STDMETHOD_(Handle, loadProgram)             (THIS_ LPCWSTR fileName) PURE;
                STDMETHOD_(void, enableProgram)             (THIS_ Handle programHandle) PURE;

                STDMETHOD_(Handle, loadMaterial)            (THIS_ LPCWSTR fileName) PURE;
                STDMETHOD_(void, enableMaterial)            (THIS_ Handle materialHandle) PURE;
            };

            DECLARE_INTERFACE_IID(Observer, "16333226-FE0A-427D-A3EF-205486E1AD4D") : virtual public Gek::ObserverInterface
            {
                STDMETHOD_(void, onRenderBegin)             (THIS_ Handle viewerHandle) { };
                STDMETHOD_(void, onCullScene)               (THIS_ Handle viewerHandle, const Gek::Shape::Frustum &viewFrustum) { };
                STDMETHOD_(void, onDrawScene)               (THIS_ Handle viewerHandle, Gek::Video3D::ContextInterface *videoContext, UINT32 vertexAttributes) { };
                STDMETHOD_(void, onRenderEnd)               (THIS_ Handle viewerHandle) { };
                STDMETHOD_(void, onRenderOverlay)           (THIS) { };
            };
        }; // namespace Render
    }; // namespace Engine
}; // namespace Gek
