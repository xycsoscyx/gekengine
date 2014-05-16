#pragma once

#include "GEKUtility.h"
#include "GEKContext.h"
#include "GEKAPI.h"

class CGEKComponent : public IGEKComponent
{
private:
    IGEKEntity *m_pEntity;

public:
    CGEKComponent(IGEKEntity *pEntity)
        : m_pEntity(pEntity)
    {
    }

    virtual ~CGEKComponent(void)
    {
    }

    STDMETHOD_(IGEKEntity *, GetEntity) (void) const
    {
        return m_pEntity;
    }
};
