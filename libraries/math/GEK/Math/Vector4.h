#pragma once

#include <cmath>
#include <initializer_list>
#include "GEK\Math\Vector3.h"
#include <algorithm>

#if _M_IX86_FP == 1
#define __SSE__ 1
#endif

#if _M_IX86_FP == 2
#define __SSE__ 2
#endif

#if __SSE__ >= 1
#include <xmmintrin.h>
#endif

#if __SSE__ >= 2
#include <emmintrin.h>
#endif

#if __SSE__ >= 3
// SSE3 includes the haddps (horizontal add) instruction
#include <pmmintrin.h>
#endif

#if __SSE__ >= 4
// SSE4.1 includes the dpps (dot product) instruction
#include <smmintrin.h>
#endif

namespace Gek
{
    namespace Math
    {
        template <typename TYPE> struct BaseVector2;
        template <typename TYPE> struct BaseVector3;
        template <typename TYPE> struct BaseQuaternion;

#if __SSE__ >= 1
        namespace SSE
        {
            inline __m128 dot4(__m128 leftValue, __m128 rightValue)
            {
#if __SSE__ >= 4
                __m128 dot = _mm_dp_ps(leftValue, rightValue, 0xff);
#elif __SSE__ >= 3
                __m128 t1 = _mm_mul_ps(leftValue, rightValue);
                __m128 t2 = _mm_hadd_ps(t1, t1);
                __m128 dot = _mm_hadd_ps(t2, t2);
#else
                __m128 t1 = _mm_mul_ps(leftValue, rightValue);
                __m128 t2 = _mm_shuffle_ps(t1, t1, 0x93);
                __m128 t3 = _mm_add_ps(t1, t2);
                __m128 t4 = _mm_shuffle_ps(t3, t3, 0x4e);
                __m128 dot = _mm_add_ps(t3, t4);
#endif
                return dot;
            }
            
            inline float dot(__m128 leftValue, __m128 rightValue)
            {
                return _mm_cvtss_f32(dot4(leftValue, rightValue));
            }
        };
#endif

        template <typename TYPE>
        struct BaseVector4
        {
        public:
            union
            {
                struct { TYPE x, y, z, w; };
                struct { TYPE r, g, b, a; };
                struct { TYPE xyzw[4]; };
                struct { TYPE rgba[4]; };
                struct { BaseVector3<TYPE> xyz; TYPE w; };
                struct { BaseVector3<TYPE> normal; TYPE distance; };
                struct { TYPE data[4]; };
            };

        public:
            BaseVector4(void)
                : x(0)
                , y(0)
                , z(0)
                , w(1)
            {
            }

#if __SSE__ >= 1
            BaseVector4(const __m128 &value)
            {
                _mm_storeu_ps(data, value);
            }
#endif

            BaseVector4(TYPE scalar)
                : x(scalar)
                , y(scalar)
                , z(scalar)
                , w(scalar)
            {
            }

            BaseVector4(const std::initializer_list<float> &list)
            {
                std::copy(list.begin(), list.end(), data);
            }

            BaseVector4(const TYPE *vector)
            {
                std::copy(vector, vector + 4, data);
            }

            BaseVector4(const BaseVector2<TYPE> &vector)
                : x(vector.x)
                , y(vector.y)
                , z(0)
                , w(1)
            {
            }

            BaseVector4(const BaseVector3<TYPE> &vector)
                : x(vector.x)
                , y(vector.y)
                , z(vector.z)
                , w(1)
            {
            }

            BaseVector4(const BaseVector3<TYPE> &vector, TYPE w)
                : x(vector.x)
                , y(vector.y)
                , z(vector.z)
                , w(w)
            {
            }

            BaseVector4(const BaseVector4<TYPE> &vector)
                : BaseVector4(vector.data)
            {
            }

            BaseVector4(TYPE x, TYPE y, TYPE z, TYPE w)
                : x(x)
                , y(y)
                , z(z)
                , w(w)
            {
            }

            void set(TYPE x, TYPE y, TYPE z, TYPE w)
            {
                this->x = x;
                this->y = y;
                this->z = z;
                this->w = w;
            }

            void setLength(TYPE length)
            {
                (*this) *= (length / getLength());
            }

            TYPE getLengthSquared(void) const
            {
#if __SSE__ >= 1
                __m128 value = _mm_loadu_ps(data);
                return SSE::dot(value, value);
#else
                return ((x * x) + (y * y) + (z * z) + (w * w));
#endif
            }

            TYPE getLength(void) const
            {
#if __SSE__ >= 1
                __m128 value = _mm_loadu_ps(data);
                return _mm_cvtss_f32(_mm_sqrt_ss(SSE::dot4(value, value)));
#else
                return std::sqrt(getLengthSquared());
#endif
            }

            TYPE getMax(void) const
            {
                return std::max(max(max(x, y), z), w);
            }

            TYPE getDistance(const BaseVector4<TYPE> &vector) const
            {
                return (vector - (*this)).getLength();
            }

            BaseVector4 getNormal(void) const
            {
#if __SSE__ >= 1
                __m128 value = _mm_loadu_ps(data);
                return BaseVector4(_mm_div_ps(value, _mm_sqrt_ps(SSE::dot4(value, value))));
#else
                TYPE length(getLength());
                if (length != 0.0f)
                {
                    return ((*this) * (1.0f / length));
                }

                return (*this);
#endif
            }

            TYPE dot(const BaseVector4<TYPE> &vector) const
            {
#if __SSE__ >= 1
                return SSE::dot(_mm_loadu_ps(data), _mm_loadu_ps(vector.data));
#else
                return ((x * vector.x) + (y * vector.y) + (z * vector.z) + (w * vector.w));
#endif
            }

            BaseVector4 cross(const BaseVector4<TYPE> &vector) const
            {
#if __SSE__ >= 1
                __m128 t1 = _mm_shuffle_ps(_mm_loadu_ps(data), _mm_loadu_ps(data), 0xc9);
                __m128 t2 = _mm_shuffle_ps(_mm_loadu_ps(data), _mm_loadu_ps(data), 0xd2);
                __m128 t3 = _mm_shuffle_ps(_mm_loadu_ps(vector.data), _mm_loadu_ps(vector.data), 0xd2);
                __m128 t4 = _mm_shuffle_ps(_mm_loadu_ps(vector.data), _mm_loadu_ps(vector.data), 0xc9);
                __m128 t5 = _mm_mul_ps(t1, t3);
                __m128 t6 = _mm_mul_ps(t2, t4);
                return BaseVector4(_mm_sub_ps(t5, t6));
#else
                return BaseVector4(0);
#endif
            }

            BaseVector4 lerp(const BaseVector4<TYPE> &vector, TYPE factor) const
            {
                return lerp((*this), vector, factor);
            }

            void normalize(void)
            {
#if __SSE__ >= 1
                __m128 value = _mm_loadu_ps(data);
                _mm_storeu_ps(data, _mm_div_ps(value, _mm_sqrt_ps(SSE::dot4(value, value))));
#else
                (*this) = getNormal();
#endif
            }

            TYPE operator [] (int axis) const
            {
                return xyzw[axis];
            }

            TYPE &operator [] (int axis)
            {
                return xyzw[axis];
            }

            bool operator < (const BaseVector4<TYPE> &vector) const
            {
                if (x >= vector.x) return false;
                if (y >= vector.y) return false;
                if (z >= vector.z) return false;
                if (w >= vector.w) return false;
                return true;
            }

            bool operator > (const BaseVector4<TYPE> &vector) const
            {
                if (x <= vector.x) return false;
                if (y <= vector.y) return false;
                if (z <= vector.z) return false;
                if (w <= vector.w) return false;
                return true;
            }

            bool operator <= (const BaseVector4<TYPE> &vector) const
            {
                if (x > vector.x) return false;
                if (y > vector.y) return false;
                if (z > vector.z) return false;
                if (w > vector.w) return false;
                return true;
            }

            bool operator >= (const BaseVector4<TYPE> &vector) const
            {
                if (x < vector.x) return false;
                if (y < vector.y) return false;
                if (z < vector.z) return false;
                if (w < vector.w) return false;
                return true;
            }

            bool operator == (const BaseVector4<TYPE> &vector) const
            {
                if (x != vector.x) return false;
                if (y != vector.y) return false;
                if (z != vector.z) return false;
                if (w != vector.w) return false;
                return true;
            }

            bool operator != (const BaseVector4<TYPE> &vector) const
            {
                if (x != vector.x) return true;
                if (y != vector.y) return true;
                if (z != vector.z) return true;
                if (w != vector.w) return true;
                return false;
            }

            BaseVector4 operator = (float scalar)
            {
                x = y = z = w = scalar;
                return (*this);
            }

            BaseVector4 operator = (const BaseVector2<TYPE> &vector)
            {
                x = vector.x;
                y = vector.y;
                z = 0.0f;
                w = 1.0f;
                return (*this);
            }

            BaseVector4 operator = (const BaseVector3<TYPE> &vector)
            {
                x = vector.x;
                y = vector.y;
                z = vector.z;
                w = 1.0f;
                return (*this);
            }

            BaseVector4 operator = (const BaseVector4<TYPE> &vector)
            {
                x = vector.x;
                y = vector.y;
                z = vector.z;
                w = vector.w;
                return (*this);
            }

#if __SSE__ >= 1
            BaseVector4 operator = (const __m128 &value)
            {
                _mm_storeu_ps(data, value);
                return (*this);
            }
#endif

            void operator -= (const BaseVector4<TYPE> &vector)
            {
#if __SSE__ >= 1
                _mm_storeu_ps(data, _mm_sub_ps(_mm_loadu_ps(data), _mm_loadu_ps(vector.data)));
#else
                x -= vector.x;
                y -= vector.y;
                z -= vector.z;
                w -= vector.w;
#endif
            }

            void operator += (const BaseVector4<TYPE> &vector)
            {
#if __SSE__ >= 1
                _mm_storeu_ps(data, _mm_add_ps(_mm_loadu_ps(data), _mm_loadu_ps(vector.data)));
#else
                x += vector.x;
                y += vector.y;
                z += vector.z;
                w += vector.w;
#endif
            }

            void operator /= (const BaseVector4<TYPE> &vector)
            {
#if __SSE__ >= 1
                _mm_storeu_ps(data, _mm_div_ps(_mm_loadu_ps(data), _mm_loadu_ps(vector.data)));
#else
                x /= vector.x;
                y /= vector.y;
                z /= vector.z;
                w /= vector.w;
#endif
            }

            void operator *= (const BaseVector4<TYPE> &vector)
            {
#if __SSE__ >= 1
                _mm_storeu_ps(data, _mm_mul_ps(_mm_loadu_ps(data), _mm_loadu_ps(vector.data)));
#else
                x *= vector.x;
                y *= vector.y;
                z *= vector.z;
                w *= vector.w;
#endif
            }

            void operator -= (TYPE scalar)
            {
#if __SSE__ >= 1
                _mm_storeu_ps(data, _mm_sub_ps(_mm_loadu_ps(data), _mm_set1_ps(scalar)));
#else
                x -= scalar;
                y -= scalar;
                z -= scalar;
                w -= scalar;
#endif
            }

            void operator += (TYPE scalar)
            {
#if __SSE__ >= 1
                _mm_storeu_ps(data, _mm_add_ps(_mm_loadu_ps(data), _mm_set1_ps(scalar)));
#else
                x += scalar;
                y += scalar;
                z += scalar;
                w += scalar;
#endif
            }

            void operator /= (TYPE scalar)
            {
#if __SSE__ >= 1
                _mm_storeu_ps(data, _mm_div_ps(_mm_loadu_ps(data), _mm_set1_ps(scalar)));
#else
                x /= scalar;
                y /= scalar;
                z /= scalar;
                w /= scalar;
#endif
            }

            void operator *= (TYPE scalar)
            {
#if __SSE__ >= 1
                _mm_storeu_ps(data, _mm_mul_ps(_mm_loadu_ps(data), _mm_set1_ps(scalar)));
#else
                x *= scalar;
                y *= scalar;
                z *= scalar;
                w *= scalar;
#endif
            }

            BaseVector4 operator - (const BaseVector4<TYPE> &vector) const
            {
#if __SSE__ >= 1
                return BaseVector4(_mm_sub_ps(_mm_loadu_ps(data), _mm_loadu_ps(vector.data)));
#else
                return BaseVector4((x - vector.x), (y - vector.y), (z - vector.z), (w - vector.w));
#endif
            }

            BaseVector4 operator + (const BaseVector4<TYPE> &vector) const
            {
#if __SSE__ >= 1
                return BaseVector4(_mm_add_ps(_mm_loadu_ps(data), _mm_loadu_ps(vector.data)));
#else
                return BaseVector4((x + vector.x), (y + vector.y), (z + vector.z), (w + vector.w));
#endif
            }

            BaseVector4 operator / (const BaseVector4<TYPE> &vector) const
            {
#if __SSE__ >= 1
                return BaseVector4(_mm_div_ps(_mm_loadu_ps(data), _mm_loadu_ps(vector.data)));
#else
                return BaseVector4((x / vector.x), (y / vector.y), (z / vector.z), (w / vector.w));
#endif
            }

            BaseVector4 operator * (const BaseVector4<TYPE> &vector) const
            {
#if __SSE__ >= 1
                return BaseVector4(_mm_mul_ps(_mm_loadu_ps(data), _mm_loadu_ps(vector.data)));
#else
                return BaseVector4((x * vector.x), (y * vector.y), (z * vector.z), (w * vector.w));
#endif
            }

            BaseVector4 operator - (TYPE scalar) const
            {
#if __SSE__ >= 1
                return BaseVector4(_mm_sub_ps(_mm_loadu_ps(data), _mm_set1_ps(scalar)));
#else
                return BaseVector4((x - scalar), (y - scalar), (z - scalar), (w - scalar));
#endif
            }

            BaseVector4 operator + (TYPE scalar) const
            {
#if __SSE__ >= 1
                return BaseVector4(_mm_add_ps(_mm_loadu_ps(data), _mm_set1_ps(scalar)));
#else
                return BaseVector4((x + scalar), (y + scalar), (z + scalar), (w + scalar));
#endif
            }

            BaseVector4 operator / (TYPE scalar) const
            {
#if __SSE__ >= 1
                return BaseVector4(_mm_div_ps(_mm_loadu_ps(data), _mm_set1_ps(scalar)));
#else
                return BaseVector4((x / scalar), (y / scalar), (z / scalar), (w / scalar));
#endif
            }

            BaseVector4 operator * (TYPE scalar) const
            {
#if __SSE__ >= 1
                return BaseVector4(_mm_mul_ps(_mm_loadu_ps(data), _mm_set1_ps(scalar)));
#else
                return BaseVector4((x * scalar), (y * scalar), (z * scalar), (w * scalar));
#endif
            }
        };

        template <typename TYPE>
        BaseVector4<TYPE> operator - (const BaseVector4<TYPE> &vector)
        {
#if __SSE__ >= 1
            return BaseVector4(_mm_sub_ps(_mm_set1_ps(0), _mm_loadu_ps(vector.data)));
#else
            return BaseVector4(-vector.x, -vector.y, -vector.z, -vector.w);
#endif
        }

        template <typename TYPE>
        BaseVector4<TYPE> operator - (TYPE scalar, const BaseVector4<TYPE> &vector)
        {
#if __SSE__ >= 1
            return BaseVector4(_mm_sub_ps(_mm_set1_ps(scalar), _mm_loadu_ps(vector.data)));
#else
            return BaseVector4((scalar - vector.x), (scalar - vector.y), (scalar - vector.z), (scalar - vector.w));
#endif
        }

        template <typename TYPE>
        BaseVector4<TYPE> operator + (TYPE scalar, const BaseVector4<TYPE> &vector)
        {
#if __SSE__ >= 1
            return BaseVector4(_mm_add_ps(_mm_set1_ps(scalar), _mm_loadu_ps(vector.data)));
#else
            return BaseVector4((scalar + vector.x), (scalar + vector.y), (scalar + vector.z), (scalar + vector.w));
#endif
        }

        template <typename TYPE>
        BaseVector4<TYPE> operator / (TYPE scalar, const BaseVector4<TYPE> &vector)
        {
#if __SSE__ >= 1
            return BaseVector4(_mm_div_ps(_mm_set1_ps(scalar), _mm_loadu_ps(vector.data)));
#else
            return BaseVector4((scalar / vector.x), (scalar / vector.y), (scalar / vector.z), (scalar / vector.w));
#endif
        }

        template <typename TYPE>
        BaseVector4<TYPE> operator * (TYPE scalar, const BaseVector4<TYPE> &vector)
        {
#if __SSE__ >= 1
            return BaseVector4(_mm_mul_ps(_mm_set1_ps(scalar), _mm_loadu_ps(vector.data)));
#else
            return BaseVector4((scalar * vector.x), (scalar * vector.y), (scalar * vector.z), (scalar * vector.w));
#endif
        }

        typedef BaseVector4<float> Float4;
    }; // namespace Math
}; // namespace Gek
