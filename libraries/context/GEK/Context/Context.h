#pragma once

#include "GEK\Context\Observer.h"
#include <functional>

namespace Gek
{
    DECLARE_INTERFACE_IID(Context, "E1BBAFAB-1DD8-42E4-A031-46E22835EF1E") : virtual public IUnknown
    {
        static HRESULT create(Context **context);
    
        STDMETHOD_(void, addSearchPath)     (THIS_ LPCWSTR fileName) PURE;
        STDMETHOD_(void, initialize)        (THIS) PURE;

        STDMETHOD(createInstance)           (THIS_ REFCLSID className, REFIID interfaceType, LPVOID FAR *returnObject) PURE;
        STDMETHOD(createEachType)           (THIS_ REFCLSID typeName, std::function<HRESULT(REFCLSID, IUnknown *)> onCreateInstance) PURE;

        STDMETHOD_(void, logMessage)        (THIS_ LPCSTR file, UINT32 line, INT32 changeIndent, LPCWSTR format, ...) PURE;
    };

    DECLARE_INTERFACE_IID(ContextObserver, "4678440B-94FC-4671-9622-0D4030F8CE94") : virtual public Observer
    {
        STDMETHOD_(void, onLogMessage)      (THIS_ LPCSTR file, UINT32 line, LPCWSTR message) PURE;
    };
}; // namespace Gek
