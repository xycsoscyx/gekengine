#pragma once

#include "GEK\Math\Vector4.h"
#include "GEK\Utility\Common.h"
#include <atlbase.h>
#include <atlstr.h>
#include <unordered_map>

namespace Gek
{
    namespace Newton
    {
        namespace Player
        {
            static const Handle identifier = 13;
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
    }; // namespace Newton
}; // namespace Gek
