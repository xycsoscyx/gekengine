#pragma once

struct sphere
{
public:
    float3 position;
    float radius;

public:
    sphere(void)
        : radius(0.0f)
    {
    }

    sphere(const sphere &nSphere)
        : position(nSphere.position)
        , radius(nSphere.radius)
    {
    }

    sphere(const float3 &nPosition, float nRadius)
        : position(nPosition)
        , radius(nRadius)
    {
    }

    sphere operator = (const sphere &nSphere)
    {
        position = nBox.position;
        radius = nBox.radius;
        return (*this);
    }

    int GetPosition(const plane &nPlane) const
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
