#pragma once

#include <atlbase.h>
#include <atlstr.h>
#include <unordered_map>

namespace Gek
{
    namespace Engine
    {
        namespace Components
        {
            namespace PointLight
            {
                struct Data
                {
                    float radius;

                    Data(void);
                    HRESULT save(std::unordered_map<CStringW, CStringW> &componentParameterList) const;
                    HRESULT load(const std::unordered_map<CStringW, CStringW> &componentParameterList);
                };
            }; // namespace PointLight
        }; // namespace Components
    }; // namespace Engine
}; // namespace Gek
