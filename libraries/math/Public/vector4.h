#pragma once

template <typename TYPE>
struct tvector4
{
public:
    union
    {
        struct { TYPE x, y, z, w; };
        struct { TYPE xyzw[4]; };
        struct { TYPE r, g, b, a; };
        struct { TYPE rgba[4]; };
        struct { tvector3<TYPE> normal; TYPE distance; };
    };

public:
    tvector4(void)
    {
        x = y = z = TYPE(0);
        w = TYPE(1);        
    }

    tvector4(TYPE fValue)
    {
        x = y = z = w = fValue;
    }

    tvector4(const TYPE *pValue)
    {
        memcpy(xyzw, pValue, (sizeof(float) * 4));
    }

    tvector4(const tvector2<TYPE> &nVector)
    {
        x = nVector.x;
        y = nVector.y;
        z = TYPE(0);
        w = TYPE(1);
    }

    tvector4(const tvector3<TYPE> &nVector)
    {
        x = nVector.x;
        y = nVector.y;
        z = nVector.z;
        w = TYPE(1);
    }

    tvector4(const tvector3<TYPE> &nVector, TYPE nW)
    {
        x = nVector.x;
        y = nVector.y;
        z = nVector.z;
        w = nW;
    }

    tvector4(const tvector4<TYPE> &nVector)
    {
        x = nVector.x;
        y = nVector.y;
        z = nVector.z;
        w = nVector.w;
    }

    tvector4(TYPE nX, TYPE nY, TYPE nZ, TYPE nW)
    {
        x = nX;
        y = nY;
        z = nZ;
        w = nW;
    }

    void Set(TYPE nX, TYPE nY)
    {
        x = nX;
        y = nY;
        z = TYPE(0);
        w = TYPE(1);
    }

    void Set(TYPE nX, TYPE nY, TYPE nZ)
    {
        x = nX;
        y = nY;
        z = nZ;
        w = TYPE(1);
    }

    void Set(TYPE nX, TYPE nY, TYPE nZ, TYPE nW)
    {
        x = nX;
        y = nY;
        z = nZ;
        w = nW;
    }

    void SetLength(TYPE nLength)
    {
        *this *= (nLength / GetLength());
    }

    TYPE GetLengthSqr(void) const
    {
        return ((x * x) + (y * y) + (z * z) + (w * w));
    }

    TYPE GetLength(void) const
    {
        return sqrt((x * x) + (y * y) + (z * z) + (w * w));
    }

    TYPE GetMax(void) const
    {
        return max(max(max(x, y), z), w);
    }

    tvector4<TYPE> GetNormal(void) const
    {
        TYPE nLength = GetLength();
        if (nLength != TYPE(0))
        {
            return (*this * (TYPE(1) / GetLength()));
        }

        return *this;
    }

    TYPE Dot(const tvector4<TYPE> &nVector) const
    {
        return ((x * nVector.x) + (y * nVector.y) + (z * nVector.z) + (w * nVector.w));
    }

    TYPE Distance(const tvector4<TYPE> &nVector) const
    {
        return (nVector - *this).GetLength();
    }

    tvector4<TYPE> Lerp(const tvector4<TYPE> &nVector, TYPE nFactor) const
    {
        return (*this + ((nVector - *this) * nFactor));
    }

    void Normalize(void)
    {
        *this = GetNormal();
    }

    TYPE operator [] (int nIndex) const
    {
        return xyzw[nIndex];
    }

    TYPE &operator [] (int nIndex)
    {
        return xyzw[nIndex];
    }

    bool operator < (const tvector4<TYPE> &nVector) const
    {
        if (x >= nVector.x) return false;
        if (y >= nVector.y) return false;
        if (z >= nVector.z) return false;
        if (w >= nVector.w) return false;
        return true;
    }

    bool operator > (const tvector4<TYPE> &nVector) const
    {
        if (x <= nVector.x) return false;
        if (y <= nVector.y) return false;
        if (z <= nVector.z) return false;
        if (w <= nVector.w) return false;
        return true;
    }

    bool operator <= (const tvector4<TYPE> &nVector) const
    {
        if (x > nVector.x) return false;
        if (y > nVector.y) return false;
        if (z > nVector.z) return false;
        if (w > nVector.w) return false;
        return true;
    }

    bool operator >= (const tvector4<TYPE> &nVector) const
    {
        if (x < nVector.x) return false;
        if (y < nVector.y) return false;
        if (z < nVector.z) return false;
        if (w < nVector.w) return false;
        return true;
    }

    bool operator == (const tvector4<TYPE> &nVector) const
    {
        if (x != nVector.x) return false;
        if (y != nVector.y) return false;
        if (z != nVector.z) return false;
        if (w != nVector.w) return false;
        return true;
    }

    bool operator != (const tvector4<TYPE> &nVector) const
    {
        if (x != nVector.x) return true;
        if (y != nVector.y) return true;
        if (z != nVector.z) return true;
        if (w != nVector.w) return true;
        return false;
    }

    tvector4<TYPE> operator = (float nValue)
    {
        x = y = z = w = nValue;
        return (*this);
    }

    tvector4<TYPE> operator = (const tvector2<TYPE> &nVector)
    {
        x = nVector.x;
        y = nVector.y;
        z = TYPE(0);
        w = TYPE(1);
        return *this;
    }

    tvector4<TYPE> operator = (const tvector3<TYPE> &nVector)
    {
        x = nVector.x;
        y = nVector.y;
        z = nVector.z;
        w = TYPE(1);
        return *this;
    }

    tvector4<TYPE> operator = (const tvector4<TYPE> &nVector)
    {
        x = nVector.x;
        y = nVector.y;
        z = nVector.z;
        w = nVector.w;
        return *this;
    }

    void operator -= (const tvector4<TYPE> &nVector)
    {
        x -= nVector.x;
        y -= nVector.y;
        z -= nVector.z;
        w -= nVector.w;
    }

    void operator += (const tvector4<TYPE> &nVector)
    {
        x += nVector.x;
        y += nVector.y;
        z += nVector.z;
        w += nVector.w;
    }

    void operator /= (const tvector4<TYPE> &nVector)
    {
        x /= nVector.x;
        y /= nVector.y;
        z /= nVector.z;
        w /= nVector.w;
    }

    void operator *= (const tvector4<TYPE> &nVector)
    {
        x *= nVector.x;
        y *= nVector.y;
        z *= nVector.z;
        w *= nVector.w;
    }

    void operator -= (TYPE nScalar)
    {
        x -= nScalar;
        y -= nScalar;
        z -= nScalar;
        w -= nScalar;
    }

    void operator += (TYPE nScalar)
    {
        x += nScalar;
        y += nScalar;
        z += nScalar;
        w += nScalar;
    }

    void operator /= (TYPE nScalar)
    {
        x /= nScalar;
        y /= nScalar;
        z /= nScalar;
        w /= nScalar;
    }

    void operator *= (TYPE nScalar)
    {
        x *= nScalar;
        y *= nScalar;
        z *= nScalar;
        w *= nScalar;
    }

    tvector4<TYPE> operator - (const tvector4<TYPE> &nVector) const
    {
        return tvector4<TYPE>((x - nVector.x), (y - nVector.y), (z - nVector.z), (w - nVector.w));
    }

    tvector4<TYPE> operator + (const tvector4<TYPE> &nVector) const
    {
        return tvector4<TYPE>((x + nVector.x), (y + nVector.y), (z + nVector.z), (w + nVector.w));
    }

    tvector4<TYPE> operator / (const tvector4<TYPE> &nVector) const
    {
        return tvector4<TYPE>((x / nVector.x), (y / nVector.y), (z / nVector.z), (w / nVector.w));
    }

    tvector4<TYPE> operator * (const tvector4<TYPE> &nVector) const
    {
        return tvector4<TYPE>((x * nVector.x), (y * nVector.y), (z * nVector.z), (w * nVector.w));
    }

    tvector4<TYPE> operator - (TYPE nScalar) const
    {
        return tvector4<TYPE>((x - nScalar), (y - nScalar), (z - nScalar), (w - nScalar));
    }

    tvector4<TYPE> operator + (TYPE nScalar) const
    {
        return tvector4<TYPE>((x + nScalar), (y + nScalar), (z + nScalar), (w + nScalar));
    }

    tvector4<TYPE> operator / (TYPE nScalar) const
    {
        return tvector4<TYPE>((x / nScalar), (y / nScalar), (z / nScalar), (w / nScalar));
    }

    tvector4<TYPE> operator * (TYPE nScalar) const
    {
        return tvector4<TYPE>((x * nScalar), (y * nScalar), (z * nScalar), (w * nScalar));
    }
};

template <typename TYPE>
tvector4<TYPE> operator - (const tvector4<TYPE> &nVector)
{
    return tvector4<TYPE>(-nVector.x, -nVector.y, -nVector.z, -nVector.w);
}

template <typename TYPE>
tvector4<TYPE> operator - (TYPE fValue, const tvector4<TYPE> &nVector)
{
    return tvector4<TYPE>((fValue - nVector.x), (fValue - nVector.y), (fValue - nVector.z), (fValue - nVector.w));
}

template <typename TYPE>
tvector4<TYPE> operator + (TYPE fValue, const tvector4<TYPE> &nVector)
{
    return tvector4<TYPE>((fValue + nVector.x), (fValue + nVector.y), (fValue + nVector.z), (fValue + nVector.w));
}

template <typename TYPE>
tvector4<TYPE> operator / (TYPE fValue, const tvector4<TYPE> &nVector)
{
    return tvector4<TYPE>((fValue / nVector.x), (fValue / nVector.y), (fValue / nVector.z), (fValue / nVector.w));
}

template <typename TYPE>
tvector4<TYPE> operator * (TYPE fValue, const tvector4<TYPE> &nVector)
{
    return tvector4<TYPE>((fValue * nVector.x), (fValue * nVector.y), (fValue * nVector.z), (fValue * nVector.w));
}
