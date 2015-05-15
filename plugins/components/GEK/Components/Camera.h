#pragma once

#include "GEK\Math\Vector4.h"
#include <atlbase.h>
#include <atlstr.h>
#include <unordered_map>

namespace Gek
{
    namespace Components
    {
        struct Camera
        {
            float fieldOfView;
            float minimumDistance;
            float maximumDistance;
            Gek::Math::Float4 viewPort;

            Camera(void);
            HRESULT getData(std::unordered_map<CStringW, CStringW> &componentParameterList) const;
            HRESULT setData(const std::unordered_map<CStringW, CStringW> &componentParameterList);
        };
    }; // namespace Components
}; // namespace Gek
