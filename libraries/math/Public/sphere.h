#pragma once

template <typename TYPE>
struct taabb;

template <typename TYPE>
struct tobb;

template <typename TYPE>
struct tplane;

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

    bool Contains(const tvector4<TYPE> &nPoint) const
    {
        return ((nPoint - position).GetLength() <= radius);
    }

    bool Contains(const tsphere<TYPE> &nSphere) const
    {
        return ((nSphere.position - position).GetLength() <= (nSphere.radius + radius));
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
