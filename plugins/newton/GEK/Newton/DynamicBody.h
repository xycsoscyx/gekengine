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
        namespace DynamicBody
        {
            static const Handle identifier = 11;
            struct Data
            {
                CStringW shape;
                CStringW material;

                Data(void);
                HRESULT getData(std::unordered_map<CStringW, CStringW> &componentParameterList) const;
                HRESULT setData(const std::unordered_map<CStringW, CStringW> &componentParameterList);
            };
        }; // namespace DynamicBody
    }; // namespace Components
}; // namespace Gek
