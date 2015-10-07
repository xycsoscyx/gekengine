#pragma once

#pragma once

#include "GEK\Math\Vector3.h"
#include "GEK\Math\Quaternion.h"
#include <atlbase.h>
#include <atlstr.h>
#include <unordered_map>

namespace Gek
{
    namespace Engine
    {
        namespace Components
        {
            namespace Follow
            {
                static const UINT32 identifier = 0x00000003;
                struct Data
                {
                    CStringW target;
                    Gek::Math::Float4 offset;
                    Gek::Math::Quaternion rotation;

                    Data(void);
                    HRESULT getData(std::unordered_map<CStringW, CStringW> &componentParameterList) const;
                    HRESULT setData(const std::unordered_map<CStringW, CStringW> &componentParameterList);
                };
            }; // namespace Follow
        }; // namespace Components
    }; // namespace Engine
}; // namespace Gek
