#pragma once

template <typename TYPE>
struct taabb
{
public:
    tvector3<TYPE> minimum;
    tvector3<TYPE> maximum;

public:
    taabb(void)
        : minimum( _INFINITY)
        , maximum(-_INFINITY)
    {
    }

    taabb(const taabb<TYPE> &nBox)
        : minimum(nBox.minimum)
        , maximum(nBox.maximum)
    {
    }

    taabb(TYPE nSize)
        : minimum(-(nSize / TYPE(2)))
        , maximum( (nSize / TYPE(2)))
    {
    }

    taabb(const tvector3<TYPE> &nMinimum, const tvector3<TYPE> &nMaximum)
        : minimum(nMinimum)
        , maximum(nMaximum)
    {
    }

    taabb operator = (const taabb<TYPE> &nBox)
    {
        minimum = nBox.minimum;
        maximum = nBox.maximum;
        return (*this);
    }

    void Extend(tvector3<TYPE> &nPoint)
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

    tvector3<TYPE> GetSize(void) const
    {
        return (maximum - minimum);
    }

    tvector3<TYPE> GetCenter(void) const
    {
        return (minimum + (GetSize() / TYPE(2)));
    }

    int GetPosition(const tplane<TYPE> &nPlane) const
    {
        tvector3<TYPE> nMinimum((nPlane.normal.x > TYPE(0) ? maximum.x : minimum.x),
                                (nPlane.normal.y > TYPE(0) ? maximum.y : minimum.y),
                                (nPlane.normal.z > TYPE(0) ? maximum.z : minimum.z));
        if (nPlane.Distance(nMinimum) < TYPE(0))
        {
            return -1;
        }

        tvector3<TYPE> nMaximum((nPlane.normal.x < TYPE(0) ? maximum.x : minimum.x),
                                (nPlane.normal.y < TYPE(0) ? maximum.y : minimum.y),
                                (nPlane.normal.z < TYPE(0) ? maximum.z : minimum.z));
        if (nPlane.Distance(nMaximum) < TYPE(0))
        {
            return 0;
        }

        return 1;
    }
};
