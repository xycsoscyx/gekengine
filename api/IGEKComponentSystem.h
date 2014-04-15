#pragma once

#include "GEKUtility.h"

DECLARE_INTERFACE(IGEKEntity);
DECLARE_INTERFACE(IGEKComponent);

DECLARE_INTERFACE_IID_(IGEKComponentSystem, IUnknown, "81A24012-F085-42D0-B931-902485673E90")
{
    STDMETHOD(Create)           (THIS_ const CLibXMLNode &kNode, IGEKEntity *pEntity, IGEKComponent **ppComponent) PURE;
    STDMETHOD(Destroy)          (THIS_ IGEKEntity *pEntity) PURE;
    STDMETHOD_(void, Clear)     (THIS) PURE;
};
