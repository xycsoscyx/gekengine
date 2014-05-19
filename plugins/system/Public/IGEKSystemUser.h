#pragma once

#include "GEKUtility.h"

class IGEKSystem;

class __declspec(uuid("D9FED400-0CFA-4877-A935-649C839156E1")) IGEKSystemUser : public IUnknown
{
public:
    virtual ~IGEKSystemUser(void)
    {
    }

    virtual HRESULT RegisterSystem(IGEKSystem *pSystem) = 0;
};
