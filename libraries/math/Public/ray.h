#pragma once

template <typename TYPE>
struct tvector3;

template <typename TYPE>
struct tobb;

template <typename TYPE>
struct tray
{
public:
    tvector3<TYPE> origin;
    tvector3<TYPE> normal;

public:
    tray(void)
    {
    }

    tray(const tvector3<TYPE> &nOrigin, const tvector3<TYPE> &nNormal)
        : origin(nOrigin)
        , normal(nNormal)
    {
    }

    tray operator = (const tray<TYPE> &nRay)
    {
        origin = nRay.origin;
        normal = nRay.normal;
        return (*this);
    }

    TYPE GetDistance(const tobb<TYPE> &nBox) const
    {
	    TYPE nMin = TYPE(0);
	    TYPE nMax = TYPE(100000);
	    tvector3<TYPE> nDelta(nBox.position - origin);
        tmatrix4x4<TYPE> nBoxMatrix(nBox.rotation);
        tvector3<TYPE> nHalfSize(nBox.size / TYPE(2));
        for(UINT32 nAxis = 0; nAxis < 3; nAxis++)
        {
		    tvector3<TYPE> nMatrixAxis(nBoxMatrix.r[nAxis]);
		    TYPE nAxisAngle = nMatrixAxis.Dot(nDelta);
		    TYPE nRayAngle = normal.Dot(nMatrixAxis);
		    if (fabs(nRayAngle) > TYPE(0.001))
            {
                TYPE nDelta1 = ((nAxisAngle - nHalfSize.xyz[nAxis]) / nRayAngle);
			    TYPE nDelta2 = ((nAxisAngle + nHalfSize.xyz[nAxis]) / nRayAngle);
			    if (nDelta1 > nDelta2)
                {
				    TYPE nDeltaSwap = nDelta1;
                    nDelta1 = nDelta2;
                    nDelta2 = nDeltaSwap;
			    }

			    if (nDelta2 < nMax)
                {
				    nMax = nDelta2;
                }

                if (nDelta1 > nMin)
                {
				    nMin = nDelta1;
                }

			    if (nMax < nMin)
                {
				    return TYPE(-1);
                }
		    }
            else
            {
			    if ((-nAxisAngle - nHalfSize.xyz[nAxis]) > TYPE(0) ||
                   (-nAxisAngle + nHalfSize.xyz[nAxis]) < TYPE(0))
                {
				    return TYPE(-1);
                }
		    }
	    }

	    return nMin;
    }
};
