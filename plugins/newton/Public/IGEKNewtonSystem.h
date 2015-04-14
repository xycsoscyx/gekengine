#pragma once

#include "GEKUtility.h"

#include <dNewton.h>
#include <dNewtonPlayerManager.h>

DECLARE_INTERFACE_IID_(IGEKNewtonSystem, IUnknown, "E8B4EB56-E5E1-41F6-B761-32C74426314F")
{
    STDMETHOD_(dNewton *, GetCore)                          (THIS) PURE;
    STDMETHOD_(dNewtonPlayerManager *, GetPlayerManager)    (THIS) PURE;
    STDMETHOD_(float3, GetGravity)                          (THIS) PURE;
};

DECLARE_INTERFACE_IID_(IGEKNewtonObserver, IGEKObserver, "A787D140-63B4-43EA-B3A9-EDEF6EAA4C95")
{
    STDMETHOD_(void, OnCollision)                           (THIS_ const GEKENTITYID &nEntity0, const GEKENTITYID &nEntity1, const float3 &nPosition, const float3 &nNormal) PURE;
};
