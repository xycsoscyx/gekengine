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
            namespace Transform
            {
                struct Data
                {
                    Gek::Math::Float3 position;
                    Gek::Math::Quaternion rotation;

                    Data(void);

                    LPVOID operator new(size_t size)
                    {
                        return _mm_malloc(size * sizeof(Data), 16);
                    }

                    void operator delete(LPVOID data)
                    {
                        _mm_free(data);
                    }

                    HRESULT save(std::unordered_map<CStringW, CStringW> &componentParameterList) const;
                    HRESULT load(const std::unordered_map<CStringW, CStringW> &componentParameterList);
                };
            }; // namespace Transform
        }; // namespace Components
    }; // namespace Engine
}; // namespace Gek
