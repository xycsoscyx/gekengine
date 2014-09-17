#pragma once

template <typename TYPE>
struct tobb
{
public:
    tvector3<TYPE> position;
    tmatrix4x4<TYPE> rotation;
    tvector3<TYPE> halfsize;

public:
    tobb(void)
    {
    }

    tobb(const tobb<TYPE> &nBox)
        : position(nBox.position)
        , rotation(nBox.rotation)
        , halfsize(nBox.halfsize)
    {
    }

    tobb(const taabb<TYPE> &nBox, const tquaternion<TYPE> &nRotation, const tvector3<TYPE> &nPosition)
    {
        rotation = nRotation;
        position = (nPosition + nBox.GetCenter());
        halfsize = (nBox.GetSize() * TYPE(0.5));
    }

    tobb(const taabb<TYPE> &nBox, const tmatrix4x4<TYPE> &nMatrix)
    {
        rotation = nMatrix;
        position = (nMatrix.t + nBox.GetCenter());
        halfsize = (nBox.GetSize() * TYPE(0.5));
    }

    tobb operator = (const tobb<TYPE> &nBox)
    {
        position = nBox.position;
        rotation = nBox.rotation;
        halfsize = nBox.halfsize;
        return (*this);
    }

    int GetPosition(const tplane<TYPE> &nPlane) const
    {
        TYPE nDistance = nPlane.Distance(position);
        TYPE nRadiusX = fabs(rotation.rx.Dot(nPlane.normal) * halfsize.x);
        TYPE nRadiusY = fabs(rotation.ry.Dot(nPlane.normal) * halfsize.y);
        TYPE nRadiusZ = fabs(rotation.rz.Dot(nPlane.normal) * halfsize.z);
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
