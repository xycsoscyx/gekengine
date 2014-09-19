#pragma once

struct GEKFrustum
{
public:
    Eigen::Vector3f origin;
    GEKPlane planes[6];

public:
    GEKFrustum(void)
    {
    }

    void Create(const Eigen::Matrix4Xf &nMatrix, const Eigen::Matrix4Xf &nProjection)
    {
        // Extract the origin;
        origin = nMatrix.row(3);

        Eigen::Matrix4Xf nView = nMatrix.inverse();
        Eigen::Matrix4Xf nTransform = (nView * nProjection);
        
        // Calculate near plane of frustum.
        planes[0].normal[0] = nTransform(0, 3) + nTransform(0, 2);
        planes[0].normal[1] = nTransform(1, 3) + nTransform(1, 2);
        planes[0].normal[2] = nTransform(2, 3) + nTransform(2, 2);
        planes[0].distance  = nTransform(3, 3) + nTransform(3, 2);
        planes[0].Normalize();

        // Calculate far plane of frustum.
        planes[1].normal[0] = nTransform(0, 3) - nTransform(0, 2);
        planes[1].normal[1] = nTransform(1, 3) - nTransform(1, 2);
        planes[1].normal[2] = nTransform(2, 3) - nTransform(2, 2);
        planes[1].distance  = nTransform(3, 3) - nTransform(3, 2);
        planes[1].Normalize();

        // Calculate left plane of frustum.
        planes[2].normal[0] = nTransform(0, 3) + nTransform(0, 0);
        planes[2].normal[1] = nTransform(1, 3) + nTransform(1, 0);
        planes[2].normal[2] = nTransform(2, 3) + nTransform(2, 0);
        planes[2].distance  = nTransform(3, 3) + nTransform(3, 0);
        planes[2].Normalize();

        // Calculate right plane of frustum.
        planes[3].normal[0] = nTransform(0, 3) - nTransform(0, 0);
        planes[3].normal[1] = nTransform(1, 3) - nTransform(1, 0);
        planes[3].normal[2] = nTransform(2, 3) - nTransform(2, 0);
        planes[3].distance  = nTransform(3, 3) - nTransform(3, 0);
        planes[3].Normalize();

        // Calculate top plane of frustum.
        planes[4].normal[0] = nTransform(0, 3) - nTransform(0, 1);
        planes[4].normal[1] = nTransform(1, 3) - nTransform(1, 1);
        planes[4].normal[2] = nTransform(2, 3) - nTransform(2, 1);
        planes[4].distance  = nTransform(3, 3) - nTransform(3, 1);
        planes[4].Normalize();

        // Calculate bottom plane of frustum.
        planes[5].normal[0] = nTransform(0, 3) + nTransform(0, 1);
        planes[5].normal[1] = nTransform(1, 3) + nTransform(1, 1);
        planes[5].normal[2] = nTransform(2, 3) + nTransform(2, 1);
        planes[5].distance  = nTransform(3, 3) + nTransform(3, 1);
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
