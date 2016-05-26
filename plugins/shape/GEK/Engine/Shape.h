#pragma once

#include "GEK\Math\Float4.h"
#include "GEK\Engine\Population.h"

namespace Gek
{
    struct ShapeComponent
    {
        wstring value;
        wstring parameters;
        wstring skin;

        ShapeComponent(void);
        HRESULT save(Population::ComponentDefinition &componentData) const;
        HRESULT load(const Population::ComponentDefinition &componentData);

        inline operator const wchar_t *() const
        {
            return value.GetString();
        }

        inline operator wstring () const
        {
            return value;
        }

        inline wstring &operator = (const wchar_t *value)
        {
            this->value = value;
            return this->value;
        }
    };
}; // namespace Gek
