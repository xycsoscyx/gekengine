#pragma once

#include "GEK\Math\Vector4.h"
#include <atlbase.h>
#include <atlstr.h>
#include <unordered_map>

namespace Gek
{
    struct ModelComponent
    {
        CStringW value;

        ModelComponent(void);
        HRESULT save(std::unordered_map<CStringW, CStringW> &componentParameterList) const;
        HRESULT load(const std::unordered_map<CStringW, CStringW> &componentParameterList);

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
