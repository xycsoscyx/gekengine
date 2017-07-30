#include "GEK/GUI/Gizmo.hpp"
#include "GEK/Shapes/Plane.hpp"

namespace Gek
{
    namespace UI
    {
        namespace Gizmo
        {
            const float screenRotateSize = 0.06f;

            static const float angleLimit = 0.96f;
            static const float planeLimit = 0.2f;

            void FPU_MatrixF_x_MatrixF(const float *a, const float *b, float *r)
            {
                r[0] = a[0] * b[0] + a[1] * b[4] + a[2] * b[8] + a[3] * b[12];
                r[1] = a[0] * b[1] + a[1] * b[5] + a[2] * b[9] + a[3] * b[13];
                r[2] = a[0] * b[2] + a[1] * b[6] + a[2] * b[10] + a[3] * b[14];
                r[3] = a[0] * b[3] + a[1] * b[7] + a[2] * b[11] + a[3] * b[15];

                r[4] = a[4] * b[0] + a[5] * b[4] + a[6] * b[8] + a[7] * b[12];
                r[5] = a[4] * b[1] + a[5] * b[5] + a[6] * b[9] + a[7] * b[13];
                r[6] = a[4] * b[2] + a[5] * b[6] + a[6] * b[10] + a[7] * b[14];
                r[7] = a[4] * b[3] + a[5] * b[7] + a[6] * b[11] + a[7] * b[15];

                r[8] = a[8] * b[0] + a[9] * b[4] + a[10] * b[8] + a[11] * b[12];
                r[9] = a[8] * b[1] + a[9] * b[5] + a[10] * b[9] + a[11] * b[13];
                r[10] = a[8] * b[2] + a[9] * b[6] + a[10] * b[10] + a[11] * b[14];
                r[11] = a[8] * b[3] + a[9] * b[7] + a[10] * b[11] + a[11] * b[15];

                r[12] = a[12] * b[0] + a[13] * b[4] + a[14] * b[8] + a[15] * b[12];
                r[13] = a[12] * b[1] + a[13] * b[5] + a[14] * b[9] + a[15] * b[13];
                r[14] = a[12] * b[2] + a[13] * b[6] + a[14] * b[10] + a[15] * b[14];
                r[15] = a[12] * b[3] + a[13] * b[7] + a[14] * b[11] + a[15] * b[15];
            }

            struct matrix_t;
            struct vec_t
            {
            public:
                float x, y, z, w;

                void Lerp(const vec_t& v, float t)
                {
                    x += (v.x - x) * t;
                    y += (v.y - y) * t;
                    z += (v.z - z) * t;
                    w += (v.w - w) * t;
                }

                void Set(float v)
                {
                    x = y = z = w = v;
                }

                void Set(float _x, float _y, float _z = 0.0f, float _w = 0.0f)
                {
                    x = _x;
                    y = _y;
                    z = _z;
                    w = _w;
                }

                vec_t& operator -= (const vec_t& v)
                {
                    x -= v.x;
                    y -= v.y;
                    z -= v.z;
                    w -= v.w;
                    return *this;
                }

                vec_t& operator += (const vec_t& v)
                {
                    x += v.x;
                    y += v.y;
                    z += v.z;
                    w += v.w;
                    return *this;
                }

                vec_t& operator *= (const vec_t& v)
                {
                    x *= v.x;
                    y *= v.y;
                    z *= v.z;
                    w *= v.w;
                    return *this;
                }

                vec_t& operator *= (float v)
                {
                    x *= v;
                    y *= v;
                    z *= v;
                    w *= v;
                    return *this;
                }

                vec_t operator * (float f) const;
                vec_t operator - () const;
                vec_t operator - (const vec_t& v) const;
                vec_t operator + (const vec_t& v) const;
                vec_t operator * (const vec_t& v) const;

                const vec_t& operator + () const
                {
                    return (*this);
                }

                float Length() const
                {
                    return std::sqrt(x*x + y*y + z*z);
                }

                float LengthSq() const
                {
                    return (x*x + y*y + z*z);
                }

                vec_t Normalize()
                {
                    (*this) *= (1.0f / Length());
                    return (*this);
                }

                vec_t Normalize(const vec_t& v)
                {
                    this->Set(v.x, v.y, v.z, v.w);
                    this->Normalize();
                    return (*this);
                }

                vec_t Abs() const;

                void Cross(const vec_t& v)
                {
                    vec_t res;
                    res.x = y * v.z - z * v.y;
                    res.y = z * v.x - x * v.z;
                    res.z = x * v.y - y * v.x;

                    x = res.x;
                    y = res.y;
                    z = res.z;
                    w = 0.0f;
                }

                void Cross(const vec_t& v1, const vec_t& v2)
                {
                    x = v1.y * v2.z - v1.z * v2.y;
                    y = v1.z * v2.x - v1.x * v2.z;
                    z = v1.x * v2.y - v1.y * v2.x;
                    w = 0.0f;
                }

                float Dot(const vec_t &v) const
                {
                    return (x * v.x) + (y * v.y) + (z * v.z) + (w * v.w);
                }

                float Dot3(const vec_t &v) const
                {
                    return (x * v.x) + (y * v.y) + (z * v.z);
                }

                void Transform(const matrix_t& matrix);
                void Transform(const vec_t & s, const matrix_t& matrix);

                void TransformVector(const matrix_t& matrix);
                void TransformPoint(const matrix_t& matrix);
                void TransformVector(const vec_t& v, const matrix_t& matrix)
                {
                    (*this) = v;
                    this->TransformVector(matrix);
                }

                void TransformPoint(const vec_t& v, const matrix_t& matrix)
                {
                    (*this) = v;
                    this->TransformPoint(matrix);
                }

                float& operator [] (size_t index)
                {
                    return ((float*)&x)[index];
                }

                const float& operator [] (size_t index) const
                {
                    return ((float*)&x)[index];
                }
            };

            vec_t makeVect(float _x, float _y, float _z = 0.0f, float _w = 0.0f)
            {
                vec_t res;
                res.x = _x;
                res.y = _y;
                res.z = _z;
                res.w = _w;
                return res;
            }

            vec_t vec_t::operator * (float f) const
            {
                return makeVect(x * f, y * f, z * f, w *f);
            }

            vec_t vec_t::operator - () const
            {
                return makeVect(-x, -y, -z, -w);
            }

            vec_t vec_t::operator - (const vec_t& v) const
            {
                return makeVect(x - v.x, y - v.y, z - v.z, w - v.w);
            }

            vec_t vec_t::operator + (const vec_t& v) const
            {
                return makeVect(x + v.x, y + v.y, z + v.z, w + v.w);
            }

            vec_t vec_t::operator * (const vec_t& v) const
            {
                return makeVect(x * v.x, y * v.y, z * v.z, w * v.w);
            }

            vec_t vec_t::Abs() const
            {
                return makeVect(std::abs(x), std::abs(y), std::abs(z));
            }

            ImVec2 operator+ (const ImVec2& a, const ImVec2& b)
            {
                return ImVec2(a.x + b.x, a.y + b.y);
            }

            vec_t Normalized(const vec_t& v)
            {
                vec_t res;
                res = v;
                res.Normalize();
                return res;
            }

            vec_t Cross(const vec_t& v1, const vec_t& v2)
            {
                vec_t res;
                res.x = v1.y * v2.z - v1.z * v2.y;
                res.y = v1.z * v2.x - v1.x * v2.z;
                res.z = v1.x * v2.y - v1.y * v2.x;
                res.w = 0.0f;
                return res;
            }

            float Dot(const vec_t &v1, const vec_t &v2)
            {
                return (v1.x * v2.x) + (v1.y * v2.y) + (v1.z * v2.z);
            }

            vec_t BuildPlane(const vec_t & p_point1, const vec_t & p_normal)
            {
                vec_t normal, res;
                normal.Normalize(p_normal);
                res.w = normal.Dot(p_point1);
                res.x = normal.x;
                res.y = normal.y;
                res.z = normal.z;
                return res;
            }

            struct matrix_t
            {
            public:
                union
                {
                    float m[4][4];
                    float m16[16];
                    struct
                    {
                        vec_t right, up, dir, position;
                    } v;

                    vec_t component[4];
                };

                matrix_t(const matrix_t& other)
                {
                    memcpy(&m16[0], &other.m16[0], sizeof(float) * 16);
                }

                matrix_t()
                {
                }

                operator float * ()
                {
                    return m16;
                }

                operator const float* () const
                {
                    return m16;
                }

                void Translation(float _x, float _y, float _z)
                {
                    this->Translation(makeVect(_x, _y, _z));
                }

                void Translation(const vec_t& vt)
                {
                    v.right.Set(1.0f, 0.0f, 0.0f, 0.0f);
                    v.up.Set(0.0f, 1.0f, 0.0f, 0.0f);
                    v.dir.Set(0.0f, 0.0f, 1.0f, 0.0f);
                    v.position.Set(vt.x, vt.y, vt.z, 1.0f);
                }

                void Scale(float _x, float _y, float _z)
                {
                    v.right.Set(_x, 0.0f, 0.0f, 0.0f);
                    v.up.Set(0.0f, _y, 0.0f, 0.0f);
                    v.dir.Set(0.0f, 0.0f, _z, 0.0f);
                    v.position.Set(0.0f, 0.0f, 0.0f, 1.0f);
                }

                void Scale(const vec_t& s)
                {
                    Scale(s.x, s.y, s.z);
                }

                matrix_t& operator *= (const matrix_t& mat)
                {
                    matrix_t tmpMat;
                    tmpMat = *this;
                    tmpMat.Multiply(mat);
                    *this = tmpMat;
                    return *this;
                }

                matrix_t operator * (const matrix_t& mat) const
                {
                    matrix_t matT;
                    matT.Multiply(*this, mat);
                    return matT;
                }

                void Multiply(const matrix_t &matrix)
                {
                    matrix_t tmp;
                    tmp = *this;

                    FPU_MatrixF_x_MatrixF((float*)&tmp, (float*)&matrix, (float*)this);
                }

                void Multiply(const matrix_t &m1, const matrix_t &m2)
                {
                    FPU_MatrixF_x_MatrixF((float*)&m1, (float*)&m2, (float*)this);
                }

                float GetDeterminant() const
                {
                    return
                        m[0][0] * m[1][1] * m[2][2] + 
                        m[0][1] * m[1][2] * m[2][0] + 
                        m[0][2] * m[1][0] * m[2][1] -
                        m[0][2] * m[1][1] * m[2][0] -
                        m[0][1] * m[1][0] * m[2][2] -
                        m[0][0] * m[1][2] * m[2][1];
                }

                float Inverse(const matrix_t &srcMatrix, bool affine = false);

                void SetToIdentity()
                {
                    v.right.Set(1.0f, 0.0f, 0.0f, 0.0f);
                    v.up.Set(0.0f, 1.0f, 0.0f, 0.0f);
                    v.dir.Set(0.0f, 0.0f, 1.0f, 0.0f);
                    v.position.Set(0.0f, 0.0f, 0.0f, 1.0f);
                }

                void Transpose()
                {
                    matrix_t tmpm;
                    for (int l = 0; l < 4; l++)
                    {
                        for (int c = 0; c < 4; c++)
                        {
                            tmpm.m[l][c] = m[c][l];
                        }
                    }

                    (*this) = tmpm;
                }

                void RotationAxis(const vec_t & axis, float angle);

                void OrthoNormalize()
                {
                    v.right.Normalize();
                    v.up.Normalize();
                    v.dir.Normalize();
                }
            };

            void vec_t::Transform(const matrix_t& matrix)
            {
                vec_t out;
                out.x = x * matrix.m[0][0] + y * matrix.m[1][0] + z * matrix.m[2][0] + w * matrix.m[3][0];
                out.y = x * matrix.m[0][1] + y * matrix.m[1][1] + z * matrix.m[2][1] + w * matrix.m[3][1];
                out.z = x * matrix.m[0][2] + y * matrix.m[1][2] + z * matrix.m[2][2] + w * matrix.m[3][2];
                out.w = x * matrix.m[0][3] + y * matrix.m[1][3] + z * matrix.m[2][3] + w * matrix.m[3][3];
                x = out.x;
                y = out.y;
                z = out.z;
                w = out.w;
            }

            void vec_t::Transform(const vec_t & s, const matrix_t& matrix)
            {
                *this = s;
                Transform(matrix);
            }

            void vec_t::TransformPoint(const matrix_t& matrix)
            {
                vec_t out;
                out.x = x * matrix.m[0][0] + y * matrix.m[1][0] + z * matrix.m[2][0] + matrix.m[3][0];
                out.y = x * matrix.m[0][1] + y * matrix.m[1][1] + z * matrix.m[2][1] + matrix.m[3][1];
                out.z = x * matrix.m[0][2] + y * matrix.m[1][2] + z * matrix.m[2][2] + matrix.m[3][2];
                out.w = x * matrix.m[0][3] + y * matrix.m[1][3] + z * matrix.m[2][3] + matrix.m[3][3];
                x = out.x;
                y = out.y;
                z = out.z;
                w = out.w;
            }


            void vec_t::TransformVector(const matrix_t& matrix)
            {
                vec_t out;
                out.x = x * matrix.m[0][0] + y * matrix.m[1][0] + z * matrix.m[2][0];
                out.y = x * matrix.m[0][1] + y * matrix.m[1][1] + z * matrix.m[2][1];
                out.z = x * matrix.m[0][2] + y * matrix.m[1][2] + z * matrix.m[2][2];
                out.w = x * matrix.m[0][3] + y * matrix.m[1][3] + z * matrix.m[2][3];
                x = out.x;
                y = out.y;
                z = out.z;
                w = out.w;
            }

            float matrix_t::Inverse(const matrix_t &srcMatrix, bool affine)
            {
                float det = 0;
                if (affine)
                {
                    det = GetDeterminant();
                    float s = 1 / det;
                    m[0][0] = (srcMatrix.m[1][1] * srcMatrix.m[2][2] - srcMatrix.m[1][2] * srcMatrix.m[2][1]) * s;
                    m[0][1] = (srcMatrix.m[2][1] * srcMatrix.m[0][2] - srcMatrix.m[2][2] * srcMatrix.m[0][1]) * s;
                    m[0][2] = (srcMatrix.m[0][1] * srcMatrix.m[1][2] - srcMatrix.m[0][2] * srcMatrix.m[1][1]) * s;
                    m[1][0] = (srcMatrix.m[1][2] * srcMatrix.m[2][0] - srcMatrix.m[1][0] * srcMatrix.m[2][2]) * s;
                    m[1][1] = (srcMatrix.m[2][2] * srcMatrix.m[0][0] - srcMatrix.m[2][0] * srcMatrix.m[0][2]) * s;
                    m[1][2] = (srcMatrix.m[0][2] * srcMatrix.m[1][0] - srcMatrix.m[0][0] * srcMatrix.m[1][2]) * s;
                    m[2][0] = (srcMatrix.m[1][0] * srcMatrix.m[2][1] - srcMatrix.m[1][1] * srcMatrix.m[2][0]) * s;
                    m[2][1] = (srcMatrix.m[2][0] * srcMatrix.m[0][1] - srcMatrix.m[2][1] * srcMatrix.m[0][0]) * s;
                    m[2][2] = (srcMatrix.m[0][0] * srcMatrix.m[1][1] - srcMatrix.m[0][1] * srcMatrix.m[1][0]) * s;
                    m[3][0] = -(m[0][0] * srcMatrix.m[3][0] + m[1][0] * srcMatrix.m[3][1] + m[2][0] * srcMatrix.m[3][2]);
                    m[3][1] = -(m[0][1] * srcMatrix.m[3][0] + m[1][1] * srcMatrix.m[3][1] + m[2][1] * srcMatrix.m[3][2]);
                    m[3][2] = -(m[0][2] * srcMatrix.m[3][0] + m[1][2] * srcMatrix.m[3][1] + m[2][2] * srcMatrix.m[3][2]);
                }
                else
                {
                    // transpose matrix
                    float src[16];
                    for (int i = 0; i < 4; ++i)
                    {
                        src[i] = srcMatrix.m16[i * 4];
                        src[i + 4] = srcMatrix.m16[i * 4 + 1];
                        src[i + 8] = srcMatrix.m16[i * 4 + 2];
                        src[i + 12] = srcMatrix.m16[i * 4 + 3];
                    }

                    // calculate pairs for first 8 elements (cofactors)
                    float tmp[12]; // temp array for pairs
                    tmp[0] = src[10] * src[15];
                    tmp[1] = src[11] * src[14];
                    tmp[2] = src[9] * src[15];
                    tmp[3] = src[11] * src[13];
                    tmp[4] = src[9] * src[14];
                    tmp[5] = src[10] * src[13];
                    tmp[6] = src[8] * src[15];
                    tmp[7] = src[11] * src[12];
                    tmp[8] = src[8] * src[14];
                    tmp[9] = src[10] * src[12];
                    tmp[10] = src[8] * src[13];
                    tmp[11] = src[9] * src[12];

                    // calculate first 8 elements (cofactors)
                    m16[0] = (tmp[0] * src[5] + tmp[3] * src[6] + tmp[4] * src[7]) - (tmp[1] * src[5] + tmp[2] * src[6] + tmp[5] * src[7]);
                    m16[1] = (tmp[1] * src[4] + tmp[6] * src[6] + tmp[9] * src[7]) - (tmp[0] * src[4] + tmp[7] * src[6] + tmp[8] * src[7]);
                    m16[2] = (tmp[2] * src[4] + tmp[7] * src[5] + tmp[10] * src[7]) - (tmp[3] * src[4] + tmp[6] * src[5] + tmp[11] * src[7]);
                    m16[3] = (tmp[5] * src[4] + tmp[8] * src[5] + tmp[11] * src[6]) - (tmp[4] * src[4] + tmp[9] * src[5] + tmp[10] * src[6]);
                    m16[4] = (tmp[1] * src[1] + tmp[2] * src[2] + tmp[5] * src[3]) - (tmp[0] * src[1] + tmp[3] * src[2] + tmp[4] * src[3]);
                    m16[5] = (tmp[0] * src[0] + tmp[7] * src[2] + tmp[8] * src[3]) - (tmp[1] * src[0] + tmp[6] * src[2] + tmp[9] * src[3]);
                    m16[6] = (tmp[3] * src[0] + tmp[6] * src[1] + tmp[11] * src[3]) - (tmp[2] * src[0] + tmp[7] * src[1] + tmp[10] * src[3]);
                    m16[7] = (tmp[4] * src[0] + tmp[9] * src[1] + tmp[10] * src[2]) - (tmp[5] * src[0] + tmp[8] * src[1] + tmp[11] * src[2]);

                    // calculate pairs for second 8 elements (cofactors)
                    tmp[0] = src[2] * src[7];
                    tmp[1] = src[3] * src[6];
                    tmp[2] = src[1] * src[7];
                    tmp[3] = src[3] * src[5];
                    tmp[4] = src[1] * src[6];
                    tmp[5] = src[2] * src[5];
                    tmp[6] = src[0] * src[7];
                    tmp[7] = src[3] * src[4];
                    tmp[8] = src[0] * src[6];
                    tmp[9] = src[2] * src[4];
                    tmp[10] = src[0] * src[5];
                    tmp[11] = src[1] * src[4];

                    // calculate second 8 elements (cofactors)
                    m16[8] = (tmp[0] * src[13] + tmp[3] * src[14] + tmp[4] * src[15]) - (tmp[1] * src[13] + tmp[2] * src[14] + tmp[5] * src[15]);
                    m16[9] = (tmp[1] * src[12] + tmp[6] * src[14] + tmp[9] * src[15]) - (tmp[0] * src[12] + tmp[7] * src[14] + tmp[8] * src[15]);
                    m16[10] = (tmp[2] * src[12] + tmp[7] * src[13] + tmp[10] * src[15]) - (tmp[3] * src[12] + tmp[6] * src[13] + tmp[11] * src[15]);
                    m16[11] = (tmp[5] * src[12] + tmp[8] * src[13] + tmp[11] * src[14]) - (tmp[4] * src[12] + tmp[9] * src[13] + tmp[10] * src[14]);
                    m16[12] = (tmp[2] * src[10] + tmp[5] * src[11] + tmp[1] * src[9]) - (tmp[4] * src[11] + tmp[0] * src[9] + tmp[3] * src[10]);
                    m16[13] = (tmp[8] * src[11] + tmp[0] * src[8] + tmp[7] * src[10]) - (tmp[6] * src[10] + tmp[9] * src[11] + tmp[1] * src[8]);
                    m16[14] = (tmp[6] * src[9] + tmp[11] * src[11] + tmp[3] * src[8]) - (tmp[10] * src[11] + tmp[2] * src[8] + tmp[7] * src[9]);
                    m16[15] = (tmp[10] * src[10] + tmp[4] * src[8] + tmp[9] * src[9]) - (tmp[8] * src[9] + tmp[11] * src[10] + tmp[5] * src[8]);

                    // calculate determinant
                    det = src[0] * m16[0] + src[1] * m16[1] + src[2] * m16[2] + src[3] * m16[3];

                    // calculate matrix inverse
                    float invdet = 1 / det;
                    for (int j = 0; j < 16; ++j)
                    {
                        m16[j] *= invdet;
                    }
                }

                return det;
            }

            void matrix_t::RotationAxis(const vec_t & axis, float angle)
            {
                float length2 = axis.LengthSq();
                if (length2 < FLT_EPSILON)
                {
                    SetToIdentity();
                    return;
                }

                vec_t n = axis * (1.0f / std::sqrt(length2));
                float s = std::sin(angle);
                float c = std::cos(angle);
                float k = 1.0f - c;

                float xx = n.x * n.x * k + c;
                float yy = n.y * n.y * k + c;
                float zz = n.z * n.z * k + c;
                float xy = n.x * n.y * k;
                float yz = n.y * n.z * k;
                float zx = n.z * n.x * k;
                float xs = n.x * s;
                float ys = n.y * s;
                float zs = n.z * s;

                m[0][0] = xx;
                m[0][1] = xy + zs;
                m[0][2] = zx - ys;
                m[0][3] = 0.0f;
                m[1][0] = xy - zs;
                m[1][1] = yy;
                m[1][2] = yz + xs;
                m[1][3] = 0.0f;
                m[2][0] = zx + ys;
                m[2][1] = yz - xs;
                m[2][2] = zz;
                m[2][3] = 0.0f;
                m[3][0] = 0.0f;
                m[3][1] = 0.0f;
                m[3][2] = 0.0f;
                m[3][3] = 1.0f;
            }

            static const vec_t directionUnary[3] =
            {
                makeVect(1.0f, 0.0f, 0.0f),
                makeVect(0.0f, 1.0f, 0.0f),
                makeVect(0.0f, 0.0f, 1.0f)
            };

            static const ImU32 directionColor[3] =
            {
                0xFF0000AA,
                0xFF00AA00,
                0xFFAA0000
            };

            static const ImU32 selectionColor = 0xFF1080FF;
            static const ImU32 inactiveColor = 0x99999999;
            static const ImU32 translationLineColor = 0xAAAAAAAA;
            static const char *translationInfoMask[] =
            {
                "X : %5.3f",
                "Y : %5.3f",
                "Z : %5.3f",
                "X : %5.3f Y : %5.3f",
                "Y : %5.3f Z : %5.3f",
                "X : %5.3f Z : %5.3f",
                "X : %5.3f Y : %5.3f Z : %5.3f"
            };

            static const char *scaleInfoMask[] =
            {
                "X : %5.2f",
                "Y : %5.2f",
                "Z : %5.2f",
                "XYZ : %5.2f"
            };

            static const char *rotationInfoMask[] =
            {
                "X : %5.2f deg %5.2f rad",
                "Y : %5.2f deg %5.2f rad",
                "Z : %5.2f deg %5.2f rad",
                "Screen : %5.2f deg %5.2f rad"
            };

            static const int translationInfoIndex[] =
            {
                0,0,0,
                1,0,0,
                2,0,0,
                0,1,0,
                1,2,0,
                0,2,1,
                0,1,2
            };

            static const float quadMin = 0.5f;
            static const float quadMax = 0.8f;
            static const float quadUV[8] =
            {
                quadMin, quadMin,
                quadMin, quadMax,
                quadMax, quadMax,
                quadMax, quadMin
            };

            static const int halfCircleSegmentCount = 64;
            static const float snapTension = 0.5f;

            enum MOVETYPE
            {
                NONE,
                MOVE_X,
                MOVE_Y,
                MOVE_Z,
                MOVE_XY,
                MOVE_XZ,
                MOVE_YZ,
                MOVE_SCREEN,
                ROTATE_X,
                ROTATE_Y,
                ROTATE_Z,
                ROTATE_SCREEN,
                SCALE_X,
                SCALE_Y,
                SCALE_Z,
                SCALE_XYZ,
            };

            struct Context
            {
                Context()
                    : mbUsing(false)
                    , mbEnable(true)
                    , mbUsingBounds(false)
                {
                }

                ImDrawList* mDrawList;

                Alignment mMode;
                matrix_t mViewMat;
                matrix_t mProjectionMat;
                matrix_t mModel;
                matrix_t mModelInverse;
                matrix_t mModelSource;
                matrix_t mModelSourceInverse;
                matrix_t mMVP;
                matrix_t mViewProjection;

                vec_t mModelScaleOrigin;
                vec_t mCameraEye;
                vec_t mCameraRight;
                vec_t mCameraDir;
                vec_t mCameraUp;
                vec_t mRayOrigin;
                vec_t mRayVector;

                ImVec2 mScreenSquareCenter;
                ImVec2 mScreenSquareMin;
                ImVec2 mScreenSquareMax;

                float mScreenFactor;
                vec_t mRelativeOrigin;

                bool mbUsing;
                bool mbEnable;

                // translation
                vec_t mTranslationPlane;
                vec_t mTranslationPlaneOrigin;
                vec_t mMatrixOrigin;

                // rotation
                vec_t mRotationVectorSource;
                float mRotationAngle;
                float mRotationAngleOrigin;
                //vec_t mWorldToLocalAxis;

                // scale
                vec_t mScale;
                vec_t mScaleValueOrigin;
                float mSaveMousePosx;

                // save axis factor when using gizmo
                bool mBelowAxisLimit[3];
                bool mBelowPlaneLimit[3];
                float mAxisFactor[3];

                // bounds stretching
                vec_t mBoundsPivot;
                vec_t mBoundsAnchor;
                vec_t mBoundsPlane;
                vec_t mBoundsLocalPivot;
                int mBoundsBestAxis;
                int mBoundsAxis[2];
                bool mbUsingBounds;
                matrix_t mBoundsMatrix;

                //
                int mCurrentOperation;

                float mX = 0.0f;
                float mY = 0.0f;
                float mWidth = 0.0f;
                float mHeight = 0.0f;

                ImVec2 worldToPos(const vec_t& worldPos, const matrix_t& mat)
                {
                    ImGuiIO& io = ImGui::GetIO();

                    vec_t trans;
                    trans.TransformPoint(worldPos, mat);
                    trans *= 0.5f / trans.w;
                    trans += makeVect(0.5f, 0.5f);
                    trans.y = 1.0f - trans.y;
                    trans.x *= mWidth;
                    trans.y *= mHeight;
                    trans.x += mX;
                    trans.y += mY;
                    return ImVec2(trans.x, trans.y);
                }

                void ComputeCameraRay(vec_t &rayOrigin, vec_t &rayDir)
                {
                    ImGuiIO& io = ImGui::GetIO();

                    matrix_t mViewProjInverse;
                    mViewProjInverse.Inverse(mViewMat * mProjectionMat);

                    float mox = ((io.MousePos.x - mX) / mWidth) * 2.0f - 1.0f;
                    float moy = (1.0f - ((io.MousePos.y - mY) / mHeight)) * 2.0f - 1.0f;

                    rayOrigin.Transform(makeVect(mox, moy, 0.0f, 1.0f), mViewProjInverse);
                    rayOrigin *= 1.0f / rayOrigin.w;
                    vec_t rayEnd;
                    rayEnd.Transform(makeVect(mox, moy, 1.0f, 1.0f), mViewProjInverse);
                    rayEnd *= 1.0f / rayEnd.w;
                    rayDir = Normalized(rayEnd - rayOrigin);
                }

                float IntersectRayPlane(const vec_t & rOrigin, const vec_t& rVector, const vec_t& plane)
                {
                    float numer = plane.Dot3(rOrigin) - plane.w;
                    float denom = plane.Dot3(rVector);

                    if (std::abs(denom) < FLT_EPSILON)  // normal is orthogonal to vector, cant intersect
                    {
                        return -1.0f;
                    }

                    return -(numer / denom);
                }

                void SetRect(float x, float y, float width, float height)
                {
                    mX = x;
                    mY = y;
                    mWidth = width;
                    mHeight = height;
                }

                void BeginFrame()
                {
                    mDrawList = ImGui::GetWindowDrawList();
                    if (!mDrawList)
                    {
                        ImGuiIO& io = ImGui::GetIO();
                        ImGui::Begin("gizmo", NULL, io.DisplaySize, 0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus);
                        mDrawList = ImGui::GetWindowDrawList();
                        ImGui::End();
                    }
                }

                bool IsUsing()
                {
                    return mbUsing || mbUsingBounds;
                }

                bool IsOver()
                {
                    return (GetMoveType(NULL) != NONE) || GetRotateType() != NONE || GetScaleType() != NONE || IsUsing();
                }

                void Enable(bool enable)
                {
                    mbEnable = enable;
                    if (!enable)
                    {
                        mbUsing = false;
                        mbUsingBounds = false;
                    }
                }

                float GetUniform(const vec_t& position, const matrix_t& mat)
                {
                    vec_t trf = makeVect(position.x, position.y, position.z, 1.0f);
                    trf.Transform(mat);
                    return trf.w;
                }

                void ComputeContext(const float *view, const float *projection, float *matrix, Alignment mode)
                {
                    mMode = mode;
                    mViewMat = *(matrix_t*)view;
                    mProjectionMat = *(matrix_t*)projection;

                    if (mode == Alignment::Local)
                    {
                        mModel = *(matrix_t*)matrix;
                        mModel.OrthoNormalize();
                    }
                    else
                    {
                        mModel.Translation(((matrix_t*)matrix)->v.position);
                    }

                    mModelSource = *(matrix_t*)matrix;
                    mModelScaleOrigin.Set(mModelSource.v.right.Length(), mModelSource.v.up.Length(), mModelSource.v.dir.Length());

                    mModelInverse.Inverse(mModel);
                    mModelSourceInverse.Inverse(mModelSource);
                    mViewProjection = mViewMat * mProjectionMat;
                    mMVP = mModel * mViewProjection;

                    matrix_t viewInverse;
                    viewInverse.Inverse(mViewMat);
                    mCameraDir = viewInverse.v.dir;
                    mCameraEye = viewInverse.v.position;
                    mCameraRight = viewInverse.v.right;
                    mCameraUp = viewInverse.v.up;
                    mScreenFactor = 0.15f * GetUniform(mModel.v.position, mViewProjection);

                    ImVec2 centerSSpace = worldToPos(makeVect(0.0f, 0.0f), mMVP);
                    mScreenSquareCenter = centerSSpace;
                    mScreenSquareMin = ImVec2(centerSSpace.x - 10.0f, centerSSpace.y - 10.0f);
                    mScreenSquareMax = ImVec2(centerSSpace.x + 10.0f, centerSSpace.y + 10.0f);

                    ComputeCameraRay(mRayOrigin, mRayVector);
                }

                void ComputeColors(ImU32 *colors, int type, Operation operation)
                {
                    if (mbEnable)
                    {
                        switch (operation)
                        {
                        case Operation::Translate:
                            colors[0] = (type == MOVE_SCREEN) ? selectionColor : 0xFFFFFFFF;
                            for (int i = 0; i < 3; i++)
                            {
                                int colorPlaneIndex = (i + 2) % 3;
                                colors[i + 1] = (type == (int)(MOVE_X + i)) ? selectionColor : directionColor[i];
                                colors[i + 4] = (type == (int)(MOVE_XY + i)) ? selectionColor : directionColor[colorPlaneIndex];
                            }

                            break;

                        case Operation::Rotate:
                            colors[0] = (type == ROTATE_SCREEN) ? selectionColor : 0xFFFFFFFF;
                            for (int i = 0; i < 3; i++)
                            {
                                colors[i + 1] = (type == (int)(ROTATE_X + i)) ? selectionColor : directionColor[i];
                            }

                            break;

                        case Operation::Scale:
                            colors[0] = (type == SCALE_XYZ) ? selectionColor : 0xFFFFFFFF;
                            for (int i = 0; i < 3; i++)
                            {
                                colors[i + 1] = (type == (int)(SCALE_X + i)) ? selectionColor : directionColor[i];
                            }

                            break;

                        case Operation::Bounds:
                            break;
                        };
                    }
                    else
                    {
                        for (int i = 0; i < 7; i++)
                        {
                            colors[i] = inactiveColor;
                        }
                    }
                }

                void ComputeTripodAxisAndVisibility(int axisIndex, vec_t& dirPlaneX, vec_t& dirPlaneY, bool& belowAxisLimit, bool& belowPlaneLimit)
                {
                    const int planeNormal = (axisIndex + 2) % 3;
                    dirPlaneX = directionUnary[axisIndex];
                    dirPlaneY = directionUnary[(axisIndex + 1) % 3];
                    if (mbUsing)
                    {
                        // when using, use stored factors so the gizmo doesn't flip when we translate
                        belowAxisLimit = mBelowAxisLimit[axisIndex];
                        belowPlaneLimit = mBelowPlaneLimit[axisIndex];

                        dirPlaneX *= mAxisFactor[axisIndex];
                        dirPlaneY *= mAxisFactor[(axisIndex + 1) % 3];
                    }
                    else
                    {
                        vec_t dirPlaneNormalWorld;
                        dirPlaneNormalWorld.TransformVector(directionUnary[planeNormal], mModel);
                        dirPlaneNormalWorld.Normalize();

                        vec_t dirPlaneXWorld(dirPlaneX);
                        dirPlaneXWorld.TransformVector(mModel);
                        dirPlaneXWorld.Normalize();

                        vec_t dirPlaneYWorld(dirPlaneY);
                        dirPlaneYWorld.TransformVector(mModel);
                        dirPlaneYWorld.Normalize();

                        vec_t cameraEyeToGizmo = Normalized(mModel.v.position - mCameraEye);
                        float dotCameraDirX = cameraEyeToGizmo.Dot3(dirPlaneXWorld);
                        float dotCameraDirY = cameraEyeToGizmo.Dot3(dirPlaneYWorld);

                        // compute factor values
                        float mulAxisX = (dotCameraDirX > 0.0f) ? -1.0f : 1.0f;
                        float mulAxisY = (dotCameraDirY > 0.0f) ? -1.0f : 1.0f;
                        dirPlaneX *= mulAxisX;
                        dirPlaneY *= mulAxisY;

                        belowAxisLimit = std::abs(dotCameraDirX) < angleLimit;
                        belowPlaneLimit = (std::abs(cameraEyeToGizmo.Dot3(dirPlaneNormalWorld)) > planeLimit);

                        // and store values
                        mAxisFactor[axisIndex] = mulAxisX;
                        mAxisFactor[(axisIndex + 1) % 3] = mulAxisY;
                        mBelowAxisLimit[axisIndex] = belowAxisLimit;
                        mBelowPlaneLimit[axisIndex] = belowPlaneLimit;
                    }
                }

                void ComputeSnap(float*value, float snap)
                {
                    if (snap <= FLT_EPSILON)
                    {
                        return;
                    }

                    float modulo = std::fmod(*value, snap);
                    float moduloRatio = std::abs(modulo) / snap;
                    if (moduloRatio < snapTension)
                    {
                        *value -= modulo;
                    }
                    else if (moduloRatio > (1.0f - snapTension))
                    {
                        *value = *value - modulo + snap * ((*value < 0.0f) ? -1.0f : 1.0f);
                    }
                }
                void ComputeSnap(vec_t& value, float *snap)
                {
                    for (int i = 0; i < 3; i++)
                    {
                        ComputeSnap(&value[i], snap[i]);
                    }
                }

                float ComputeAngleOnPlane()
                {
                    const float len = IntersectRayPlane(mRayOrigin, mRayVector, mTranslationPlane);
                    vec_t localPos = Normalized(mRayOrigin + mRayVector * len - mModel.v.position);

                    vec_t perpendicularVector;
                    perpendicularVector.Cross(mRotationVectorSource, mTranslationPlane);
                    perpendicularVector.Normalize();
                    float acosAngle = std::clamp(Dot(localPos, mRotationVectorSource), -0.9999f, 0.9999f);
                    float angle = std::acos(acosAngle);
                    angle *= (Dot(localPos, perpendicularVector) < 0.0f) ? 1.0f : -1.0f;
                    return angle;
                }

                void DrawRotationGizmo(int type)
                {
                    ImDrawList* drawList = mDrawList;
                    ImGuiIO& io = ImGui::GetIO();

                    // colors
                    ImU32 colors[7];
                    ComputeColors(colors, type, Operation::Rotate);

                    vec_t cameraToModelNormalized = Normalized(mModel.v.position - mCameraEye);
                    cameraToModelNormalized.TransformVector(mModelInverse);

                    for (int axis = 0; axis < 3; axis++)
                    {
                        ImVec2 circlePos[halfCircleSegmentCount];
                        float angleStart = std::atan2(cameraToModelNormalized[(4 - axis) % 3], cameraToModelNormalized[(3 - axis) % 3]) + Math::Pi * 0.5f;
                        for (unsigned int i = 0; i < halfCircleSegmentCount; i++)
                        {
                            float ng = angleStart + Math::Pi * ((float)i / (float)halfCircleSegmentCount);
                            vec_t axisPos = makeVect(std::cos(ng), std::sin(ng), 0.0f);
                            vec_t pos = makeVect(axisPos[axis], axisPos[(axis + 1) % 3], axisPos[(axis + 2) % 3]) * mScreenFactor;
                            circlePos[i] = worldToPos(pos, mMVP);
                        }

                        drawList->AddPolyline(circlePos, halfCircleSegmentCount, colors[3 - axis], false, 5, true);
                    }

                    drawList->AddCircle(worldToPos(mModel.v.position, mViewProjection), screenRotateSize * mHeight, colors[0], 64, 5);
                    if (mbUsing)
                    {
                        ImVec2 circlePos[halfCircleSegmentCount + 1];

                        circlePos[0] = worldToPos(mModel.v.position, mViewProjection);
                        for (unsigned int i = 1; i < halfCircleSegmentCount; i++)
                        {
                            float ng = mRotationAngle * ((float)(i - 1) / (float)(halfCircleSegmentCount - 1));

                            matrix_t rotateVectorMatrix;
                            rotateVectorMatrix.RotationAxis(mTranslationPlane, ng);

                            vec_t pos;
                            pos.TransformPoint(mRotationVectorSource, rotateVectorMatrix);
                            pos *= mScreenFactor;
                            circlePos[i] = worldToPos(pos + mModel.v.position, mViewProjection);
                        }

                        drawList->AddConvexPolyFilled(circlePos, halfCircleSegmentCount, 0x801080FF, true);
                        drawList->AddPolyline(circlePos, halfCircleSegmentCount, 0xFF1080FF, true, 2, true);

                        ImVec2 destinationPosOnScreen = circlePos[1];
                        char tmps[512];
                        ImFormatString(tmps, sizeof(tmps), rotationInfoMask[type - ROTATE_X], (mRotationAngle / Math::Pi)*180.0f, mRotationAngle);
                        drawList->AddText(ImVec2(destinationPosOnScreen.x + 15, destinationPosOnScreen.y + 15), 0xFF000000, tmps);
                        drawList->AddText(ImVec2(destinationPosOnScreen.x + 14, destinationPosOnScreen.y + 14), 0xFFFFFFFF, tmps);
                    }
                }

                void DrawHatchedAxis(const vec_t& axis)
                {
                    for (int j = 1; j < 10; j++)
                    {
                        ImVec2 baseSSpace2 = worldToPos(axis * 0.05f * (float)(j * 2) * mScreenFactor, mMVP);
                        ImVec2 worldDirSSpace2 = worldToPos(axis * 0.05f * (float)(j * 2 + 1) * mScreenFactor, mMVP);
                        mDrawList->AddLine(baseSSpace2, worldDirSSpace2, 0x80000000, 6.0f);
                    }
                }

                void DrawScaleGizmo(int type)
                {
                    ImDrawList* drawList = mDrawList;

                    // colors
                    ImU32 colors[7];
                    ComputeColors(colors, type, Operation::Scale);

                    // draw screen cirle
                    drawList->AddCircleFilled(mScreenSquareCenter, 12.0f, colors[0], 32);

                    // draw
                    vec_t scaleDisplay = { 1.0f, 1.0f, 1.0f, 1.0f };
                    if (mbUsing)
                    {
                        scaleDisplay = mScale;
                    }

                    for (unsigned int i = 0; i < 3; i++)
                    {
                        vec_t dirPlaneX, dirPlaneY;
                        bool belowAxisLimit, belowPlaneLimit;
                        ComputeTripodAxisAndVisibility(i, dirPlaneX, dirPlaneY, belowAxisLimit, belowPlaneLimit);

                        // draw axis
                        if (belowAxisLimit)
                        {
                            ImVec2 baseSSpace = worldToPos(dirPlaneX * 0.1f * mScreenFactor, mMVP);
                            ImVec2 worldDirSSpaceNoScale = worldToPos(dirPlaneX * mScreenFactor, mMVP);
                            ImVec2 worldDirSSpace = worldToPos((dirPlaneX * scaleDisplay[i]) * mScreenFactor, mMVP);

                            if (mbUsing)
                            {
                                drawList->AddLine(baseSSpace, worldDirSSpaceNoScale, 0xFF404040, 6.0f);
                                drawList->AddCircleFilled(worldDirSSpaceNoScale, 10.0f, 0xFF404040);
                            }

                            drawList->AddLine(baseSSpace, worldDirSSpace, colors[i + 1], 6.0f);
                            drawList->AddCircleFilled(worldDirSSpace, 10.0f, colors[i + 1]);

                            if (mAxisFactor[i] < 0.0f)
                            {
                                DrawHatchedAxis(dirPlaneX * scaleDisplay[i]);
                            }
                        }
                    }

                    if (mbUsing)
                    {
                        ImVec2 destinationPosOnScreen = worldToPos(mModel.v.position, mViewProjection);

                        char tmps[512];
                        int componentInfoIndex = (type - SCALE_X) * 3;
                        ImFormatString(tmps, sizeof(tmps), scaleInfoMask[type - SCALE_X], scaleDisplay[translationInfoIndex[componentInfoIndex]]);
                        drawList->AddText(ImVec2(destinationPosOnScreen.x + 15, destinationPosOnScreen.y + 15), 0xFF000000, tmps);
                        drawList->AddText(ImVec2(destinationPosOnScreen.x + 14, destinationPosOnScreen.y + 14), 0xFFFFFFFF, tmps);
                    }
                }

                void DrawTranslationGizmo(int type)
                {
                    ImDrawList* drawList = mDrawList;
                    if (!drawList)
                    {
                        return;
                    }

                    // colors
                    ImU32 colors[7];
                    ComputeColors(colors, type, Operation::Translate);

                    // draw screen quad
                    drawList->AddRectFilled(mScreenSquareMin, mScreenSquareMax, colors[0], 2.0f);

                    // draw
                    for (unsigned int i = 0; i < 3; i++)
                    {
                        vec_t dirPlaneX, dirPlaneY;
                        bool belowAxisLimit, belowPlaneLimit;
                        ComputeTripodAxisAndVisibility(i, dirPlaneX, dirPlaneY, belowAxisLimit, belowPlaneLimit);

                        // draw axis
                        if (belowAxisLimit)
                        {
                            ImVec2 baseSSpace = worldToPos(dirPlaneX * 0.1f * mScreenFactor, mMVP);
                            ImVec2 worldDirSSpace = worldToPos(dirPlaneX * mScreenFactor, mMVP);
                            drawList->AddLine(baseSSpace, worldDirSSpace, colors[i + 1], 6.0f);
                            if (mAxisFactor[i] < 0.0f)
                            {
                                DrawHatchedAxis(dirPlaneX);
                            }
                        }

                        // draw plane
                        if (belowPlaneLimit)
                        {
                            ImVec2 screenQuadPts[4];
                            for (int j = 0; j < 4; j++)
                            {
                                vec_t cornerWorldPos = (dirPlaneX * quadUV[j * 2] + dirPlaneY  * quadUV[j * 2 + 1]) * mScreenFactor;
                                screenQuadPts[j] = worldToPos(cornerWorldPos, mMVP);
                            }

                            drawList->AddConvexPolyFilled(screenQuadPts, 4, colors[i + 4], true);
                        }
                    }

                    if (mbUsing)
                    {
                        ImVec2 sourcePosOnScreen = worldToPos(mMatrixOrigin, mViewProjection);
                        ImVec2 destinationPosOnScreen = worldToPos(mModel.v.position, mViewProjection);
                        vec_t dif = { destinationPosOnScreen.x - sourcePosOnScreen.x, destinationPosOnScreen.y - sourcePosOnScreen.y, 0.0f, 0.0f };
                        dif.Normalize();
                        dif *= 5.0f;
                        drawList->AddCircle(sourcePosOnScreen, 6.0f, translationLineColor);
                        drawList->AddCircle(destinationPosOnScreen, 6.0f, translationLineColor);
                        drawList->AddLine(ImVec2(sourcePosOnScreen.x + dif.x, sourcePosOnScreen.y + dif.y), ImVec2(destinationPosOnScreen.x - dif.x, destinationPosOnScreen.y - dif.y), translationLineColor, 2.0f);

                        char tmps[512];
                        vec_t deltaInfo = mModel.v.position - mMatrixOrigin;
                        int componentInfoIndex = (type - MOVE_X) * 3;
                        ImFormatString(tmps, sizeof(tmps), translationInfoMask[type - MOVE_X], deltaInfo[translationInfoIndex[componentInfoIndex]], deltaInfo[translationInfoIndex[componentInfoIndex + 1]], deltaInfo[translationInfoIndex[componentInfoIndex + 2]]);
                        drawList->AddText(ImVec2(destinationPosOnScreen.x + 15, destinationPosOnScreen.y + 15), 0xFF000000, tmps);
                        drawList->AddText(ImVec2(destinationPosOnScreen.x + 14, destinationPosOnScreen.y + 14), 0xFFFFFFFF, tmps);
                    }
                }

                bool isZero(float v)
                {
                    return (v > -0.000001f && v < 0.000001f);
                }

                bool isPointInside(int x, int y)
                {
                    return (x >= mX && x <= (mX + mWidth) && y >= mY && y <= (mY + mHeight));
                }

                bool isClipped(float num, float denom)
                {
                    float t;
                    if (isZero(denom))
                    {
                        return num < 0.0;
                    }

                    t = num / denom;
                    if (denom > 0)
                    {
                        if (t > 1.0f)
                        {
                            return false;
                        }
                    }
                    else
                    {
                        if (t < 0.0f)
                        {
                            return false;
                        }
                    }

                    return true;
                }

                bool isInside(int x1, int y1, int x2, int y2)
                {
                    float dx = x2 - x1;
                    float dy = y2 - y1;
                    if (isZero(dx) && isZero(dy) && isPointInside(x1, y1))
                    {
                        return true;
                    }

                    if (isClipped(mX - x1, dx) &&
                        isClipped(x1 - (mX + mWidth), -dx) &&
                        isClipped(mY - y1, dy) &&
                        isClipped(y1 - (mY + mHeight), -dy))
                    {
                        return true;
                    }

                    return false;
                }

                void HandleBounds(float *bounds, matrix_t *matrix, float *snapValues)
                {
                    ImGuiIO& io = ImGui::GetIO();
                    ImDrawList* drawList = mDrawList;

                    // compute best projection axis
                    vec_t bestAxisWorldDirection;
                    int bestAxis = mBoundsBestAxis;
                    if (!mbUsingBounds)
                    {

                        float bestDot = 0.0f;
                        for (unsigned int i = 0; i < 3; i++)
                        {
                            vec_t dirPlaneNormalWorld;
                            dirPlaneNormalWorld.TransformVector(directionUnary[i], mModelSource);
                            dirPlaneNormalWorld.Normalize();

                            float dt = Dot(Normalized(mCameraEye - mModelSource.v.position), dirPlaneNormalWorld);
                            if (std::abs(dt) >= bestDot)
                            {
                                bestDot = std::abs(dt);
                                bestAxis = i;
                                bestAxisWorldDirection = dirPlaneNormalWorld;
                            }
                        }
                    }

                    // corners
                    vec_t aabb[4];

                    int secondAxis = (bestAxis + 1) % 3;
                    int thirdAxis = (bestAxis + 2) % 3;

                    for (int i = 0; i < 4; i++)
                    {
                        aabb[i][3] = aabb[i][bestAxis] = 0.0f;
                        aabb[i][secondAxis] = bounds[secondAxis + 3 * (i >> 1)];
                        aabb[i][thirdAxis] = bounds[thirdAxis + 3 * ((i >> 1) ^ (i & 1))];
                    }

                    // draw bounds
                    unsigned int anchorAlpha = mbEnable ? 0xFF000000 : 0x80000000;

                    matrix_t boundsMVP = mModelSource * mViewProjection;
                    for (int i = 0; i < 4; i++)
                    {
                        ImVec2 worldBound1 = worldToPos(aabb[i], boundsMVP);
                        ImVec2 worldBound2 = worldToPos(aabb[(i + 1) % 4], boundsMVP);
                        float boundDistance = std::sqrt(ImLengthSqr(worldBound1 - worldBound2));
                        int stepCount = (int)(boundDistance / 10.0f);
                        float stepLength = 1.0f / (float)stepCount;
                        for (int j = 0; j < stepCount; j++)
                        {
                            float t1 = (float)j * stepLength;
                            float t2 = (float)j * stepLength + stepLength * 0.5f;
                            ImVec2 worldBoundSS1 = ImLerp(worldBound1, worldBound2, ImVec2(t1, t1));
                            ImVec2 worldBoundSS2 = ImLerp(worldBound1, worldBound2, ImVec2(t2, t2));
                            if (isInside(worldBoundSS1.x, worldBoundSS1.y, worldBoundSS2.x, worldBoundSS2.y))
                            {
                                drawList->AddLine(worldBoundSS1, worldBoundSS2, 0xAAAAAA + anchorAlpha, 3.0f);
                            }
                        }

                        vec_t midPoint = (aabb[i] + aabb[(i + 1) % 4]) * 0.5f;
                        ImVec2 midBound = worldToPos(midPoint, boundsMVP);
                        static const float AnchorBigRadius = 8.0f;
                        static const float AnchorSmallRadius = 6.0f;
                        bool overBigAnchor = ImLengthSqr(worldBound1 - io.MousePos) <= (AnchorBigRadius*AnchorBigRadius);
                        bool overSmallAnchor = ImLengthSqr(midBound - io.MousePos) <= (AnchorBigRadius*AnchorBigRadius);

                        unsigned int bigAnchorColor = overBigAnchor ? selectionColor : (0xAAAAAA + anchorAlpha);
                        unsigned int smallAnchorColor = overSmallAnchor ? selectionColor : (0xAAAAAA + anchorAlpha);

                        drawList->AddCircleFilled(worldBound1, AnchorBigRadius, bigAnchorColor);
                        drawList->AddCircleFilled(midBound, AnchorSmallRadius, smallAnchorColor);
                        int oppositeIndex = (i + 2) % 4;
                        // big anchor on corners
                        if (!mbUsingBounds && mbEnable && overBigAnchor && io.MouseDown[0])
                        {
                            mBoundsPivot.TransformPoint(aabb[(i + 2) % 4], mModelSource);
                            mBoundsAnchor.TransformPoint(aabb[i], mModelSource);
                            mBoundsPlane = BuildPlane(mBoundsAnchor, bestAxisWorldDirection);
                            mBoundsBestAxis = bestAxis;
                            mBoundsAxis[0] = secondAxis;
                            mBoundsAxis[1] = thirdAxis;

                            mBoundsLocalPivot.Set(0.0f);
                            mBoundsLocalPivot[secondAxis] = aabb[oppositeIndex][secondAxis];
                            mBoundsLocalPivot[thirdAxis] = aabb[oppositeIndex][thirdAxis];

                            mbUsingBounds = true;
                            mBoundsMatrix = mModelSource;
                        }

                        // small anchor on middle of segment
                        if (!mbUsingBounds && mbEnable && overSmallAnchor && io.MouseDown[0])
                        {
                            vec_t midPointOpposite = (aabb[(i + 2) % 4] + aabb[(i + 3) % 4]) * 0.5f;
                            mBoundsPivot.TransformPoint(midPointOpposite, mModelSource);
                            mBoundsAnchor.TransformPoint(midPoint, mModelSource);
                            mBoundsPlane = BuildPlane(mBoundsAnchor, bestAxisWorldDirection);
                            mBoundsBestAxis = bestAxis;
                            int indices[] = { secondAxis , thirdAxis };
                            mBoundsAxis[0] = indices[i % 2];
                            mBoundsAxis[1] = -1;

                            mBoundsLocalPivot.Set(0.0f);
                            mBoundsLocalPivot[mBoundsAxis[0]] = aabb[oppositeIndex][indices[i % 2]];// bounds[mBoundsAxis[0]] * (((i + 1) & 2) ? 1.0f : -1.0f);

                            mbUsingBounds = true;
                            mBoundsMatrix = mModelSource;
                        }
                    }

                    if (mbUsingBounds)
                    {
                        matrix_t scale;
                        scale.SetToIdentity();

                        // compute projected mouse position on plane
                        const float len = IntersectRayPlane(mRayOrigin, mRayVector, mBoundsPlane);
                        vec_t newPos = mRayOrigin + mRayVector * len;

                        // compute a reference and delta vectors base on mouse move
                        vec_t deltaVector = (newPos - mBoundsPivot).Abs();
                        vec_t referenceVector = (mBoundsAnchor - mBoundsPivot).Abs();

                        // for 1 or 2 axes, compute a ratio that's used for scale and snap it based on resulting length
                        for (int i = 0; i < 2; i++)
                        {
                            int axisIndex = mBoundsAxis[i];
                            if (axisIndex == -1)
                            {
                                continue;
                            }

                            float ratioAxis = 1.0f;
                            vec_t axisDir = mBoundsMatrix.component[axisIndex].Abs();

                            float dtAxis = axisDir.Dot(referenceVector);
                            float boundSize = bounds[axisIndex + 3] - bounds[axisIndex];
                            if (dtAxis > FLT_EPSILON)
                            {
                                ratioAxis = axisDir.Dot(deltaVector) / dtAxis;
                            }

                            if (snapValues)
                            {
                                float length = boundSize * ratioAxis;
                                ComputeSnap(&length, snapValues[axisIndex]);
                                if (boundSize > FLT_EPSILON)
                                {
                                    ratioAxis = length / boundSize;
                                }
                            }

                            scale.component[axisIndex] *= ratioAxis;
                        }

                        // transform matrix
                        matrix_t preScale, postScale;
                        preScale.Translation(-mBoundsLocalPivot);
                        postScale.Translation(mBoundsLocalPivot);
                        matrix_t res = preScale * scale * postScale * mBoundsMatrix;
                        *matrix = res;

                        // info text
                        char tmps[512];
                        ImVec2 destinationPosOnScreen = worldToPos(mModel.v.position, mViewProjection);
                        ImFormatString(tmps, sizeof(tmps), "X: %.2f Y: %.2f Z:%.2f"
                            , (bounds[3] - bounds[0]) * mBoundsMatrix.component[0].Length() * scale.component[0].Length()
                            , (bounds[4] - bounds[1]) * mBoundsMatrix.component[1].Length() * scale.component[1].Length()
                            , (bounds[5] - bounds[2]) * mBoundsMatrix.component[2].Length() * scale.component[2].Length()
                        );

                        drawList->AddText(ImVec2(destinationPosOnScreen.x + 15, destinationPosOnScreen.y + 15), 0xFF000000, tmps);
                        drawList->AddText(ImVec2(destinationPosOnScreen.x + 14, destinationPosOnScreen.y + 14), 0xFFFFFFFF, tmps);
                    }

                    if (!io.MouseDown[0])
                    {
                        mbUsingBounds = false;
                    }
                }

                int GetScaleType()
                {
                    ImGuiIO& io = ImGui::GetIO();
                    int type = NONE;

                    // screen
                    if (io.MousePos.x >= mScreenSquareMin.x && io.MousePos.x <= mScreenSquareMax.x &&
                        io.MousePos.y >= mScreenSquareMin.y && io.MousePos.y <= mScreenSquareMax.y)
                    {
                        type = SCALE_XYZ;
                    }

                    const vec_t direction[3] = { mModel.v.right, mModel.v.up, mModel.v.dir };
                    // compute
                    for (unsigned int i = 0; i < 3 && type == NONE; i++)
                    {
                        vec_t dirPlaneX, dirPlaneY;
                        bool belowAxisLimit, belowPlaneLimit;
                        ComputeTripodAxisAndVisibility(i, dirPlaneX, dirPlaneY, belowAxisLimit, belowPlaneLimit);
                        dirPlaneX.TransformVector(mModel);
                        dirPlaneY.TransformVector(mModel);

                        const int planeNormal = (i + 2) % 3;

                        const float len = IntersectRayPlane(mRayOrigin, mRayVector, BuildPlane(mModel.v.position, direction[planeNormal]));
                        vec_t posOnPlane = mRayOrigin + mRayVector * len;

                        const float dx = dirPlaneX.Dot3((posOnPlane - mModel.v.position) * (1.0f / mScreenFactor));
                        const float dy = dirPlaneY.Dot3((posOnPlane - mModel.v.position) * (1.0f / mScreenFactor));
                        if (belowAxisLimit && dy > -0.1f && dy < 0.1f && dx > 0.1f  && dx < 1.0f)
                        {
                            type = SCALE_X + i;
                        }
                    }

                    return type;
                }

                int GetRotateType()
                {
                    ImGuiIO& io = ImGui::GetIO();
                    int type = NONE;

                    vec_t deltaScreen = { io.MousePos.x - mScreenSquareCenter.x, io.MousePos.y - mScreenSquareCenter.y, 0.0f, 0.0f };
                    float dist = deltaScreen.Length();
                    if (dist >= (screenRotateSize - 0.002f) * mHeight && dist < (screenRotateSize + 0.002f) * mHeight)
                    {
                        type = ROTATE_SCREEN;
                    }

                    const vec_t planeNormals[] = { mModel.v.right, mModel.v.up, mModel.v.dir };
                    for (unsigned int i = 0; i < 3 && type == NONE; i++)
                    {
                        // pickup plane
                        vec_t pickupPlane = BuildPlane(mModel.v.position, planeNormals[i]);

                        const float len = IntersectRayPlane(mRayOrigin, mRayVector, pickupPlane);
                        vec_t localPos = mRayOrigin + mRayVector * len - mModel.v.position;

                        if (Dot(Normalized(localPos), mRayVector) > FLT_EPSILON)
                        {
                            continue;
                        }

                        float distance = localPos.Length() / mScreenFactor;
                        if (distance > 0.9f && distance < 1.1f)
                        {
                            type = ROTATE_X + i;
                        }
                    }

                    return type;
                }

                int GetMoveType(vec_t *gizmoHitProportion)
                {
                    ImGuiIO& io = ImGui::GetIO();
                    int type = NONE;

                    // screen
                    if (io.MousePos.x >= mScreenSquareMin.x && io.MousePos.x <= mScreenSquareMax.x &&
                        io.MousePos.y >= mScreenSquareMin.y && io.MousePos.y <= mScreenSquareMax.y)
                    {
                        type = MOVE_SCREEN;
                    }

                    const vec_t direction[3] = { mModel.v.right, mModel.v.up, mModel.v.dir };

                    // compute
                    for (unsigned int i = 0; i < 3 && type == NONE; i++)
                    {
                        vec_t dirPlaneX, dirPlaneY;
                        bool belowAxisLimit, belowPlaneLimit;
                        ComputeTripodAxisAndVisibility(i, dirPlaneX, dirPlaneY, belowAxisLimit, belowPlaneLimit);
                        dirPlaneX.TransformVector(mModel);
                        dirPlaneY.TransformVector(mModel);

                        const int planeNormal = (i + 2) % 3;

                        const float len = IntersectRayPlane(mRayOrigin, mRayVector, BuildPlane(mModel.v.position, direction[planeNormal]));
                        vec_t posOnPlane = mRayOrigin + mRayVector * len;

                        const float dx = dirPlaneX.Dot3((posOnPlane - mModel.v.position) * (1.0f / mScreenFactor));
                        const float dy = dirPlaneY.Dot3((posOnPlane - mModel.v.position) * (1.0f / mScreenFactor));
                        if (belowAxisLimit && dy > -0.1f && dy < 0.1f && dx > 0.1f  && dx < 1.0f)
                        {
                            type = MOVE_X + i;
                        }

                        if (belowPlaneLimit && dx >= quadUV[0] && dx <= quadUV[4] && dy >= quadUV[1] && dy <= quadUV[3])
                        {
                            type = MOVE_XY + i;
                        }

                        if (gizmoHitProportion)
                        {
                            *gizmoHitProportion = makeVect(dx, dy, 0.0f);
                        }
                    }

                    return type;
                }

                void HandleTranslation(float *matrix, float *deltaMatrix, float *snap)
                {
                    ImGuiIO& io = ImGui::GetIO();
                    bool applyRotationLocaly = mMode == Alignment::Local;

                    // move
                    int type = NONE;
                    if (mbUsing)
                    {
                        const float len = IntersectRayPlane(mRayOrigin, mRayVector, mTranslationPlane);
                        vec_t newPos = mRayOrigin + mRayVector * len;

                        // compute delta
                        vec_t newOrigin = newPos - mRelativeOrigin * mScreenFactor;
                        vec_t delta = newOrigin - mModel.v.position;

                        // 1 axis constraint
                        if (mCurrentOperation >= MOVE_X && mCurrentOperation <= MOVE_Z)
                        {
                            int axisIndex = mCurrentOperation - MOVE_X;
                            const vec_t& axisValue = *(vec_t*)&mModel.m[axisIndex];
                            float lengthOnAxis = Dot(axisValue, delta);
                            delta = axisValue * lengthOnAxis;
                        }

                        // snap
                        if (snap)
                        {
                            vec_t cumulativeDelta = mModel.v.position + delta - mMatrixOrigin;
                            if (applyRotationLocaly)
                            {
                                matrix_t modelSourceNormalized = mModelSource;
                                modelSourceNormalized.OrthoNormalize();
                                matrix_t modelSourceNormalizedInverse;
                                modelSourceNormalizedInverse.Inverse(modelSourceNormalized);
                                cumulativeDelta.TransformVector(modelSourceNormalizedInverse);
                                ComputeSnap(cumulativeDelta, snap);
                                cumulativeDelta.TransformVector(modelSourceNormalized);
                            }
                            else
                            {
                                ComputeSnap(cumulativeDelta, snap);
                            }

                            delta = mMatrixOrigin + cumulativeDelta - mModel.v.position;
                        }

                        // compute matrix & delta
                        matrix_t deltaMatrixTranslation;
                        deltaMatrixTranslation.Translation(delta);
                        if (deltaMatrix)
                        {
                            memcpy(deltaMatrix, deltaMatrixTranslation.m16, sizeof(float) * 16);
                        }


                        matrix_t res = mModelSource * deltaMatrixTranslation;
                        *(matrix_t*)matrix = res;

                        if (!io.MouseDown[0])
                        {
                            mbUsing = false;
                        }

                        type = mCurrentOperation;
                    }
                    else
                    {
                        // find new possible way to move
                        vec_t gizmoHitProportion;
                        type = GetMoveType(&gizmoHitProportion);
                        if (io.MouseDown[0] && type != NONE)
                        {
                            mbUsing = true;
                            mCurrentOperation = type;
                            const vec_t movePlaneNormal[] = { mModel.v.up, mModel.v.dir, mModel.v.right, mModel.v.dir, mModel.v.right, mModel.v.up, -mCameraDir };
                            // pickup plane
                            mTranslationPlane = BuildPlane(mModel.v.position, movePlaneNormal[type - MOVE_X]);
                            const float len = IntersectRayPlane(mRayOrigin, mRayVector, mTranslationPlane);
                            mTranslationPlaneOrigin = mRayOrigin + mRayVector * len;
                            mMatrixOrigin = mModel.v.position;

                            mRelativeOrigin = (mTranslationPlaneOrigin - mModel.v.position) * (1.0f / mScreenFactor);
                        }
                    }

                    DrawTranslationGizmo(type);
                }

                void HandleScale(float *matrix, float *deltaMatrix, float *snap)
                {
                    ImGuiIO& io = ImGui::GetIO();

                    int type = NONE;
                    if (!mbUsing)
                    {
                        // find new possible way to scale
                        type = GetScaleType();
                        if (io.MouseDown[0] && type != NONE)
                        {
                            mbUsing = true;
                            mCurrentOperation = type;
                            const vec_t movePlaneNormal[] = { mModel.v.up, mModel.v.dir, mModel.v.right, mModel.v.dir, mModel.v.up, mModel.v.right, -mCameraDir };

                            // pickup plane
                            mTranslationPlane = BuildPlane(mModel.v.position, movePlaneNormal[type - SCALE_X]);
                            const float len = IntersectRayPlane(mRayOrigin, mRayVector, mTranslationPlane);
                            mTranslationPlaneOrigin = mRayOrigin + mRayVector * len;
                            mMatrixOrigin = mModel.v.position;
                            mScale.Set(1.0f, 1.0f, 1.0f);
                            mRelativeOrigin = (mTranslationPlaneOrigin - mModel.v.position) * (1.0f / mScreenFactor);
                            mScaleValueOrigin = makeVect(mModelSource.v.right.Length(), mModelSource.v.up.Length(), mModelSource.v.dir.Length());
                            mSaveMousePosx = io.MousePos.x;
                        }
                    }

                    // scale
                    if (mbUsing)
                    {
                        const float len = IntersectRayPlane(mRayOrigin, mRayVector, mTranslationPlane);
                        vec_t newPos = mRayOrigin + mRayVector * len;
                        vec_t newOrigin = newPos - mRelativeOrigin * mScreenFactor;
                        vec_t delta = newOrigin - mModel.v.position;

                        // 1 axis constraint
                        if (mCurrentOperation >= SCALE_X && mCurrentOperation <= SCALE_Z)
                        {
                            int axisIndex = mCurrentOperation - SCALE_X;
                            const vec_t& axisValue = *(vec_t*)&mModel.m[axisIndex];
                            float lengthOnAxis = Dot(axisValue, delta);
                            delta = axisValue * lengthOnAxis;

                            vec_t baseVector = mTranslationPlaneOrigin - mModel.v.position;
                            float ratio = Dot(axisValue, baseVector + delta) / Dot(axisValue, baseVector);

                            mScale[axisIndex] = std::max(ratio, 0.001f);
                        }
                        else
                        {
                            float scaleDelta = (io.MousePos.x - mSaveMousePosx)  * 0.01f;
                            mScale.Set(std::max(1.0f + scaleDelta, 0.001f));
                        }

                        // snap
                        if (snap)
                        {
                            float scaleSnap[] = { snap[0], snap[0], snap[0] };
                            ComputeSnap(mScale, scaleSnap);
                        }

                        // no 0 allowed
                        for (int i = 0; i < 3; i++)
                        {
                            mScale[i] = std::max(mScale[i], 0.001f);
                        }

                        // compute matrix & delta
                        matrix_t deltaMatrixScale;
                        deltaMatrixScale.Scale(mScale * mScaleValueOrigin);

                        matrix_t res = deltaMatrixScale * mModel;
                        *(matrix_t*)matrix = res;

                        if (deltaMatrix)
                        {
                            deltaMatrixScale.Scale(mScale);
                            memcpy(deltaMatrix, deltaMatrixScale.m16, sizeof(float) * 16);
                        }

                        if (!io.MouseDown[0])
                        {
                            mbUsing = false;
                        }

                        type = mCurrentOperation;
                    }

                    DrawScaleGizmo(type);
                }

                void HandleRotation(float *matrix, float *deltaMatrix, float *snap)
                {
                    ImGuiIO& io = ImGui::GetIO();
                    bool applyRotationLocaly = mMode == Alignment::Local;

                    int type = NONE;
                    if (!mbUsing)
                    {
                        type = GetRotateType();
                        if (type == ROTATE_SCREEN)
                        {
                            applyRotationLocaly = true;
                        }

                        if (io.MouseDown[0] && type != NONE)
                        {
                            mbUsing = true;
                            mCurrentOperation = type;
                            const vec_t rotatePlaneNormal[] = { mModel.v.right, mModel.v.up, mModel.v.dir, -mCameraDir };
                            // pickup plane
                            if (applyRotationLocaly)
                            {
                                mTranslationPlane = BuildPlane(mModel.v.position, rotatePlaneNormal[type - ROTATE_X]);
                            }
                            else
                            {
                                mTranslationPlane = BuildPlane(mModelSource.v.position, directionUnary[type - ROTATE_X]);
                            }

                            const float len = IntersectRayPlane(mRayOrigin, mRayVector, mTranslationPlane);
                            vec_t localPos = mRayOrigin + mRayVector * len - mModel.v.position;
                            mRotationVectorSource = Normalized(localPos);
                            mRotationAngleOrigin = ComputeAngleOnPlane();
                        }
                    }

                    // rotation
                    if (mbUsing)
                    {
                        mRotationAngle = ComputeAngleOnPlane();
                        if (snap)
                        {
                            float snapInRadian = Math::DegreesToRadians(snap[0]);
                            ComputeSnap(&mRotationAngle, snapInRadian);
                        }

                        vec_t rotationAxisLocalSpace;
                        rotationAxisLocalSpace.TransformVector(makeVect(mTranslationPlane.x, mTranslationPlane.y, mTranslationPlane.z, 0.0f), mModelInverse);
                        rotationAxisLocalSpace.Normalize();

                        matrix_t deltaRotation;
                        deltaRotation.RotationAxis(rotationAxisLocalSpace, mRotationAngle - mRotationAngleOrigin);
                        mRotationAngleOrigin = mRotationAngle;

                        matrix_t scaleOrigin;
                        scaleOrigin.Scale(mModelScaleOrigin);

                        if (applyRotationLocaly)
                        {
                            *(matrix_t*)matrix = scaleOrigin * deltaRotation * mModel;
                        }
                        else
                        {
                            matrix_t res = mModelSource;
                            res.v.position.Set(0.0f);

                            *(matrix_t*)matrix = res * deltaRotation;
                            ((matrix_t*)matrix)->v.position = mModelSource.v.position;
                        }

                        if (deltaMatrix)
                        {
                            *(matrix_t*)deltaMatrix = mModelInverse * deltaRotation * mModel;
                        }

                        if (!io.MouseDown[0])
                        {
                            mbUsing = false;
                        }

                        type = mCurrentOperation;
                    }

                    DrawRotationGizmo(type);
                }

                void DecomposeMatrixToComponents(const float *matrix, float *translation, float *rotation, float *scale)
                {
                    matrix_t mat = *(matrix_t*)matrix;

                    scale[0] = mat.v.right.Length();
                    scale[1] = mat.v.up.Length();
                    scale[2] = mat.v.dir.Length();

                    mat.OrthoNormalize();

                    rotation[0] = Math::RadiansToDegrees(std::atan2(mat.m[1][2], mat.m[2][2]));
                    rotation[1] = Math::RadiansToDegrees(std::atan2(-mat.m[0][2], std::sqrt(mat.m[1][2] * mat.m[1][2] + mat.m[2][2] * mat.m[2][2])));
                    rotation[2] = Math::RadiansToDegrees(std::atan2(mat.m[0][1], mat.m[0][0]));

                    translation[0] = mat.v.position.x;
                    translation[1] = mat.v.position.y;
                    translation[2] = mat.v.position.z;
                }

                void RecomposeMatrixFromComponents(const float *translation, const float *rotation, const float *scale, float *matrix)
                {
                    matrix_t& mat = *(matrix_t*)matrix;

                    matrix_t rot[3];
                    for (int i = 0; i < 3; i++)
                    {
                        rot[i].RotationAxis(directionUnary[i], Math::DegreesToRadians(rotation[i]));
                    }

                    mat = rot[0] * rot[1] * rot[2];

                    float validScale[3];
                    for (int i = 0; i < 3; i++)
                    {
                        if (std::abs(scale[i]) < FLT_EPSILON)
                        {
                            validScale[i] = 0.001f;
                        }
                        else
                        {
                            validScale[i] = scale[i];
                        }
                    }

                    mat.v.right *= validScale[0];
                    mat.v.up *= validScale[1];
                    mat.v.dir *= validScale[2];
                    mat.v.position.Set(translation[0], translation[1], translation[2], 1.0f);
                }

                void Manipulate(const float *view, const float *projection, Operation operation, Alignment mode, float *matrix, float *deltaMatrix, float *snap, float *localBounds, float *boundsSnap)
                {
                    ComputeContext(view, projection, matrix, mode);

                    // set delta to identity 
                    if (deltaMatrix)
                    {
                        ((matrix_t*)deltaMatrix)->SetToIdentity();
                    }

                    // behind camera
                    vec_t camSpacePosition;
                    camSpacePosition.TransformPoint(makeVect(0.0f, 0.0f, 0.0f), mMVP);
                    if (camSpacePosition.z < 0.001f)
                    {
                        return;
                    }

                    // -- 
                    if (mbEnable)
                    {
                        if (!mbUsingBounds)
                        {
                            switch (operation)
                            {
                            case Operation::Rotate:
                                HandleRotation(matrix, deltaMatrix, snap);
                                break;

                            case Operation::Translate:
                                HandleTranslation(matrix, deltaMatrix, snap);
                                break;

                            case Operation::Scale:
                                HandleScale(matrix, deltaMatrix, snap);
                                break;

                            case Operation::Bounds:
                                HandleBounds(localBounds, (matrix_t*)matrix, boundsSnap);
                                break;
                            };
                        }
                    }
                }

                void DrawCube(const float *view, const float *projection, float *matrix)
                {
                    matrix_t viewInverse;
                    viewInverse.Inverse(*(matrix_t*)view);
                    const matrix_t& model = *(matrix_t*)matrix;
                    matrix_t res = *(matrix_t*)matrix * *(matrix_t*)view * *(matrix_t*)projection;
                    for (int iFace = 0; iFace < 6; iFace++)
                    {
                        const int normalIndex = (iFace % 3);
                        const int perpXIndex = (normalIndex + 1) % 3;
                        const int perpYIndex = (normalIndex + 2) % 3;
                        const float invert = (iFace > 2) ? -1.0f : 1.0f;

                        const vec_t faceCoords[4] =
                        {
                            directionUnary[normalIndex] + directionUnary[perpXIndex] + directionUnary[perpYIndex],
                            directionUnary[normalIndex] + directionUnary[perpXIndex] - directionUnary[perpYIndex],
                            directionUnary[normalIndex] - directionUnary[perpXIndex] - directionUnary[perpYIndex],
                            directionUnary[normalIndex] - directionUnary[perpXIndex] + directionUnary[perpYIndex],
                        };

                        // clipping
                        bool skipFace = false;
                        for (unsigned int iCoord = 0; iCoord < 4; iCoord++)
                        {
                            vec_t camSpacePosition;
                            camSpacePosition.TransformPoint(faceCoords[iCoord] * 0.5f * invert, res);
                            if (camSpacePosition.z < 0.001f)
                            {
                                skipFace = true;
                                break;
                            }
                        }

                        if (skipFace)
                        {
                            continue;
                        }

                        // 3D->2D
                        ImVec2 faceCoordsScreen[4];
                        for (unsigned int iCoord = 0; iCoord < 4; iCoord++)
                        {
                            faceCoordsScreen[iCoord] = worldToPos(faceCoords[iCoord] * 0.5f * invert, res);
                        }

                        // back face culling 
                        vec_t cullPos, cullNormal;
                        cullPos.TransformPoint(faceCoords[0] * 0.5f * invert, model);
                        cullNormal.TransformVector(directionUnary[normalIndex] * invert, model);
                        float dt = Dot(Normalized(cullPos - viewInverse.v.position), Normalized(cullNormal));
                        if (dt > 0.0f)
                        {
                            continue;
                        }

                        // draw face with lighter color
                        mDrawList->AddConvexPolyFilled(faceCoordsScreen, 4, directionColor[normalIndex] | 0x808080, true);
                    }
                }
            };

            WorkSpace::WorkSpace(void)
                : context((Context *)ImGui::MemAlloc(sizeof(Context)))
            {
                IM_PLACEMENT_NEW(context) Context();
            }

            WorkSpace::~WorkSpace(void)
            {
                ImGui::MemFree(context);
            }

            void WorkSpace::beginFrame(float x, float y, float width, float height)
            {
                context->BeginFrame();
                context->SetRect(x, y, width, height);
            }

            void WorkSpace::manipulate(float const *view, float const *projection, Operation operation, Alignment alignment, float *matrix, float *deltaMatrix, float *snap, float *localBounds, float *boundsSnap)
            {
                context->Manipulate(view, projection, operation, alignment, matrix, deltaMatrix, snap, localBounds, boundsSnap);
            }
        }; // Gizmo
    }; // namespace UI
}; // namespace Gek