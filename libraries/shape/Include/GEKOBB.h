#pragma once

struct GEKOBB
{
public:
    Eigen::Vector3f position;
    Eigen::Matrix3Xf rotation;
    Eigen::Vector3f halfsize;

public:
    GEKOBB(void)
    {
    }

    GEKOBB(const GEKOBB &nBox)
        : position(nBox.position)
        , rotation(nBox.rotation)
        , halfsize(nBox.halfsize)
    {
    }

    GEKOBB(const GEKAABB &nBox, const Eigen::Quaternionf &nRotation, const Eigen::Vector3f &nPosition)
    {
        rotation = nRotation.toRotationMatrix();
        position = (nPosition + nBox.getCenter());
        halfsize = (nBox.getSize() * 0.5f);
    }

    GEKOBB(const GEKAABB &nBox, const Eigen::Matrix4Xf &nMatrix)
    {
        rotation = nMatrix;
        position = nBox.getCenter();
        position += nMatrix.row(3);
        halfsize = (nBox.getSize() * 0.5f);
    }

    GEKOBB operator = (const GEKOBB &nBox)
    {
        position = nBox.position;
        rotation = nBox.rotation;
        halfsize = nBox.halfsize;
        return (*this);
    }

    int GetPosition(const GEKPlane &nPlane) const
    {
        float nDistance = nPlane.Distance(position);
        float nRadiusX = fabs(rotation.row(0).dot(nPlane.normal) * halfsize[0]);
        float nRadiusY = fabs(rotation.row(1).dot(nPlane.normal) * halfsize[1]);
        float nRadiusZ = fabs(rotation.row(2).dot(nPlane.normal) * halfsize[2]);
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
