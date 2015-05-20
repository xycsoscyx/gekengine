#pragma once

#include "GEK\Math\Vector4.h"
#include "GEK\Utility\Common.h"
#include "GEK\Engine\ComponentInterface.h"
#include <atlbase.h>
#include <atlstr.h>
#include <unordered_map>

namespace Gek
{
    namespace Engine
    {
        namespace Components
        {
            namespace Camera
            {
                static const Handle identifier = 0x00000001;
                struct Data
                {
                    float fieldOfView;
                    float minimumDistance;
                    float maximumDistance;
                    Gek::Math::Float4 viewPort;

                    Data(void);
                    HRESULT getData(std::unordered_map<CStringW, CStringW> &componentParameterList) const;
                    HRESULT setData(const std::unordered_map<CStringW, CStringW> &componentParameterList);
                };
            }; // namespace Camera
        }; // namespace Components
    }; // namespace Engine
}; // namespace Gek
