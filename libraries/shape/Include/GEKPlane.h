#pragma once

struct GEKPlane
{
public:
    Eigen::Vector3f normal;
    float distance;

public:
    GEKPlane(void)
    {
        normal[0] = 0.0f;
        normal[1] = 0.0f;
        normal[2] = 0.0f;
        distance = 0.0f;
    }

    GEKPlane(const GEKPlane &nPlane)
    {
        normal = nPlane.normal;
        distance = nPlane.distance;
    }

    GEKPlane(const Eigen::Vector4f &nPlane)
    {
        normal[0] = nPlane[0];
        normal[1] = nPlane[1];
        normal[2] = nPlane[2];
        distance = nPlane[3];
    }

    GEKPlane(float nA, float nB, float nC, float nD)
    {
        normal[0] = nA;
        normal[1] = nB;
        normal[2] = nC;
        distance = nD;
    }

    GEKPlane(const Eigen::Vector3f &nNormal, float nDistance)
    {
        normal = nNormal;
        distance = nDistance;
    }

    GEKPlane(const Eigen::Vector3f &nPointA, const Eigen::Vector3f &nPointB, const Eigen::Vector3f &nPointC)
    {
        Eigen::Vector3f kSideA(nPointB - nPointA);
        Eigen::Vector3f kSideB(nPointC - nPointA);
        Eigen::Vector3f kCross(kSideA.cross(kSideB));

        normal = kCross.normalized();
        distance = -normal.dot(nPointA);
    }

    GEKPlane(const Eigen::Vector3f &nNormal, const Eigen::Vector3f &nPointOnPlane)
    {
        normal = nNormal;
        distance = -normal.dot(nPointOnPlane);
    }

    GEKPlane operator = (const GEKPlane &nPlane)
    {
        normal = nPlane.normal;
        distance = nPlane.distance;
        return *this;
    }

    void Normalize(void)
    {
        float nLength = (1.0f / normal.norm());
        normal *= nLength;
        distance *= nLength;
    }

    float Distance(const Eigen::Vector3f &nPoint) const
    {
        return (normal.dot(nPoint) + distance);
    }
};
