#pragma once

template <typename TYPE>
struct tplane : public tvector4<TYPE>
{
public:
    tplane(void)
    {
        normal.x = TYPE(0);
        normal.y = TYPE(0);
        normal.z = TYPE(0);
        distance = TYPE(0);
    }

    tplane(const tplane<TYPE> &nPlane)
    {
        normal = nPlane.normal;
        distance = nPlane.distance;
    }

    tplane(const tvector4<TYPE> &nPlane)
    {
        normal.x = nPlane.x;
        normal.y = nPlane.y;
        normal.z = nPlane.z;
        distance = nPlane.w;
    }

    tplane(TYPE nA, TYPE nB, TYPE nC, TYPE nD)
    {
        normal.x = nA;
        normal.y = nB;
        normal.z = nC;
        distance = nD;
    }

    tplane(const tvector3<TYPE> &nNormal, TYPE nDistance)
    {
        normal = nNormal;
        distance = nDistance;
    }

    tplane(const tvector3<TYPE> &nPointA, const tvector3<TYPE> &nPointB, const tvector3<TYPE> &nPointC)
    {
        tvector3 kSideA(nPointB - nPointA);
        tvector3 kSideB(nPointC - nPointA);
        tvector3 kCross(kSideA.Cross(kSideB));

        normal = kCross.GetNormal();
        distance = -normal.Dot(nPointA);
    }

    tplane(const tvector3<TYPE> &nNormal, const tvector3<TYPE> &nPointOnPlane)
    {
        normal = nNormal;
        distance = -normal.Dot(nPointOnPlane);
    }

    tplane<TYPE> operator = (const tplane<TYPE> &nPlane)
    {
        normal = nPlane.normal;
        distance = nPlane.distance;
        return *this;
    }

    void Normalize(void)
    {
        TYPE nLength = (TYPE(1) / normal.GetLength());
        normal.x *= nLength;
        normal.y *= nLength;
        normal.z *= nLength;
        distance *= nLength;
    }

    TYPE Distance(const tvector3<TYPE> &nPoint) const
    {
        return (normal.Dot(nPoint) + distance);
    }
};
