#pragma once

struct plane
{
public:
    float3 normal;
    float distance;

public:
    plane(void)
    {
        normal.x = 0.0f;
        normal.y = 0.0f;
        normal.z = 0.0f;
        distance = 0.0f;
    }

    plane(const plane &nPlane)
    {
        normal = nPlane.normal;
        distance = nPlane.distance;
    }

    plane(const float4 &nPlane)
    {
        normal.x = nPlane.x;
        normal.y = nPlane.y;
        normal.z = nPlane.z;
        distance = nPlane.w;
    }

    plane(float nA, float nB, float nC, float nD)
    {
        normal.x = nA;
        normal.y = nB;
        normal.z = nC;
        distance = nD;
    }

    plane(const float3 &nNormal, float nDistance)
    {
        normal = nNormal;
        distance = nDistance;
    }

    plane(const float3 &nPointA, const float3 &nPointB, const float3 &nPointC)
    {
        float3 kSideA(nPointB - nPointA);
        float3 kSideB(nPointC - nPointA);
        float3 kCross(kSideA.Cross(kSideB));

        normal = kCross.GetNormal();
        distance = -normal.Dot(nPointA);
    }

    plane(const float3 &nNormal, const float3 &nPointOnPlane)
    {
        normal = nNormal;
        distance = -normal.Dot(nPointOnPlane);
    }

    plane operator = (const plane &nPlane)
    {
        normal = nPlane.normal;
        distance = nPlane.distance;
        return *this;
    }

    void Normalize(void)
    {
        float nLength = (1.0f / normal.GetLength());
        normal.x *= nLength;
        normal.y *= nLength;
        normal.z *= nLength;
        distance *= nLength;
    }

    float Distance(const float3 &nPoint) const
    {
        return (normal.Dot(nPoint) + distance);
    }
};
