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
            namespace Shader
            {
                DECLARE_INTERFACE_IID(Class, "02B8870C-2AEC-48FD-8F47-34166C9F16C6");

                DECLARE_INTERFACE_IID(Interface, "E4410687-FA71-4177-922D-B8A4C30EDB1D") : virtual public IUnknown
                {
                    STDMETHOD(initialize)                       (THIS_ IUnknown *initializerContext, LPCWSTR fileName) PURE;
                };
            }; // namespace Shader
        }; // namespace Render
    }; // namespace Engine
}; // namespace Gek
