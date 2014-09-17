#pragma once

struct float3;

struct float4;

struct float2
{
public:
    union
    {
        struct { float x, y; };
        struct { float xy[2]; };
        struct { float u, v; };
        struct { float uv[2]; };
    };

public:
    float2(void)
    {
        x = y = 0.0f;
    }

    float2(float fValue)
    {
        x = y = fValue;
    }

    float2(const float2 &nVector)
    {
        x = nVector.x;
        y = nVector.y;
    }

    float2(const float3 &nVector)
    {
        x = nVector.x;
        y = nVector.y;
    }

    float2(const float4 &nVector)
    {
        x = nVector.x;
        y = nVector.y;
    }

    float2(float nX, float nY)
    {
        x = nX;
        y = nY;
    }

    void Set(float nX, float nY)
    {
        x = nX;
        y = nY;
    }

    void Set(float nX, float nY, float nZ)
    {
        x = nX;
        y = nY;
    }

    void Set(float nX, float nY, float nZ, float nW)
    {
        x = nX;
        y = nY;
    }

    void SetLength(float nLength)
    {
        (*this) *= (nLength / GetLength());
    }

    float GetLength(void) const
    {
        return sqrt((x * x) + (y * y));
    }

    float GetLengthSqr(void) const
    {
        return ((x * x) + (y * y));
    }

    float GetMax(void) const
    {
        return max(x, y);
    }

    float2 GetNormal(void) const
    {
        float nLength = GetLength();
        if (nLength != 0.0f)
        {
            return ((*this) * (1.0f / GetLength()));
        }

        return (*this);
    }

    void Normalize(void)
    {
        (*this) = GetNormal();
    }

    float Dot(const float2 &nVector) const
    {
        return ((x * nVector.x) + (y * nVector.y));
    }

    float Distance(const float2 &nVector) const
    {
        return (nVector - (*this)).GetLength();
    }

    float2 Lerp(const float2 &nVector, float nFactor) const
    {
        return ((*this) + ((nVector - (*this)) * nFactor));
    }

    float operator [] (int nIndex) const
    {
        return xy[nIndex];
    }

    float &operator [] (int nIndex)
    {
        return xy[nIndex];
    }

    bool operator < (const float2 &nVector) const
    {
        if (x >= nVector.x) return false;
        if (y >= nVector.y) return false;
        return true;
    }

    bool operator > (const float2 &nVector) const
    {
        if (x <= nVector.x) return false;
        if (y <= nVector.y) return false;
        return true;
    }

    bool operator <= (const float2 &nVector) const
    {
        if (x > nVector.x) return false;
        if (y > nVector.y) return false;
        return true;
    }

    bool operator >= (const float2 &nVector) const
    {
        if (x < nVector.x) return false;
        if (y < nVector.y) return false;
        return true;
    }

    bool operator == (const float2 &nVector) const
    {
        if (x != nVector.x) return false;
        if (y != nVector.y) return false;
        return true;
    }

    bool operator != (const float2 &nVector) const
    {
        if (x != nVector.x) return true;
        if (y != nVector.y) return true;
        return false;
    }

    float2 operator = (float nValue)
    {
        x = y = nValue;
        return (*this);
    }

    float2 operator = (const float2 &nVector)
    {
        x = nVector.x;
        y = nVector.y;
        return (*this);
    }

    float2 operator = (const float3 &nVector)
    {
        x = nVector.x;
        y = nVector.y;
        return (*this);
    }

    float2 operator = (const float4 &nVector)
    {
        x = nVector.x;
        y = nVector.y;
        return (*this);
    }

    void operator -= (const float2 &nVector)
    {
        x -= nVector.x;
        y -= nVector.y;
    }

    void operator += (const float2 &nVector)
    {
        x += nVector.x;
        y += nVector.y;
    }

    void operator /= (const float2 &nVector)
    {
        x /= nVector.x;
        y /= nVector.y;
    }

    void operator *= (const float2 &nVector)
    {
        x *= nVector.x;
        y *= nVector.y;
    }

    void operator -= (float nScalar)
    {
        x -= nScalar;
        y -= nScalar;
    }

    void operator += (float nScalar)
    {
        x += nScalar;
        y += nScalar;
    }

    void operator /= (float nScalar)
    {
        x /= nScalar;
        y /= nScalar;
    }

    void operator *= (float nScalar)
    {
        x *= nScalar;
        y *= nScalar;
    }

    float2 operator - (const float2 &nVector) const
    {
        return float2((x - nVector.x), (y - nVector.y));
    }

    float2 operator + (const float2 &nVector) const
    {
        return float2((x + nVector.x), (y + nVector.y));
    }

    float2 operator / (const float2 &nVector) const
    {
        return float2((x / nVector.x), (y / nVector.y));
    }

    float2 operator * (const float2 &nVector) const
    {
        return float2((x * nVector.x), (y * nVector.y));
    }

    float2 operator - (float nScalar) const
    {
        return float2((x - nScalar), (y - nScalar));
    }

    float2 operator + (float nScalar) const
    {
        return float2((x + nScalar), (y + nScalar));
    }

    float2 operator / (float nScalar) const
    {
        return float2((x / nScalar), (y / nScalar));
    }

    float2 operator * (float nScalar) const
    {
        return float2((x * nScalar), (y * nScalar));
    }
};

float2 operator - (const float2 &nVector)
{
    return float2(-nVector.x, -nVector.y);
}

float2 operator - (float fValue, const float2 &nVector)
{
    return float2((fValue - nVector.x), (fValue - nVector.y));
}

float2 operator + (float fValue, const float2 &nVector)
{
    return float2((fValue + nVector.x), (fValue + nVector.y));
}

float2 operator / (float fValue, const float2 &nVector)
{
    return float2((fValue / nVector.x), (fValue / nVector.y));
}

float2 operator * (float fValue, const float2 &nVector)
{
    return float2((fValue * nVector.x), (fValue * nVector.y));
}
