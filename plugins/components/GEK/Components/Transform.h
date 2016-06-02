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

        Math::Float3 position;
        Math::Quaternion rotation;
        Math::Float3 scale;

        TransformComponent(void);
        void save(Population::ComponentDefinition &componentData) const;
        void load(const Population::ComponentDefinition &componentData);

        inline Math::Float4x4 getMatrix(void) const
        {
            return rotation.getMatrix(position);
        }
    };
}; // namespace Gek
