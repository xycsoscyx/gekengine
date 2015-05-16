#pragma once

#include "GEK\Utility\Common.h"
#include <atlbase.h>
#include <atlstr.h>
#include <unordered_map>

namespace Gek
{
    namespace Components
    {
        namespace PointLight
        {
            static const Handle  *identifier = 4;
            struct Data
            {
                float radius;

                Data(void);
                HRESULT getData(std::unordered_map<CStringW, CStringW> &componentParameterList) const;
                HRESULT setData(const std::unordered_map<CStringW, CStringW> &componentParameterList);
            };
        }; // namespace PointLight
    }; // namespace Components
}; // namespace Gek
