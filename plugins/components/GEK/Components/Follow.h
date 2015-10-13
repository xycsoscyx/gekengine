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
                struct Data
                {
                    CStringW target;
                    Gek::Math::Float4 offset;
                    Gek::Math::Quaternion rotation;

                    LPVOID operator new(size_t size)
                    {
                        return _mm_malloc(size * sizeof(Data), 16);
                    }

                        void operator delete(LPVOID data)
                    {
                        _mm_free(data);
                    }

                    Data(void);
                    HRESULT save(std::unordered_map<CStringW, CStringW> &componentParameterList) const;
                    HRESULT load(const std::unordered_map<CStringW, CStringW> &componentParameterList);
                };
            }; // namespace Follow
        }; // namespace Components
    }; // namespace Engine
}; // namespace Gek
