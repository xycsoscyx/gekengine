#pragma once

#include "GEK\Context\ObserverInterface.h"
#include "GEK\System\VideoSystem.h"
#include "GEK\Shape\Frustum.h"

namespace Gek
{
    namespace Engine
    {
        namespace Render
        {
            namespace Shader
            {
                DECLARE_INTERFACE(Interface);
            };

            namespace Material
            {
                DECLARE_INTERFACE_IID(Class, "0E7141AC-0CC5-4F5F-B96E-F10ED4155471");

                DECLARE_INTERFACE_IID(Interface, "57D5B374-A559-44D0-B017-82034A136C16") : virtual public IUnknown
                {
                    STDMETHOD(initialize)                       (THIS_ IUnknown *initializerContext, LPCWSTR fileName) PURE;
                    STDMETHOD_(Shader::Interface *, getShader)  (THIS) PURE;

                    STDMETHOD_(void, enable)                    (THIS_ Video::Context::Interface *context, LPCVOID passData) PURE;
                };
            }; // namespace Material
        }; // namespace Render
    }; // namespace Engine
}; // namespace Gek
