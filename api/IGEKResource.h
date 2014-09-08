#pragma once

#include "GEKUtility.h"

DECLARE_INTERFACE_IID_(IGEKResource, IUnknown, "9CEFC12E-9D6E-4430-A6D6-12FEDF7FCF39")
{
    STDMETHOD(Load)             (THIS_ const UINT8 *pBuffer, LPCWSTR pName, LPCWSTR pParams) PURE;
};
