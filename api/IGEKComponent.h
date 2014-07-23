#pragma once

#include "GEKUtility.h"
#include "GEKVALUE.h"
#include "IGEKSceneManager.h"

DECLARE_INTERFACE_IID_(IGEKComponent, IUnknown, "F1CA9EEC-0F09-45DA-BF24-0C70F5F96E3E")
{
    STDMETHOD_(void, ListProperties)            (THIS_ const GEKENTITYID &nEntityID, std::function<void(LPCWSTR, const GEKVALUE &)> OnProperty) PURE;
    STDMETHOD_(bool, GetProperty)               (THIS_ const GEKENTITYID &nEntityID, LPCWSTR pName, GEKVALUE &kValue) const PURE;
    STDMETHOD_(bool, SetProperty)               (THIS_ const GEKENTITYID &nEntityID, LPCWSTR pName, const GEKVALUE &kValue) PURE;
};

DECLARE_INTERFACE_IID_(IGEKSystem, IUnknown, "81A24012-F085-42D0-B931-902485673E90")
{
};

