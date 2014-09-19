#pragma once

struct GEKSphere
{
public:
    Eigen::Vector3f position;
    float radius;

public:
    GEKSphere(void)
        : radius(0.0f)
    {
    }

    GEKSphere(const GEKSphere &nSphere)
        : position(nSphere.position)
        , radius(nSphere.radius)
    {
    }

    GEKSphere(const Eigen::Vector3f &nPosition, float nRadius)
        : position(nPosition)
        , radius(nRadius)
    {
    }

    GEKSphere operator = (const GEKSphere &nSphere)
    {
        position = nSphere.position;
        radius = nSphere.radius;
        return (*this);
    }

    int GetPosition(const GEKPlane &nPlane) const
    {
        float nDistance = nPlane.Distance(position);
        if (nDistance < -radius)
        {
            return -1;
        }
        else if (nDistance > radius)
        {
            return 1;
        }
        else
        {
            return 0;
        }
    }
};
