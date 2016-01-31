#pragma once

#include "GEK\Math\Vector4.h"
#include "GEK\Engine\Component.h"
#include <atlbase.h>
#include <atlstr.h>
#include <unordered_map>

namespace Gek
{
    struct CameraComponent
    {
        float fieldOfView;
        float minimumDistance;
        float maximumDistance;

        CameraComponent(void);
        HRESULT save(std::unordered_map<CStringW, CStringW> &componentParameterList) const;
        HRESULT load(const std::unordered_map<CStringW, CStringW> &componentParameterList);
    };
}; // namespace Gek
