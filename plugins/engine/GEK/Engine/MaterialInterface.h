#pragma once

#include "GEK\Context\ObserverInterface.h"
#include "GEK\System\VideoInterface.h"
#include "GEK\Shape\Frustum.h"

namespace Gek
{
    namespace Engine
    {
        namespace Render
        {
            namespace Material
            {
                DECLARE_INTERFACE_IID(Class, "0E7141AC-0CC5-4F5F-B96E-F10ED4155471");

                DECLARE_INTERFACE_IID(Interface, "57D5B374-A559-44D0-B017-82034A136C16") : virtual public IUnknown
                {
                    STDMETHOD(initialize)                       (THIS_ IUnknown *initializerContext, LPCWSTR fileName) PURE;
                    STDMETHOD(getShader)                        (THIS_ IUnknown **returnObject) PURE;

                    STDMETHOD_(void, enable)                    (THIS_ Video3D::ContextInterface *context) PURE;
                };
            }; // namespace Material
        }; // namespace Render
    }; // namespace Engine
}; // namespace Gek
