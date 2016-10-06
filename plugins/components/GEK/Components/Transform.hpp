#pragma once

#include "GEK\Math\Float3.hpp"
#include "GEK\Math\Quaternion.hpp"
#include "GEK\Math\Float4x4.hpp"
#include "GEK\Engine\Component.hpp"

namespace Gek
{
    namespace Components
    {
        GEK_COMPONENT(Transform)
        {
            void *operator new(size_t size)
            {
                return _mm_malloc(size * sizeof(Transform), 16);
            }

            void operator delete(void *data)
            {
                _mm_free(data);
            }

            Math::Float3 position;
            Math::Quaternion rotation;
            Math::Float3 scale;

            void save(Xml::Leaf &componentData) const;
            void load(const Xml::Leaf &componentData);

            inline Math::Float4x4 getMatrix(void) const
            {
                auto matrix(rotation.getMatrix());
                matrix.translation = position;
                return matrix;
            }
        };
    }; // namespace Components
}; // namespace Gek
