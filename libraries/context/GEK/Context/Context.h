#pragma once

#include "GEK\Context\Observer.h"
#include <functional>
#include <typeindex>

namespace Gek
{
    DECLARE_INTERFACE_IID(Context, "E1BBAFAB-1DD8-42E4-A031-46E22835EF1E") : virtual public IUnknown
    {
        static HRESULT create(Context **context);
    
        STDMETHOD_(void, addSearchPath)     (THIS_ LPCWSTR fileName) PURE;
        STDMETHOD_(void, initialize)        (THIS) PURE;

        STDMETHOD(createInstance)           (THIS_ REFCLSID className, REFIID interfaceType, LPVOID FAR *returnObject) PURE;
        STDMETHOD(createEachType)           (THIS_ REFCLSID typeName, std::function<HRESULT(REFCLSID, IUnknown *)> onCreateInstance) PURE;
    };
}; // namespace Gek
