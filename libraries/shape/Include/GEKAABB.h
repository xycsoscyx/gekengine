#pragma once

struct GEKAABB
{
public:
    Eigen::Vector3f minimum;
    Eigen::Vector3f maximum;

public:
    GEKAABB(void)
        : minimum(_INFINITY, _INFINITY, _INFINITY)
        , maximum(-_INFINITY, -_INFINITY, -_INFINITY)
    {
    }

    GEKAABB(const GEKAABB &nBox)
        : minimum(nBox.minimum)
        , maximum(nBox.maximum)
    {
    }

    GEKAABB(const Eigen::Vector3f &nMinimum, const Eigen::Vector3f &nMaximum)
        : minimum(nMinimum)
        , maximum(nMaximum)
    {
    }

    GEKAABB operator = (const GEKAABB &nBox)
    {
        minimum = nBox.minimum;
        maximum = nBox.maximum;
        return (*this);
    }

    void Extend(Eigen::Vector3f &nPoint)
    {
        if (nPoint[0] < minimum[0])
        {
            minimum[0] = nPoint[0];
        }

        if (nPoint[1] < minimum[1])
        {
            minimum[1] = nPoint[1];
        }

        if (nPoint[2] < minimum[2])
        {
            minimum[2] = nPoint[2];
        }

        if (nPoint[0] > maximum[0])
        {
            maximum[0] = nPoint[0];
        }

        if (nPoint[1] > maximum[1])
        {
            maximum[1] = nPoint[1];
        }

        if (nPoint[2] > maximum[2])
        {
            maximum[2] = nPoint[2];
        }
    }

    Eigen::Vector3f GetSize(void) const
    {
        return (maximum - minimum);
    }

    Eigen::Vector3f GetCenter(void) const
    {
        return (minimum + (GetSize() * 0.5f));
    }

    int GetPosition(const GEKPlane &nPlane) const
    {
        Eigen::Vector3f nMinimum((nPlane.normal[0] > 0.0f ? maximum[0] : minimum[0]),
                                (nPlane.normal[1] > 0.0f ? maximum[1] : minimum[1]),
                                (nPlane.normal[2] > 0.0f ? maximum[2] : minimum[2]));
        if (nPlane.Distance(nMinimum) < 0.0f)
        {
            return -1;
        }

        Eigen::Vector3f nMaximum((nPlane.normal[0] < 0.0f ? maximum[0] : minimum[0]),
                                (nPlane.normal[1] < 0.0f ? maximum[1] : minimum[1]),
                                (nPlane.normal[2] < 0.0f ? maximum[2] : minimum[2]));
        if (nPlane.Distance(nMaximum) < 0.0f)
        {
            return 0;
        }

        return 1;
    }
};
