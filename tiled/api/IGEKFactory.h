#pragma once

#include "GEKUtility.h"

DECLARE_INTERFACE_IID_(IGEKFactory, IUnknown, "09452B53-4A6E-4655-BAC7-F37F42209A47")
{
    STDMETHOD(Create)           (THIS_ const UINT8 *pBuffer, REFIID rIID, LPVOID FAR *ppObject) PURE;
};
