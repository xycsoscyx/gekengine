#pragma once

#include <Windows.h>
#include <functional>

namespace Gek
{
    DECLARE_INTERFACE_IID_(ContextInterface, IUnknown, "E1BBAFAB-1DD8-42E4-A031-46E22835EF1E")
    {
        STDMETHOD_(void, addSearchPath)                 (THIS_ LPCWSTR basePath) PURE;
        STDMETHOD_(void, initialize)                    (THIS) PURE;

        STDMETHOD(createInstance)                       (THIS_ REFCLSID classID, REFIID interfaceID, LPVOID FAR *newInstance) PURE;
        STDMETHOD(createEachType)                       (THIS_ REFCLSID typeID, std::function<HRESULT(REFCLSID, IUnknown *)> onCreateInstance) PURE;
    };

    HRESULT createContext(ContextInterface **context);
}; // namespace Gek
