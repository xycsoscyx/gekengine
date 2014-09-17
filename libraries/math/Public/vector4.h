#pragma once

struct float4
{
public:
    union
    {
        struct { float x, y, z, w; };
        struct { float xyzw[4]; };
        struct { float r, g, b, a; };
        struct { float rgba[4]; };
        struct { float3 normal; float distance; };
    };

public:
    float4(void)
    {
        x = y = z = 0.0f;
        w = 1.0f;        
    }

    float4(float fValue)
    {
        x = y = z = w = fValue;
    }

    float4(const float *pValue)
    {
        memcpy(xyzw, pValue, (sizeof(float) * 4));
    }

    float4(const float2 &nVector)
    {
        x = nVector.x;
        y = nVector.y;
        z = 0.0f;
        w = 1.0f;
    }

    float4(const float3 &nVector)
    {
        x = nVector.x;
        y = nVector.y;
        z = nVector.z;
        w = 1.0f;
    }

    float4(const float3 &nVector, float nW)
    {
        x = nVector.x;
        y = nVector.y;
        z = nVector.z;
        w = nW;
    }

    float4(const float4 &nVector)
    {
        x = nVector.x;
        y = nVector.y;
        z = nVector.z;
        w = nVector.w;
    }

    float4(float nX, float nY, float nZ, float nW)
    {
        x = nX;
        y = nY;
        z = nZ;
        w = nW;
    }

    void Set(float nX, float nY)
    {
        x = nX;
        y = nY;
        z = 0.0f;
        w = 1.0f;
    }

    void Set(float nX, float nY, float nZ)
    {
        x = nX;
        y = nY;
        z = nZ;
        w = 1.0f;
    }

    void Set(float nX, float nY, float nZ, float nW)
    {
        x = nX;
        y = nY;
        z = nZ;
        w = nW;
    }

    void SetLength(float nLength)
    {
        *this *= (nLength / GetLength());
    }

    float GetLengthSqr(void) const
    {
        return ((x * x) + (y * y) + (z * z) + (w * w));
    }

    float GetLength(void) const
    {
        return sqrt((x * x) + (y * y) + (z * z) + (w * w));
    }

    float GetMax(void) const
    {
        return max(max(max(x, y), z), w);
    }

    float4 GetNormal(void) const
    {
        float nLength = GetLength();
        if (nLength != 0.0f)
        {
            return (*this * (1.0f / GetLength()));
        }

        return *this;
    }

    float Dot(const float4 &nVector) const
    {
        return ((x * nVector.x) + (y * nVector.y) + (z * nVector.z) + (w * nVector.w));
    }

    float Distance(const float4 &nVector) const
    {
        return (nVector - *this).GetLength();
    }

    float4 Lerp(const float4 &nVector, float nFactor) const
    {
        return (*this + ((nVector - *this) * nFactor));
    }

    void Normalize(void)
    {
        *this = GetNormal();
    }

    float operator [] (int nIndex) const
    {
        return xyzw[nIndex];
    }

    float &operator [] (int nIndex)
    {
        return xyzw[nIndex];
    }

    bool operator < (const float4 &nVector) const
    {
        if (x >= nVector.x) return false;
        if (y >= nVector.y) return false;
        if (z >= nVector.z) return false;
        if (w >= nVector.w) return false;
        return true;
    }

    bool operator > (const float4 &nVector) const
    {
        if (x <= nVector.x) return false;
        if (y <= nVector.y) return false;
        if (z <= nVector.z) return false;
        if (w <= nVector.w) return false;
        return true;
    }

    bool operator <= (const float4 &nVector) const
    {
        if (x > nVector.x) return false;
        if (y > nVector.y) return false;
        if (z > nVector.z) return false;
        if (w > nVector.w) return false;
        return true;
    }

    bool operator >= (const float4 &nVector) const
    {
        if (x < nVector.x) return false;
        if (y < nVector.y) return false;
        if (z < nVector.z) return false;
        if (w < nVector.w) return false;
        return true;
    }

    bool operator == (const float4 &nVector) const
    {
        if (x != nVector.x) return false;
        if (y != nVector.y) return false;
        if (z != nVector.z) return false;
        if (w != nVector.w) return false;
        return true;
    }

    bool operator != (const float4 &nVector) const
    {
        if (x != nVector.x) return true;
        if (y != nVector.y) return true;
        if (z != nVector.z) return true;
        if (w != nVector.w) return true;
        return false;
    }

    float4 operator = (float nValue)
    {
        x = y = z = w = nValue;
        return (*this);
    }

    float4 operator = (const float2 &nVector)
    {
        x = nVector.x;
        y = nVector.y;
        z = 0.0f;
        w = 1.0f;
        return *this;
    }

    float4 operator = (const float3 &nVector)
    {
        x = nVector.x;
        y = nVector.y;
        z = nVector.z;
        w = 1.0f;
        return *this;
    }

    float4 operator = (const float4 &nVector)
    {
        x = nVector.x;
        y = nVector.y;
        z = nVector.z;
        w = nVector.w;
        return *this;
    }

    void operator -= (const float4 &nVector)
    {
        x -= nVector.x;
        y -= nVector.y;
        z -= nVector.z;
        w -= nVector.w;
    }

    void operator += (const float4 &nVector)
    {
        x += nVector.x;
        y += nVector.y;
        z += nVector.z;
        w += nVector.w;
    }

    void operator /= (const float4 &nVector)
    {
        x /= nVector.x;
        y /= nVector.y;
        z /= nVector.z;
        w /= nVector.w;
    }

    void operator *= (const float4 &nVector)
    {
        x *= nVector.x;
        y *= nVector.y;
        z *= nVector.z;
        w *= nVector.w;
    }

    void operator -= (float nScalar)
    {
        x -= nScalar;
        y -= nScalar;
        z -= nScalar;
        w -= nScalar;
    }

    void operator += (float nScalar)
    {
        x += nScalar;
        y += nScalar;
        z += nScalar;
        w += nScalar;
    }

    void operator /= (float nScalar)
    {
        x /= nScalar;
        y /= nScalar;
        z /= nScalar;
        w /= nScalar;
    }

    void operator *= (float nScalar)
    {
        x *= nScalar;
        y *= nScalar;
        z *= nScalar;
        w *= nScalar;
    }

    float4 operator - (const float4 &nVector) const
    {
        return float4((x - nVector.x), (y - nVector.y), (z - nVector.z), (w - nVector.w));
    }

    float4 operator + (const float4 &nVector) const
    {
        return float4((x + nVector.x), (y + nVector.y), (z + nVector.z), (w + nVector.w));
    }

    float4 operator / (const float4 &nVector) const
    {
        return float4((x / nVector.x), (y / nVector.y), (z / nVector.z), (w / nVector.w));
    }

    float4 operator * (const float4 &nVector) const
    {
        return float4((x * nVector.x), (y * nVector.y), (z * nVector.z), (w * nVector.w));
    }

    float4 operator - (float nScalar) const
    {
        return float4((x - nScalar), (y - nScalar), (z - nScalar), (w - nScalar));
    }

    float4 operator + (float nScalar) const
    {
        return float4((x + nScalar), (y + nScalar), (z + nScalar), (w + nScalar));
    }

    float4 operator / (float nScalar) const
    {
        return float4((x / nScalar), (y / nScalar), (z / nScalar), (w / nScalar));
    }

    float4 operator * (float nScalar) const
    {
        return float4((x * nScalar), (y * nScalar), (z * nScalar), (w * nScalar));
    }
};

float4 operator - (const float4 &nVector)
{
    return float4(-nVector.x, -nVector.y, -nVector.z, -nVector.w);
}

float4 operator - (float fValue, const float4 &nVector)
{
    return float4((fValue - nVector.x), (fValue - nVector.y), (fValue - nVector.z), (fValue - nVector.w));
}

float4 operator + (float fValue, const float4 &nVector)
{
    return float4((fValue + nVector.x), (fValue + nVector.y), (fValue + nVector.z), (fValue + nVector.w));
}

float4 operator / (float fValue, const float4 &nVector)
{
    return float4((fValue / nVector.x), (fValue / nVector.y), (fValue / nVector.z), (fValue / nVector.w));
}

float4 operator * (float fValue, const float4 &nVector)
{
    return float4((fValue * nVector.x), (fValue * nVector.y), (fValue * nVector.z), (fValue * nVector.w));
}
