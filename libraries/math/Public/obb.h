#pragma once

struct obb
{
public:
    float3 position;
    float4x4 rotation;
    float3 halfsize;

public:
    obb(void)
    {
    }

    obb(const obb &nBox)
        : position(nBox.position)
        , rotation(nBox.rotation)
        , halfsize(nBox.halfsize)
    {
    }

    obb(const aabb &nBox, const quaternion &nRotation, const float3 &nPosition)
    {
        rotation = nRotation;
        position = (nPosition + nBox.GetCenter());
        halfsize = (nBox.GetSize() * 0.5f);
    }

    obb(const aabb &nBox, const float4x4 &nMatrix)
    {
        rotation = nMatrix;
        position = (nMatrix.t + nBox.GetCenter());
        halfsize = (nBox.GetSize() * 0.5f);
    }

    obb operator = (const obb &nBox)
    {
        position = nBox.position;
        rotation = nBox.rotation;
        halfsize = nBox.halfsize;
        return (*this);
    }

    int GetPosition(const plane &nPlane) const
    {
        float nDistance = nPlane.Distance(position);
        float nRadiusX = fabs(rotation.rx.Dot(nPlane.normal) * halfsize.x);
        float nRadiusY = fabs(rotation.ry.Dot(nPlane.normal) * halfsize.y);
        float nRadiusZ = fabs(rotation.rz.Dot(nPlane.normal) * halfsize.z);
        float nRadius = (nRadiusX + nRadiusY + nRadiusZ);
        if (nDistance < -nRadius)
        {
            return -1;
        }
        else if (nDistance > nRadius)
        {
            return 1;
        }
        else
        {
            return 0;
        }
    }
};
