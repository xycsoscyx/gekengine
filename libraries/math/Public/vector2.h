#pragma once

template <typename TYPE>
struct tvector3;

template <typename TYPE>
struct tvector4;

template <typename TYPE>
struct tvector2
{
public:
    union
    {
        struct { TYPE x, y; };
        struct { TYPE xy[2]; };
        struct { TYPE u, v; };
        struct { TYPE uv[2]; };
    };

public:
    tvector2(void)
    {
        x = y = TYPE(0);
    }

    tvector2(TYPE fValue)
    {
        x = y = fValue;
    }

    tvector2(const tvector2<TYPE> &nVector)
    {
        x = nVector.x;
        y = nVector.y;
    }

    tvector2(const tvector3<TYPE> &nVector)
    {
        x = nVector.x;
        y = nVector.y;
    }

    tvector2(const tvector4<TYPE> &nVector)
    {
        x = nVector.x;
        y = nVector.y;
    }

    tvector2(TYPE nX, TYPE nY)
    {
        x = nX;
        y = nY;
    }

    void Set(TYPE nX, TYPE nY)
    {
        x = nX;
        y = nY;
    }

    void Set(TYPE nX, TYPE nY, TYPE nZ)
    {
        x = nX;
        y = nY;
    }

    void Set(TYPE nX, TYPE nY, TYPE nZ, TYPE nW)
    {
        x = nX;
        y = nY;
    }

    void SetLength(TYPE nLength)
    {
        (*this) *= (nLength / GetLength());
    }

    TYPE GetLength(void) const
    {
        return sqrt((x * x) + (y * y));
    }

    TYPE GetLengthSqr(void) const
    {
        return ((x * x) + (y * y));
    }

    TYPE GetMax(void) const
    {
        return max(x, y);
    }

    tvector2 GetNormal(void) const
    {
        TYPE nLength = GetLength();
        if (nLength != TYPE(0))
        {
            return ((*this) * (TYPE(1) / GetLength()));
        }

        return (*this);
    }

    void Normalize(void)
    {
        (*this) = GetNormal();
    }

    TYPE Dot(const tvector2<TYPE> &nVector) const
    {
        return ((x * nVector.x) + (y * nVector.y));
    }

    TYPE Distance(const tvector2 &nVector) const
    {
        return (nVector - (*this)).GetLength();
    }

    tvector2 Lerp(const tvector2 &nVector, TYPE nFactor) const
    {
        return ((*this) + ((nVector - (*this)) * nFactor));
    }

    TYPE operator [] (int nIndex) const
    {
        return xy[nIndex];
    }

    TYPE &operator [] (int nIndex)
    {
        return xy[nIndex];
    }

    bool operator < (const tvector2 &nVector) const
    {
        if (x >= nVector.x) return false;
        if (y >= nVector.y) return false;
        return true;
    }

    bool operator > (const tvector2 &nVector) const
    {
        if (x <= nVector.x) return false;
        if (y <= nVector.y) return false;
        return true;
    }

    bool operator <= (const tvector2 &nVector) const
    {
        if (x > nVector.x) return false;
        if (y > nVector.y) return false;
        return true;
    }

    bool operator >= (const tvector2 &nVector) const
    {
        if (x < nVector.x) return false;
        if (y < nVector.y) return false;
        return true;
    }

    bool operator == (const tvector2 &nVector) const
    {
        if (x != nVector.x) return false;
        if (y != nVector.y) return false;
        return true;
    }

    bool operator != (const tvector2 &nVector) const
    {
        if (x != nVector.x) return true;
        if (y != nVector.y) return true;
        return false;
    }

    tvector2<TYPE> operator = (float nValue)
    {
        x = y = nValue;
        return (*this);
    }

    tvector2<TYPE> operator = (const tvector2<TYPE> &nVector)
    {
        x = nVector.x;
        y = nVector.y;
        return (*this);
    }

    tvector2<TYPE> operator = (const tvector3<TYPE> &nVector)
    {
        x = nVector.x;
        y = nVector.y;
        return (*this);
    }

    tvector2<TYPE> operator = (const tvector4<TYPE> &nVector)
    {
        x = nVector.x;
        y = nVector.y;
        return (*this);
    }

    void operator -= (const tvector2<TYPE> &nVector)
    {
        x -= nVector.x;
        y -= nVector.y;
    }

    void operator += (const tvector2<TYPE> &nVector)
    {
        x += nVector.x;
        y += nVector.y;
    }

    void operator /= (const tvector2<TYPE> &nVector)
    {
        x /= nVector.x;
        y /= nVector.y;
    }

    void operator *= (const tvector2<TYPE> &nVector)
    {
        x *= nVector.x;
        y *= nVector.y;
    }

    void operator -= (TYPE nScalar)
    {
        x -= nScalar;
        y -= nScalar;
    }

    void operator += (TYPE nScalar)
    {
        x += nScalar;
        y += nScalar;
    }

    void operator /= (TYPE nScalar)
    {
        x /= nScalar;
        y /= nScalar;
    }

    void operator *= (TYPE nScalar)
    {
        x *= nScalar;
        y *= nScalar;
    }

    tvector2<TYPE> operator - (const tvector2<TYPE> &nVector) const
    {
        return tvector2<TYPE>((x - nVector.x), (y - nVector.y));
    }

    tvector2<TYPE> operator + (const tvector2<TYPE> &nVector) const
    {
        return tvector2<TYPE>((x + nVector.x), (y + nVector.y));
    }

    tvector2<TYPE> operator / (const tvector2<TYPE> &nVector) const
    {
        return tvector2<TYPE>((x / nVector.x), (y / nVector.y));
    }

    tvector2<TYPE> operator * (const tvector2<TYPE> &nVector) const
    {
        return tvector2<TYPE>((x * nVector.x), (y * nVector.y));
    }

    tvector2<TYPE> operator - (TYPE nScalar) const
    {
        return tvector2<TYPE>((x - nScalar), (y - nScalar));
    }

    tvector2<TYPE> operator + (TYPE nScalar) const
    {
        return tvector2<TYPE>((x + nScalar), (y + nScalar));
    }

    tvector2<TYPE> operator / (TYPE nScalar) const
    {
        return tvector2<TYPE>((x / nScalar), (y / nScalar));
    }

    tvector2<TYPE> operator * (TYPE nScalar) const
    {
        return tvector2<TYPE>((x * nScalar), (y * nScalar));
    }
};

template <typename TYPE>
tvector2<TYPE> operator - (const tvector2<TYPE> &nVector)
{
    return tvector2<TYPE>(-nVector.x, -nVector.y);
}

template <typename TYPE>
tvector2<TYPE> operator - (TYPE fValue, const tvector2<TYPE> &nVector)
{
    return tvector2<TYPE>((fValue - nVector.x), (fValue - nVector.y));
}

template <typename TYPE>
tvector2<TYPE> operator + (TYPE fValue, const tvector2<TYPE> &nVector)
{
    return tvector2<TYPE>((fValue + nVector.x), (fValue + nVector.y));
}

template <typename TYPE>
tvector2<TYPE> operator / (TYPE fValue, const tvector2<TYPE> &nVector)
{
    return tvector2<TYPE>((fValue / nVector.x), (fValue / nVector.y));
}

template <typename TYPE>
tvector2<TYPE> operator * (TYPE fValue, const tvector2<TYPE> &nVector)
{
    return tvector2<TYPE>((fValue * nVector.x), (fValue * nVector.y));
}
