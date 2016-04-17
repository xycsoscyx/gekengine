#pragma once

#include "GEK\Math\Vector4.h"
#include "GEK\Engine\Population.h"

namespace Gek
{
    struct ModelComponent
    {
        CStringW value;
        CStringW skin;

        ModelComponent(void);
        HRESULT save(Population::ComponentDefinition &componentData) const;
        HRESULT load(const Population::ComponentDefinition &componentData);

        inline operator LPCWSTR () const
        {
            return value.GetString();
        }

        inline operator CStringW () const
        {
            return value;
        }

        inline CStringW &operator = (LPCWSTR value)
        {
            this->value = value;
            return this->value;
        }
    };
}; // namespace Gek
