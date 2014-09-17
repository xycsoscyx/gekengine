#pragma once

struct aabb
{
public:
    float3 minimum;
    float3 maximum;

public:
    aabb(void)
        : minimum( _INFINITY)
        , maximum(-_INFINITY)
    {
    }

    aabb(const aabb &nBox)
        : minimum(nBox.minimum)
        , maximum(nBox.maximum)
    {
    }

    aabb(float nSize)
        : minimum(-(nSize * 0.5f))
        , maximum( (nSize * 0.5f))
    {
    }

    aabb(const float3 &nMinimum, const float3 &nMaximum)
        : minimum(nMinimum)
        , maximum(nMaximum)
    {
    }

    aabb operator = (const aabb &nBox)
    {
        minimum = nBox.minimum;
        maximum = nBox.maximum;
        return (*this);
    }

    void Extend(float3 &nPoint)
    {
        if (nPoint.x < minimum.x)
        {
            minimum.x = nPoint.x;
        }

        if (nPoint.y < minimum.y)
        {
            minimum.y = nPoint.y;
        }

        if (nPoint.z < minimum.z)
        {
            minimum.z = nPoint.z;
        }

        if (nPoint.x > maximum.x)
        {
            maximum.x = nPoint.x;
        }

        if (nPoint.y > maximum.y)
        {
            maximum.y = nPoint.y;
        }

        if (nPoint.z > maximum.z)
        {
            maximum.z = nPoint.z;
        }
    }

    float3 GetSize(void) const
    {
        return (maximum - minimum);
    }

    float3 GetCenter(void) const
    {
        return (minimum + (GetSize() * 0.5f));
    }

    int GetPosition(const plane &nPlane) const
    {
        float3 nMinimum((nPlane.normal.x > 0.0f ? maximum.x : minimum.x),
                        (nPlane.normal.y > 0.0f ? maximum.y : minimum.y),
                        (nPlane.normal.z > 0.0f ? maximum.z : minimum.z));
        if (nPlane.Distance(nMinimum) < 0.0f)
        {
            return -1;
        }

        float3 nMaximum((nPlane.normal.x < 0.0f ? maximum.x : minimum.x),
                        (nPlane.normal.y < 0.0f ? maximum.y : minimum.y),
                        (nPlane.normal.z < 0.0f ? maximum.z : minimum.z));
        if (nPlane.Distance(nMaximum) < 0.0f)
        {
            return 0;
        }

        return 1;
    }
};
