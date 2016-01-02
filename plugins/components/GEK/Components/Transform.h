#pragma once

#include "GEK\Math\Vector3.h"
#include "GEK\Math\Quaternion.h"
#include "GEK\Math\Matrix4x4.h"
#include <atlbase.h>
#include <atlstr.h>
#include <unordered_map>

namespace Gek
{
    struct TransformComponent
    {
        LPVOID operator new(size_t size)
        {
            return _mm_malloc(size * sizeof(TransformComponent), 16);
        }

        void operator delete(LPVOID data)
        {
            _mm_free(data);
        }

        Gek::Math::Quaternion rotation;
        Gek::Math::Float3 position;

        TransformComponent(void);
        HRESULT save(std::unordered_map<CStringW, CStringW> &componentParameterList) const;
        HRESULT load(const std::unordered_map<CStringW, CStringW> &componentParameterList);

        inline Math::Float4x4 getMatrix(void) const
        {
            return rotation.getMatrix(position);
        }
    };
}; // namespace Gek
