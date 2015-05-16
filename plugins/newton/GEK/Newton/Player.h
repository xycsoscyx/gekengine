#pragma once

#include "GEK\Math\Vector4.h"
#include "GEK\Engine\ComponentInterface.h"
#include <atlbase.h>
#include <atlstr.h>
#include <unordered_map>

namespace Gek
{
    namespace Components
    {
        namespace Player
        {
            static const void *Identifier = nullptr;
            struct Data
            {
                float outerRadius;
                float innerRadius;
                float height;
                float stairStep;

                Data(void);
                HRESULT getData(std::unordered_map<CStringW, CStringW> &componentParameterList) const;
                HRESULT setData(const std::unordered_map<CStringW, CStringW> &componentParameterList);
            };
        }; // namespace Player
    }; // namespace Components
}; // namespace Gek
