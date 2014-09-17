#pragma once

template <typename TYPE>
struct tfrustum
{
public:
    tvector3<TYPE> origin;
    tplane<TYPE> planes[6];

public:
    tfrustum(void)
    {
    }

    void Create(const tmatrix4x4<TYPE> &nMatrix, const tmatrix4x4<TYPE> &nProjection)
    {
        // Extract the origin;
        origin = nMatrix.t;

        tmatrix4x4<TYPE> nView = nMatrix.GetInverse();
        tmatrix4x4<TYPE> nTransform = (nView * nProjection);
        
        // Calculate near plane of frustum.
        planes[0].normal.x = nTransform._14 + nTransform._13;
        planes[0].normal.y = nTransform._24 + nTransform._23;
        planes[0].normal.z = nTransform._34 + nTransform._33;
        planes[0].distance = nTransform._44 + nTransform._43;
        planes[0].Normalize();

        // Calculate far plane of frustum.
        planes[1].normal.x = nTransform._14 - nTransform._13;
        planes[1].normal.y = nTransform._24 - nTransform._23;
        planes[1].normal.z = nTransform._34 - nTransform._33;
        planes[1].distance = nTransform._44 - nTransform._43;
        planes[1].Normalize();

        // Calculate left plane of frustum.
        planes[2].normal.x = nTransform._14 + nTransform._11;
        planes[2].normal.y = nTransform._24 + nTransform._21;
        planes[2].normal.z = nTransform._34 + nTransform._31;
        planes[2].distance = nTransform._44 + nTransform._41;
        planes[2].Normalize();

        // Calculate right plane of frustum.
        planes[3].normal.x = nTransform._14 - nTransform._11;
        planes[3].normal.y = nTransform._24 - nTransform._21;
        planes[3].normal.z = nTransform._34 - nTransform._31;
        planes[3].distance = nTransform._44 - nTransform._41;
        planes[3].Normalize();

        // Calculate top plane of frustum.
        planes[4].normal.x = nTransform._14 - nTransform._12;
        planes[4].normal.y = nTransform._24 - nTransform._22;
        planes[4].normal.z = nTransform._34 - nTransform._32;
        planes[4].distance = nTransform._44 - nTransform._42;
        planes[4].Normalize();

        // Calculate bottom plane of frustum.
        planes[5].normal.x = nTransform._14 + nTransform._12;
        planes[5].normal.y = nTransform._24 + nTransform._22;
        planes[5].normal.z = nTransform._34 + nTransform._32;
        planes[5].distance = nTransform._44 + nTransform._42;
        planes[5].Normalize();
    }

    template <class SHAPE>
    bool IsVisible(const SHAPE &nShape) const
    {
        for (auto &nPlane : planes)
        {
            if (nShape.GetPosition(nPlane) == -1) return false;
        }

	    return true;
    }
};
