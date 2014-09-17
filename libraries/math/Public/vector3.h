#pragma once

struct float4;

struct float3
{
public:
    union
    {
        struct { float x, y, z; };
        struct { float xyz[3]; };
        struct { float r, g, b; };
        struct { float rgb[3]; };
    };

public:
    float3(void)
    {
        x = y = z = 0.0f;
    }

    float3(float fValue)
    {
        x = y = z = fValue;
    }

    float3(const float2 &nVector)
    {
        x = nVector.x;
        y = nVector.y;
        z = 0.0f;
    }

    float3(const float3 &nVector)
    {
        x = nVector.x;
        y = nVector.y;
        z = nVector.z;
    }

    float3(const float4 &nVector)
    {
        x = nVector.x;
        y = nVector.y;
        z = nVector.z;
    }

    float3(float nX, float nY, float nZ)
    {
        x = nX;
        y = nY;
        z = nZ;
    }

    void Set(float nX, float nY)
    {
        x = nX;
        y = nY;
        z = 0.0f;
    }

    void Set(float nX, float nY, float nZ)
    {
        x = nX;
        y = nY;
        z = nZ;
    }

    void Set(float nX, float nY, float nZ, float nW)
    {
        x = nX;
        y = nY;
        z = nZ;
    }

    void SetLength(float nLength)
    {
        (*this) *= (nLength / GetLength());
    }

    float GetLengthSqr(void) const
    {
        return ((x * x) + (y * y) + (z * z));
    }

    float GetLength(void) const
    {
        return sqrt((x * x) + (y * y) + (z * z));
    }

    float GetMax(void) const
    {
        return max(max(x, y), z);
    }

    float3 GetNormal(void) const
    {
        float nLength = GetLength();
        if (nLength != 0.0f)
        {
            return ((*this) * (1.0f / GetLength()));
        }

        return (*this);
    }

    float Dot(const float3 &nVector) const
    {
        return ((x * nVector.x) + (y * nVector.y) +  (z * nVector.z));
    }

    float Distance(const float3 &nVector) const
    {
        return (nVector - (*this)).GetLength();
    }

    float3 Cross(const float3 &nVector) const
    {
        return float3(((y * nVector.z) - (z * nVector.y)),
                       ((z * nVector.x) - (x * nVector.z)),
                       ((x * nVector.y) - (y * nVector.x)));
    }

    float3 Lerp(const float3 &nVector, float nFactor) const
    {
        return ((*this) + ((nVector - (*this)) * nFactor));
    }

    void Normalize(void)
    {
        (*this) = GetNormal();
    }

    float operator [] (int nIndex) const
    {
        return xyz[nIndex];
    }

    float &operator [] (int nIndex)
    {
        return xyz[nIndex];
    }

    bool operator < (const float3 &nVector) const
    {
        if (x >= nVector.x) return false;
        if (y >= nVector.y) return false;
        if (z >= nVector.z) return false;
        return true;
    }

    bool operator > (const float3 &nVector) const
    {
        if (x <= nVector.x) return false;
        if (y <= nVector.y) return false;
        if (z <= nVector.z) return false;
        return true;
    }

    bool operator <= (const float3 &nVector) const
    {
        if (x > nVector.x) return false;
        if (y > nVector.y) return false;
        if (z > nVector.z) return false;
        return true;
    }

    bool operator >= (const float3 &nVector) const
    {
        if (x < nVector.x) return false;
        if (y < nVector.y) return false;
        if (z < nVector.z) return false;
        return true;
    }

    bool operator == (const float3 &nVector) const
    {
        if (x != nVector.x) return false;
        if (y != nVector.y) return false;
        if (z != nVector.z) return false;
        return true;
    }

    bool operator != (const float3 &nVector) const
    {
        if (x != nVector.x) return true;
        if (y != nVector.y) return true;
        if (z != nVector.z) return true;
        return false;
    }

    float3 operator = (float nValue)
    {
        x = y = z = nValue;
        return (*this);
    }

    float3 operator = (const float2 &nVector)
    {
        x = nVector.x;
        y = nVector.y;
        z = 0.0f;
        return (*this);
    }

    float3 operator = (const float3 &nVector)
    {
        x = nVector.x;
        y = nVector.y;
        z = nVector.z;
        return (*this);
    }

    float3 operator = (const float4 &nVector)
    {
        x = nVector.x;
        y = nVector.y;
        z = nVector.z;
        return (*this);
    }

    void operator -= (const float3 &nVector)
    {
        x -= nVector.x;
        y -= nVector.y;
        z -= nVector.z;
    }

    void operator += (const float3 &nVector)
    {
        x += nVector.x;
        y += nVector.y;
        z += nVector.z;
    }

    void operator /= (const float3 &nVector)
    {
        x /= nVector.x;
        y /= nVector.y;
        z /= nVector.z;
    }

    void operator *= (const float3 &nVector)
    {
        x *= nVector.x;
        y *= nVector.y;
        z *= nVector.z;
    }

    void operator -= (float nScalar)
    {
        x -= nScalar;
        y -= nScalar;
        z -= nScalar;
    }

    void operator += (float nScalar)
    {
        x += nScalar;
        y += nScalar;
        z += nScalar;
    }

    void operator /= (float nScalar)
    {
        x /= nScalar;
        y /= nScalar;
        z /= nScalar;
    }

    void operator *= (float nScalar)
    {
        x *= nScalar;
        y *= nScalar;
        z *= nScalar;
    }

    float3 operator - (const float3 &nVector) const
    {
        return float3((x - nVector.x), (y - nVector.y), (z - nVector.z));
    }

    float3 operator + (const float3 &nVector) const
    {
        return float3((x + nVector.x), (y + nVector.y), (z + nVector.z));
    }

    float3 operator / (const float3 &nVector) const
    {
        return float3((x / nVector.x), (y / nVector.y), (z / nVector.z));
    }

    float3 operator * (const float3 &nVector) const
    {
        return float3((x * nVector.x), (y * nVector.y), (z * nVector.z));
    }

    float3 operator - (float nScalar) const
    {
        return float3((x - nScalar), (y - nScalar), (z - nScalar));
    }

    float3 operator + (float nScalar) const
    {
        return float3((x + nScalar), (y + nScalar), (z + nScalar));
    }

    float3 operator / (float nScalar) const
    {
        return float3((x / nScalar), (y / nScalar), (z / nScalar));
    }

    float3 operator * (float nScalar) const
    {
        return float3((x * nScalar), (y * nScalar), (z * nScalar));
    }
};

float3 operator - (const float3 &nVector)
{
    return float3(-nVector.x, -nVector.y, -nVector.z);
}

float3 operator - (float fValue, const float3 &nVector)
{
    return float3((fValue - nVector.x), (fValue - nVector.y), (fValue - nVector.z));
}

float3 operator + (float fValue, const float3 &nVector)
{
    return float3((fValue + nVector.x), (fValue + nVector.y), (fValue + nVector.z));
}

float3 operator / (float fValue, const float3 &nVector)
{
    return float3((fValue / nVector.x), (fValue / nVector.y), (fValue / nVector.z));
}

float3 operator * (float fValue, const float3 &nVector)
{
    return float3((fValue * nVector.x), (fValue * nVector.y), (fValue * nVector.z));
}
