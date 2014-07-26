#pragma once

#include "GEKUtility.h"
#include "GEKContext.h"
#include "GEKAPI.h"

template <typename CLASS>
class TGEKComponent
{
private:
    std::map<GEKENTITYID, CLASS> m_aEntityIndices;

public:
    TGEKComponent(void)
    {
    }

    virtual ~TGEKComponent(void)
    {
    }
};
