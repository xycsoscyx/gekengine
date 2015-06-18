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
            namespace Plugin
            {
                DECLARE_INTERFACE_IID(Class, "379FF0E3-85F2-4138-A6E4-BCA93241D4F0");

                DECLARE_INTERFACE_IID(Interface, "F025D5AE-BE84-4DF7-90C0-0E241EABB56B") : virtual public IUnknown
                {
                    STDMETHOD(initialize)                       (THIS_ IUnknown *initializerContext, LPCWSTR fileName) PURE;
                };
            }; // namespace Plugin
        }; // namespace Render
    }; // namespace Engine
}; // namespace Gek
