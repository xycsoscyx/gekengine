#pragma once

#include "GEK\Math\Vector4.h"
#include "GEK\Engine\Component.h"
#include <atlbase.h>
#include <atlstr.h>
#include <unordered_map>

namespace Gek
{
    struct CameraComponent
    {
        LPVOID operator new(size_t size)
        {
            return _mm_malloc(size * sizeof(CameraComponent), 16);
        }

            void operator delete(LPVOID data)
        {
            _mm_free(data);
        }

        float fieldOfView;
        float minimumDistance;
        float maximumDistance;
        Gek::Math::Float4 viewPort;

        CameraComponent(void);
        HRESULT save(std::unordered_map<CStringW, CStringW> &componentParameterList) const;
        HRESULT load(const std::unordered_map<CStringW, CStringW> &componentParameterList);
    };
}; // namespace Gek
