#pragma once

template <typename TYPE>
struct tmatrix3x2
{
public:
    union
    {
        struct { TYPE data[6]; };
        struct { TYPE matrix[3][2]; };

        struct
        {
            TYPE _11, _12;
            TYPE _21, _22;
            TYPE _31, _32;
        };

        struct
        {
            tvector2<TYPE> rx;
            tvector2<TYPE> ry;
            union
            {
                struct
                {
                    tvector2<TYPE> rz;
                };

                struct
                {
                    tvector2<TYPE> t;
                };
            };
        };
    };

public:
    tmatrix3x2(void)
    {
        SetIdentity();
    }

    tmatrix3x2(const tmatrix3x2<TYPE> &nMatrix)
    {
        _11 = nMatrix._11;    _12 = nMatrix._12;
        _21 = nMatrix._21;    _22 = nMatrix._22;
        _31 = nMatrix._31;    _32 = nMatrix._32;
    }

    tmatrix3x2(TYPE f11, TYPE f12,
               TYPE f21, TYPE f22,
               TYPE f31, TYPE f32)
    {
        _11 = f11;    _12 = f12;
        _21 = f21;    _22 = f22;
        _31 = f31;    _32 = f32;
    }

    void SetZero(void)
    {
        _11 = _12 = TYPE(0);
        _21 = _22 = TYPE(0);
        _31 = _32 = TYPE(0);
    }

    void SetIdentity(void)
    {
        _11 = _22 = TYPE(1);
        _12 = TYPE(0);
        _21 = TYPE(0);
        _31 = _32 = TYPE(0);
    }

    void SetScaling(TYPE nScale)
    {
        _11 = nScale;
        _22 = nScale;
    }

    void SetScaling(const tvector2<TYPE> &nScale)
    {
        _11 = nScale.x;
        _22 = nScale.y;
    }

    void SetTranslation(const tvector2<TYPE> &nTranslation)
    {
        _31 = nTranslation.x;
        _32 = nTranslation.y;
    }

    void SetRotation(float nAngle)
    {
        matrix[0][0] = cos(nAngle);
        matrix[1][0] = sin(nAngle);
        matrix[2][0] = TYPE(0);

        matrix[0][1] = -sin(nAngle);
        matrix[1][1] = cos(nAngle);
        matrix[2][1] = TYPE(0);
    }

    tvector2<TYPE> GetScaling(void) const
    {
        return tvector2<TYPE>(_11, _22);
    }

    void operator *= (const tmatrix3x2<TYPE> &nMatrix)
    {
        (*this) = ((*this) * nMatrix);
    }

    tmatrix3x2<TYPE> operator * (const tmatrix3x2<TYPE> &nMatrix) const
    {
        return tmatrix3x2<TYPE>(_11 * nMatrix._11 + _12 * nMatrix._21,
                                _11 * nMatrix._12 + _12 * nMatrix._22,
                                _21 * nMatrix._11 + _22 * nMatrix._21,
                                _21 * nMatrix._12 + _22 * nMatrix._22,
                                _31 * nMatrix._11 + _32 * nMatrix._21 + nMatrix._31,
                                _31 * nMatrix._12 + _32 * nMatrix._22 + nMatrix._32);
    }

    tmatrix3x2<TYPE> operator = (const tmatrix3x2<TYPE> &nMatrix)
    {
        _11 = nMatrix._11;    _12 = nMatrix._12;
        _21 = nMatrix._21;    _22 = nMatrix._22;
        _31 = nMatrix._31;    _32 = nMatrix._32;
        return *this;
    }
};
