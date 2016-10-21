/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision$
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date$
#pragma once

#include <xmmintrin.h>

namespace Gek
{
    namespace Math
    {
        template <typename TYPE>
        struct RGBA
        {
        public:
            static const RGBA Black;
            static const RGBA White;

        public:
            union
            {
                struct { TYPE r, g, b, a; };
                struct { TYPE data[4]; };
            };

        public:
            RGBA(void)
            {
            }

            RGBA(TYPE value)
                : data{ value, value, value, value }
            {
            }

            RGBA(const TYPE(&data)[4])
                : data{ data[0], data[1], data[2], data[3] }
            {
            }

            RGBA(const TYPE *data)
                : data{ data[0], data[1], data[2], data[3] }
            {
            }

            RGBA(const RGBA &color)
                : r(color.r)
                , g(color.g)
                , b(color.b)
                , a(color.a)
            {
            }

            RGBA(TYPE r, TYPE g, TYPE b, TYPE a)
                : r(r)
                , g(g)
                , b(b)
                , a(a)
            {
            }

            void set(TYPE value)
            {
                r = g = b = a = value;
            }

            void set(TYPE r, TYPE g, TYPE b, TYPE a)
            {
                this->r = r;
                this->g = g;
                this->b = b;
                this->a = a;
            }

            void set(const RGBA &color)
            {
                this->r = color.r;
                this->g = color.g;
                this->b = color.b;
                this->a = color.a;
            }

            float dot(const RGBA &color) const
            {
                return ((r * color.r) + (g * color.g) + (b * color.b) + (a * color.a));
            }

            RGBA lerp(const RGBA &color, float factor) const
            {
                return Math::lerp((*this), color, factor);
            }

            TYPE operator [] (int channel) const
            {
                return data[channel];
            }

            TYPE &operator [] (int channel)
            {
                return data[channel];
            }

            operator const TYPE *() const
            {
                return data;
            }

            operator TYPE *()
            {
                return data;
            }

            // color operations
            RGBA &operator = (const RGBA &color)
            {
                this->r = color.r;
                this->g = color.g;
                this->b = color.b;
                this->a = color.a;
                return (*this);
            }

            void operator -= (const RGBA &color)
            {
                this->r -= color.r;
                this->b -= color.g;
                this->b -= color.b;
                this->a -= color.a;
            }

            void operator += (const RGBA &color)
            {
                this->r += color.r;
                this->b += color.g;
                this->b += color.b;
                this->a += color.a;
            }

            void operator /= (const RGBA &color)
            {
                this->r /= color.r;
                this->b /= color.g;
                this->b /= color.b;
                this->a /= color.a;
            }

            void operator *= (const RGBA &color)
            {
                this->r *= color.r;
                this->b *= color.g;
                this->b *= color.b;
                this->a *= color.a;
            }

            RGBA operator - (const RGBA &color) const
            {
                return RGBA((this->r - color.r),
                             (this->g - color.g),
                             (this->b - color.b),
                             (this->a - color.a));
            }

            RGBA operator + (const RGBA &color) const
            {
                return RGBA((this->r + color.r),
                             (this->g + color.g),
                             (this->b + color.b),
                             (this->a + color.a));
            }

            RGBA operator / (const RGBA &color) const
            {
                return RGBA((this->r / color.r),
                             (this->g / color.g),
                             (this->b / color.b),
                             (this->a / color.a));
            }

            RGBA operator * (const RGBA &color) const
            {
                return RGBA((this->r * color.r),
                             (this->g * color.g),
                             (this->b * color.b),
                             (this->a * color.a));
            }

            // scalar operations
            RGBA &operator = (TYPE scalar)
            {
                this->r = this->g = this->b = this->a = scalar;
                return (*this);
            }

            void operator -= (TYPE scalar)
            {
                this->r -= scalar;
                this->g -= scalar;
                this->b -= scalar;
                this->a -= scalar;
            }

            void operator += (TYPE scalar)
            {
                this->r += scalar;
                this->g += scalar;
                this->b += scalar;
                this->a += scalar;
            }

            void operator /= (TYPE scalar)
            {
                this->r /= scalar;
                this->g /= scalar;
                this->b /= scalar;
                this->a /= scalar;
            }

            void operator *= (TYPE scalar)
            {
                this->r *= scalar;
                this->g *= scalar;
                this->b *= scalar;
                this->a *= scalar;
            }

            RGBA operator - (TYPE scalar) const
            {
                return RGBA((this->r - scalar),
                             (this->g - scalar),
                             (this->b - scalar),
                             (this->a - scalar));
            }

            RGBA operator + (TYPE scalar) const
            {
                return RGBA((this->r + scalar),
                             (this->g + scalar),
                             (this->b + scalar),
                             (this->a + scalar));
            }

            RGBA operator / (TYPE scalar) const
            {
                return RGBA((this->r / scalar),
                             (this->g / scalar),
                             (this->b / scalar),
                             (this->a / scalar));
            }

            RGBA operator * (TYPE scalar) const
            {
                return RGBA((this->r * scalar),
                             (this->g * scalar),
                             (this->b * scalar),
                             (this->a * scalar));
            }
        };

        template <typename TYPE>
        RGBA<TYPE> operator - (const RGBA<TYPE> &color)
        {
            return RGBA<TYPE>(-color.r, -color.g, -color.b, -color.a);
        }

        template <typename TYPE>
        RGBA<TYPE> operator + (TYPE scalar, const RGBA<TYPE> &color)
        {
            return RGBA<TYPE>((scalar + color.r),
                         (scalar + color.g),
                         (scalar + color.b),
                         (scalar + color.a));
        }

        template <typename TYPE>
        RGBA<TYPE> operator - (TYPE scalar, const RGBA<TYPE> &color)
        {
            return RGBA<TYPE>((scalar - color.r),
                         (scalar - color.g),
                         (scalar - color.b),
                         (scalar - color.a));
        }

        template <typename TYPE>
        RGBA<TYPE> operator * (TYPE scalar, const RGBA<TYPE> &color)
        {
            return RGBA<TYPE>((scalar * color.r),
                         (scalar * color.g),
                         (scalar * color.b),
                         (scalar * color.a));
        }

        template <typename TYPE>
        RGBA<TYPE> operator / (TYPE scalar, const RGBA<TYPE> &color)
        {
            return RGBA<TYPE>((scalar / color.r),
                         (scalar / color.g),
                         (scalar / color.b),
                         (scalar / color.a));
        }

        using Color = RGBA<float>;
        using Pixel = RGBA<unsigned char>;
    }; // namespace Math
}; // namespace Gek
