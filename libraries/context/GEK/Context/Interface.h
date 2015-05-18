#pragma once

#include "GEK\Utility\Common.h"
#include "GEK\Context\ObserverInterface.h"
#include <functional>

namespace Gek
{
    namespace Context
    {
        DECLARE_INTERFACE_IID_(Interface, IUnknown, "E1BBAFAB-1DD8-42E4-A031-46E22835EF1E")
        {
            STDMETHOD_(void, addSearchPath)     (THIS_ LPCWSTR fileName) PURE;
            STDMETHOD_(void, initialize)        (THIS) PURE;

            STDMETHOD(createInstance)           (THIS_ REFCLSID className, REFIID interfaceType, LPVOID FAR *returnObject) PURE;
            STDMETHOD(createEachType)           (THIS_ REFCLSID type, std::function<HRESULT(REFCLSID, IUnknown *)> onCreateInstance) PURE;

            STDMETHOD_(void, logMessage)        (THIS_ LPCSTR file, UINT32 line, LPCWSTR format, ...) PURE;
            STDMETHOD_(void, logEnterScope)     (THIS) PURE;
            STDMETHOD_(void, logExitScope)      (THIS) PURE;
        };

        DECLARE_INTERFACE_IID_(Observer, ObserverInterface, "4678440B-94FC-4671-9622-0D4030F8CE94")
        {
            STDMETHOD_(void, onLogMessage)      (THIS_ LPCSTR file, UINT32 line, LPCWSTR message) PURE;
        };

        HRESULT create(Interface **context);
    }; // namespace Context
}; // namespace Gek
