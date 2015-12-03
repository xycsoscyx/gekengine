#pragma once

#include "GEK\Math\Vector4.h"
#include <atlbase.h>
#include <atlstr.h>
#include <unordered_map>

namespace Gek
{
    struct ColorComponent
    {
        LPVOID operator new(size_t size)
        {
            return _mm_malloc(size * sizeof(ColorComponent), 16);
        }

            void operator delete(LPVOID data)
        {
            _mm_free(data);
        }

        Gek::Math::Float4 value;

        ColorComponent(void);
        HRESULT save(std::unordered_map<CStringW, CStringW> &componentParameterList) const;
        HRESULT load(const std::unordered_map<CStringW, CStringW> &componentParameterList);

        inline operator Gek::Math::Float4&()
        {
            return value;
        }

        inline operator const Gek::Math::Float4&() const
        {
            return value;
        }

        inline Gek::Math::Float4 &operator = (const Gek::Math::Float4 &value)
        {
            this->value = value;
            return this->value;
        }
    };
}; // namespace Gek
