#pragma once

template <typename TYPE>
struct tvector3
{
public:
    union
    {
        struct { TYPE x, y, z; };
        struct { TYPE xyz[3]; };
        struct { TYPE r, g, b; };
        struct { TYPE rgb[3]; };
    };

public:
    tvector3(void)
    {
        x = y = z = TYPE(0);
    }

    tvector3(TYPE fValue)
    {
        x = y = z = fValue;
    }

    tvector3(const tvector2<TYPE> &nVector)
    {
        x = nVector.x;
        y = nVector.y;
        z = TYPE(0);
    }

    tvector3(const tvector3<TYPE> &nVector)
    {
        x = nVector.x;
        y = nVector.y;
        z = nVector.z;
    }

    tvector3(const tvector4<TYPE> &nVector)
    {
        x = nVector.x;
        y = nVector.y;
        z = nVector.z;
    }

    tvector3(TYPE nX, TYPE nY, TYPE nZ)
    {
        x = nX;
        y = nY;
        z = nZ;
    }

    void Set(TYPE nX, TYPE nY)
    {
        x = nX;
        y = nY;
        z = TYPE(0);
    }

    void Set(TYPE nX, TYPE nY, TYPE nZ)
    {
        x = nX;
        y = nY;
        z = nZ;
    }

    void Set(TYPE nX, TYPE nY, TYPE nZ, TYPE nW)
    {
        x = nX;
        y = nY;
        z = nZ;
    }

    void SetLength(TYPE nLength)
    {
        (*this) *= (nLength / GetLength());
    }

    TYPE GetLengthSqr(void) const
    {
        return ((x * x) + (y * y) + (z * z));
    }

    TYPE GetLength(void) const
    {
        return sqrt((x * x) + (y * y) + (z * z));
    }

    TYPE GetMax(void) const
    {
        return max(max(x, y), z);
    }

    tvector3<TYPE> GetNormal(void) const
    {
        TYPE nLength(GetLength());
        if (nLength != TYPE(0))
        {
            return ((*this) * (1.0f / GetLength()));
        }

        return (*this);
    }

    TYPE Dot(const tvector3<TYPE> &nVector) const
    {
        return ((x * nVector.x) + (y * nVector.y) +  (z * nVector.z));
    }

    TYPE Distance(const tvector3<TYPE> &nVector) const
    {
        return (nVector - (*this)).GetLength();
    }

    tvector3<TYPE> Cross(const tvector3<TYPE> &nVector) const
    {
        return tvector3<TYPE>(((y * nVector.z) - (z * nVector.y)),
                              ((z * nVector.x) - (x * nVector.z)),
                              ((x * nVector.y) - (y * nVector.x)));
    }

    tvector3<TYPE> Lerp(const tvector3<TYPE> &nVector, TYPE nFactor) const
    {
        return ((*this) + ((nVector - (*this)) * nFactor));
    }

    void Normalize(void)
    {
        (*this) = GetNormal();
    }

    TYPE operator [] (int nIndex) const
    {
        return xyz[nIndex];
    }

    TYPE &operator [] (int nIndex)
    {
        return xyz[nIndex];
    }

    bool operator < (const tvector3<TYPE> &nVector) const
    {
        if (x >= nVector.x) return false;
        if (y >= nVector.y) return false;
        if (z >= nVector.z) return false;
        return true;
    }

    bool operator > (const tvector3<TYPE> &nVector) const
    {
        if (x <= nVector.x) return false;
        if (y <= nVector.y) return false;
        if (z <= nVector.z) return false;
        return true;
    }

    bool operator <= (const tvector3<TYPE> &nVector) const
    {
        if (x > nVector.x) return false;
        if (y > nVector.y) return false;
        if (z > nVector.z) return false;
        return true;
    }

    bool operator >= (const tvector3<TYPE> &nVector) const
    {
        if (x < nVector.x) return false;
        if (y < nVector.y) return false;
        if (z < nVector.z) return false;
        return true;
    }

    bool operator == (const tvector3<TYPE> &nVector) const
    {
        if (x != nVector.x) return false;
        if (y != nVector.y) return false;
        if (z != nVector.z) return false;
        return true;
    }

    bool operator != (const tvector3<TYPE> &nVector) const
    {
        if (x != nVector.x) return true;
        if (y != nVector.y) return true;
        if (z != nVector.z) return true;
        return false;
    }

    tvector3<TYPE> operator = (float nValue)
    {
        x = y = z = nValue;
        return (*this);
    }

    tvector3<TYPE> operator = (const tvector2<TYPE> &nVector)
    {
        x = nVector.x;
        y = nVector.y;
        z = TYPE(0);
        return (*this);
    }

    tvector3<TYPE> operator = (const tvector3<TYPE> &nVector)
    {
        x = nVector.x;
        y = nVector.y;
        z = nVector.z;
        return (*this);
    }

    tvector3<TYPE> operator = (const tvector4<TYPE> &nVector)
    {
        x = nVector.x;
        y = nVector.y;
        z = nVector.z;
        return (*this);
    }

    void operator -= (const tvector3<TYPE> &nVector)
    {
        x -= nVector.x;
        y -= nVector.y;
        z -= nVector.z;
    }

    void operator += (const tvector3<TYPE> &nVector)
    {
        x += nVector.x;
        y += nVector.y;
        z += nVector.z;
    }

    void operator /= (const tvector3<TYPE> &nVector)
    {
        x /= nVector.x;
        y /= nVector.y;
        z /= nVector.z;
    }

    void operator *= (const tvector3<TYPE> &nVector)
    {
        x *= nVector.x;
        y *= nVector.y;
        z *= nVector.z;
    }

    void operator -= (TYPE nScalar)
    {
        x -= nScalar;
        y -= nScalar;
        z -= nScalar;
    }

    void operator += (TYPE nScalar)
    {
        x += nScalar;
        y += nScalar;
        z += nScalar;
    }

    void operator /= (TYPE nScalar)
    {
        x /= nScalar;
        y /= nScalar;
        z /= nScalar;
    }

    void operator *= (TYPE nScalar)
    {
        x *= nScalar;
        y *= nScalar;
        z *= nScalar;
    }

    tvector3<TYPE> operator - (const tvector3<TYPE> &nVector) const
    {
        return tvector3<TYPE>((x - nVector.x), (y - nVector.y), (z - nVector.z));
    }

    tvector3<TYPE> operator + (const tvector3<TYPE> &nVector) const
    {
        return tvector3<TYPE>((x + nVector.x), (y + nVector.y), (z + nVector.z));
    }

    tvector3<TYPE> operator / (const tvector3<TYPE> &nVector) const
    {
        return tvector3<TYPE>((x / nVector.x), (y / nVector.y), (z / nVector.z));
    }

    tvector3<TYPE> operator * (const tvector3<TYPE> &nVector) const
    {
        return tvector3<TYPE>((x * nVector.x), (y * nVector.y), (z * nVector.z));
    }

    tvector3<TYPE> operator - (TYPE nScalar) const
    {
        return tvector3<TYPE>((x - nScalar), (y - nScalar), (z - nScalar));
    }

    tvector3<TYPE> operator + (TYPE nScalar) const
    {
        return tvector3<TYPE>((x + nScalar), (y + nScalar), (z + nScalar));
    }

    tvector3<TYPE> operator / (TYPE nScalar) const
    {
        return tvector3<TYPE>((x / nScalar), (y / nScalar), (z / nScalar));
    }

    tvector3<TYPE> operator * (TYPE nScalar) const
    {
        return tvector3<TYPE>((x * nScalar), (y * nScalar), (z * nScalar));
    }
};

template <typename TYPE>
tvector3<TYPE> operator - (const tvector3<TYPE> &nVector)
{
    return tvector3<TYPE>(-nVector.x, -nVector.y, -nVector.z);
}

template <typename TYPE>
tvector3<TYPE> operator - (TYPE fValue, const tvector3<TYPE> &nVector)
{
    return tvector3<TYPE>((fValue - nVector.x), (fValue - nVector.y), (fValue - nVector.z));
}

template <typename TYPE>
tvector3<TYPE> operator + (TYPE fValue, const tvector3<TYPE> &nVector)
{
    return tvector3<TYPE>((fValue + nVector.x), (fValue + nVector.y), (fValue + nVector.z));
}

template <typename TYPE>
tvector3<TYPE> operator / (TYPE fValue, const tvector3<TYPE> &nVector)
{
    return tvector3<TYPE>((fValue / nVector.x), (fValue / nVector.y), (fValue / nVector.z));
}

template <typename TYPE>
tvector3<TYPE> operator * (TYPE fValue, const tvector3<TYPE> &nVector)
{
    return tvector3<TYPE>((fValue * nVector.x), (fValue * nVector.y), (fValue * nVector.z));
}
