#pragma once

#include "GEK\Math\Float3.h"
#include "GEK\Math\Quaternion.h"
#include "GEK\Math\Float4x4.h"
#include "GEK\Engine\Population.h"

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

        Gek::Math::Float3 position;
        Gek::Math::Quaternion rotation;
        Gek::Math::Float3 scale;

        TransformComponent(void);
        HRESULT save(Population::ComponentDefinition &componentData) const;
        HRESULT load(const Population::ComponentDefinition &componentData);

        inline Math::Float4x4 getMatrix(void) const
        {
            return rotation.getMatrix(position);
        }
    };
}; // namespace Gek
