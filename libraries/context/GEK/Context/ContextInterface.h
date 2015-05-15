#pragma once

#include <Windows.h>
#include <functional>

namespace Gek
{
    DECLARE_INTERFACE_IID_(ContextInterface, IUnknown, "E1BBAFAB-1DD8-42E4-A031-46E22835EF1E")
    {
        STDMETHOD_(void, addSearchPath)                 (THIS_ LPCWSTR fileName) PURE;
        STDMETHOD_(void, initialize)                    (THIS) PURE;

        STDMETHOD(createInstance)                       (THIS_ REFCLSID classType, REFIID interfaceType, LPVOID FAR *returnObject) PURE;
        STDMETHOD(createEachType)                       (THIS_ REFCLSID type, std::function<HRESULT(REFCLSID, IUnknown *)> onCreateInstance) PURE;
    };

    HRESULT createContext(ContextInterface **context);
}; // namespace Gek
