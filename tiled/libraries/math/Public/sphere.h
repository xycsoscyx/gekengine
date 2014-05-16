#pragma once

template <typename TYPE>
struct tsphere
{
public:
    tvector4<TYPE> position;
    TYPE radius;

public:
    tsphere(void)
        : radius(TYPE(0))
    {
    }

    tsphere(const tsphere<TYPE> &nSphere)
        : position(nSphere.position)
        , radius(nSphere.radius)
    {
    }

    tsphere(const tvector4<TYPE> &nPosition, TYPE nRadius)
        : position(nPosition)
        , radius(nRadius)
    {
    }

    tsphere operator = (const tsphere<TYPE> &nSphere)
    {
        position = nBox.position;
        radius = nBox.radius;
        return (*this);
    }

    int GetPosition(const tplane<TYPE> &nPlane) const
    {
        TYPE nDistance = nPlane.Distance(position);
        if (nDistance < -radius)
        {
            return -1;
        }
        else if (nDistance > radius)
        {
            return 1;
        }
        else
        {
            return 0;
        }
    }
};
