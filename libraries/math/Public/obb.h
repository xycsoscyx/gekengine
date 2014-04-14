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

    bool Contains(const tvector4<TYPE> &nPoint) const
    {
        return false;
    }

    bool Contains(const tsphere<TYPE> &nSphere) const
    {
        return false;
    }

    bool Contains(const taabb<TYPE> &nBox) const
    {
        return false;
    }

    bool Contains(const tobb<TYPE> &nBox) const
    {
        return false;
    }
};
