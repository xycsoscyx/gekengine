/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision$
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date$
#pragma once

#include "GEK\Math\Float3.hpp"
#include "GEK\Math\Float4.hpp"
#include <xmmintrin.h>

namespace Gek
{
    namespace Math
    {
        struct Color
        {
        public:
            static const Color Black;
            static const Color White;

        public:
            union
            {
                struct { float r, g, b, a; };
                struct { float data[4]; };
            };

        public:
            inline Color(void)
            {
            }

            inline Color(float value)
                : data{ value, value, value, value }
            {
            }

            inline Color(const float(&data)[4])
                : data{ data[0], data[1], data[2], data[3] }
            {
            }

            inline Color(const float *data)
                : data{ data[0], data[1], data[2], data[3] }
            {
            }

            inline Color(const Color &color)
                : r(color.r)
                , g(color.g)
                , b(color.b)
                , a(color.a)
            {
            }

            inline Color(float r, float g, float b, float a)
                : r(r)
                , g(g)
                , b(b)
                , a(a)
            {
            }

            Color(const Float3 &rgb);
            Color(const Float4 &rgba);

            Float3 getXYZ(void) const;
            Float4 getXYZW(void) const;
            __declspec(property(get = getXYZ)) Float3 xyz;
            __declspec(property(get = getXYZW)) Float4 xyzw;

            inline void set(float value)
            {
                r = g = b = a = value;
            }

            inline void set(float r, float g, float b, float a)
            {
                this->r = r;
                this->g = g;
                this->b = b;
                this->a = a;
            }

            inline void set(const Color &color)
            {
                this->r = color.r;
                this->g = color.g;
                this->b = color.b;
                this->a = color.a;
            }

            void set(const Float3 &vector);
            void set(const Float4 &vector);

            float dot(const Color &color) const;
            Color lerp(const Color &color, float factor) const;

            inline float operator [] (int channel) const
            {
                return data[channel];
            }

            inline float &operator [] (int channel)
            {
                return data[channel];
            }

            inline operator const float *() const
            {
                return data;
            }

            inline operator float *()
            {
                return data;
            }

            // color operations
            inline Color &operator = (const Color &color)
            {
                this->r = color.r;
                this->g = color.g;
                this->b = color.b;
                this->a = color.a;
                return (*this);
            }

            inline void operator -= (const Color &color)
            {
                this->r -= color.r;
                this->b -= color.g;
                this->b -= color.b;
                this->a -= color.a;
            }

            inline void operator += (const Color &color)
            {
                this->r += color.r;
                this->b += color.g;
                this->b += color.b;
                this->a += color.a;
            }

            inline void operator /= (const Color &color)
            {
                this->r /= color.r;
                this->b /= color.g;
                this->b /= color.b;
                this->a /= color.a;
            }

            inline void operator *= (const Color &color)
            {
                this->r *= color.r;
                this->b *= color.g;
                this->b *= color.b;
                this->a *= color.a;
            }

            inline Color operator - (const Color &color) const
            {
                return Color((this->r - color.r),
                             (this->g - color.g),
                             (this->b - color.b),
                             (this->a - color.a));
            }

            inline Color operator + (const Color &color) const
            {
                return Color((this->r + color.r),
                             (this->g + color.g),
                             (this->b + color.b),
                             (this->a + color.a));
            }

            inline Color operator / (const Color &color) const
            {
                return Color((this->r / color.r),
                             (this->g / color.g),
                             (this->b / color.b),
                             (this->a / color.a));
            }

            inline Color operator * (const Color &color) const
            {
                return Color((this->r * color.r),
                             (this->g * color.g),
                             (this->b * color.b),
                             (this->a * color.a));
            }

            // scalar operations
            inline Color &operator = (float scalar)
            {
                this->r = this->g = this->b = this->a = scalar;
                return (*this);
            }

            inline void operator -= (float scalar)
            {
                this->r -= scalar;
                this->g -= scalar;
                this->b -= scalar;
                this->a -= scalar;
            }

            inline void operator += (float scalar)
            {
                this->r += scalar;
                this->g += scalar;
                this->b += scalar;
                this->a += scalar;
            }

            inline void operator /= (float scalar)
            {
                this->r /= scalar;
                this->g /= scalar;
                this->b /= scalar;
                this->a /= scalar;
            }

            inline void operator *= (float scalar)
            {
                this->r *= scalar;
                this->g *= scalar;
                this->b *= scalar;
                this->a *= scalar;
            }

            inline Color operator - (float scalar) const
            {
                return Color((this->r - scalar),
                             (this->g - scalar),
                             (this->b - scalar),
                             (this->a - scalar));
            }

            inline Color operator + (float scalar) const
            {
                return Color((this->r + scalar),
                             (this->g + scalar),
                             (this->b + scalar),
                             (this->a + scalar));
            }

            inline Color operator / (float scalar) const
            {
                return Color((this->r / scalar),
                             (this->g / scalar),
                             (this->b / scalar),
                             (this->a / scalar));
            }

            inline Color operator * (float scalar) const
            {
                return Color((this->r * scalar),
                             (this->g * scalar),
                             (this->b * scalar),
                             (this->a * scalar));
            }
        };

        inline Color operator - (const Color &color)
        {
            return Color(-color.r, -color.g, -color.b, -color.a);
        }

        inline Color operator + (float scalar, const Color &color)
        {
            return Color((scalar + color.r),
                         (scalar + color.g),
                         (scalar + color.b),
                         (scalar + color.a));
        }

        inline Color operator - (float scalar, const Color &color)
        {
            return Color((scalar - color.r),
                         (scalar - color.g),
                         (scalar - color.b),
                         (scalar - color.a));
        }

        inline Color operator * (float scalar, const Color &color)
        {
            return Color((scalar * color.r),
                         (scalar * color.g),
                         (scalar * color.b),
                         (scalar * color.a));
        }

        inline Color operator / (float scalar, const Color &color)
        {
            return Color((scalar / color.r),
                         (scalar / color.g),
                         (scalar / color.b),
                         (scalar / color.a));
        }
    }; // namespace Math
}; // namespace Gek
