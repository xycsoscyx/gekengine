#pragma once

#include <atlbase.h>
#include <atlstr.h>
#include <unordered_map>

namespace Gek
{
    struct PointLightComponent
    {
        float radius;

        PointLightComponent(void);
        HRESULT save(std::unordered_map<CStringW, CStringW> &componentParameterList) const;
        HRESULT load(const std::unordered_map<CStringW, CStringW> &componentParameterList);
    };
}; // namespace Gek
