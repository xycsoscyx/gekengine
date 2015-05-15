#pragma once

#pragma once

#include "GEK\Math\Vector4.h"
#include "GEK\Math\Quaternion.h"
#include <atlbase.h>
#include <atlstr.h>
#include <unordered_map>

namespace Gek
{
    namespace Components
    {
        struct Follow
        {
            CStringW target;
            Gek::Math::Float3 offset;
            Gek::Math::Quaternion rotation;

            Follow(void);
            HRESULT getData(std::unordered_map<CStringW, CStringW> &componentParameterList) const;
            HRESULT setData(const std::unordered_map<CStringW, CStringW> &componentParameterList);
        };
    }; // namespace Components
}; // namespace Gek
