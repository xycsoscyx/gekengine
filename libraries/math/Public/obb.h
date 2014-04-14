#pragma once

template <typename TYPE>
struct tobb
{
public:
    tvector3<TYPE> position;
    tquaternion<TYPE> rotation;
    tvector3<TYPE> size;

public:
    tobb(void)
    {
    }

    tobb(const tobb<TYPE> &nBox)
        : position(nBox.position)
        , rotation(nBox.rotation)
        , size(nBox.size)
    {
    }

    tobb(const taabb<TYPE> &nBox, const tquaternion<TYPE> &nRotation, const tvector3<TYPE> &nPosition)
    {
        rotation = nRotation;
        position = (nPosition + nBox.GetCenter());
        size = nBox.GetSize();
    }

    tobb(const taabb<TYPE> &nBox, const tmatrix4x4<TYPE> &nMatrix)
    {
        rotation = nMatrix;
        position = (nMatrix.t + nBox.GetCenter());
        size = nBox.GetSize();
    }

    tobb operator = (const tobb<TYPE> &nBox)
    {
        position = nBox.position;
        rotation = nBox.rotation;
        size = nBox.size;
        return (*this);
    }

    int GetPosition(const tplane<TYPE> &nPlane) const
    {
        tmatrix4x4<TYPE> nRotation(rotation);
        TYPE nDistance = nPlane.Distance(position);
        TYPE nRadiusX = fabs(nRotation.rx.Dot(nPlane.normal) * (size.x / TYPE(2)));
        TYPE nRadiusY = fabs(nRotation.ry.Dot(nPlane.normal) * (size.y / TYPE(2)));
        TYPE nRadiusZ = fabs(nRotation.rz.Dot(nPlane.normal) * (size.z / TYPE(2)));
        TYPE nRadius = (nRadiusX + nRadiusY + nRadiusZ);
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
