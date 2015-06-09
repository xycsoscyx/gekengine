#pragma once

#include "GEK\Utility\Common.h"
#include "GEK\Utility\XML.h"
#include "GEK\Context\ObserverInterface.h"
#include "GEK\System\VideoInterface.h"
#include "GEK\Shape\Frustum.h"

namespace Gek
{
    namespace Engine
    {
        namespace Render
        {
            namespace Pass
            {
                DECLARE_INTERFACE_IID(Class, "022BA74E-CE90-4782-894F-BFFAB95171EE");

                DECLARE_INTERFACE_IID(Interface, "808F478F-C9A3-4C17-B743-C5CE95CBC9B3") : virtual public IUnknown
                {
                    STDMETHOD(initialize)                       (THIS_ IUnknown *initializerContext, Xml::Node &xmlPassNode) PURE;
                };
            }; // namespace Shader
        }; // namespace Render
    }; // namespace Engine
}; // namespace Gek
