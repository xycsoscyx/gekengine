#pragma once

#include "GEK\Math\Float3.h"
#include "GEK\Math\Quaternion.h"
#include "GEK\Math\Float4x4.h"
#include "GEK\Engine\Population.h"

namespace Gek
{
    namespace Components
    {
        struct Transform
        {
            LPVOID operator new(size_t size)
            {
                return _mm_malloc(size * sizeof(Transform), 16);
            }

            void operator delete(LPVOID data)
            {
                _mm_free(data);
            }

            Math::Float3 position;
            Math::Quaternion rotation;
            Math::Float3 scale;

            Transform(void);
            void save(Plugin::Population::ComponentDefinition &componentData) const;
            void load(const Plugin::Population::ComponentDefinition &componentData);

            inline Math::Float4x4 getMatrix(void) const
            {
                auto matrix(rotation.getMatrix());
                matrix.translation = position;
                return matrix;
            }
        };
    }; // namespace Components
}; // namespace Gek
