#pragma once

struct GEKRay
{
public:
    Eigen::Vector3f origin;
    Eigen::Vector3f normal;

public:
    GEKRay(void)
    {
    }

    GEKRay(const Eigen::Vector3f &nOrigin, const Eigen::Vector3f &nNormal)
        : origin(nOrigin)
        , normal(nNormal)
    {
    }

    GEKRay operator = (const GEKRay &nRay)
    {
        origin = nRay.origin;
        normal = nRay.normal;
        return (*this);
    }

    float GetDistance(const GEKOBB &nBox) const
    {
	    float nMin = 0.0f;
	    float nMax = float(100000);
	    Eigen::Vector3f nDelta(nBox.position - origin);
        Eigen::Matrix4Xf nBoxMatrix(nBox.rotation);
        for(int nAxis = 0; nAxis < 3; ++nAxis)
        {
		    Eigen::Vector3f nMatrixAxis(nBoxMatrix.row(nAxis));
		    float nAxisAngle = nMatrixAxis.dot(nDelta);
		    float nRayAngle = normal.dot(nMatrixAxis);
		    if (fabs(nRayAngle) > float(0.001))
            {
                float nDelta1 = ((nAxisAngle - nBox.halfsize[nAxis]) / nRayAngle);
			    float nDelta2 = ((nAxisAngle + nBox.halfsize[nAxis]) / nRayAngle);
			    if (nDelta1 > nDelta2)
                {
				    float nDeltaSwap = nDelta1;
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
				    return float(-1);
                }
		    }
            else
            {
			    if ((-nAxisAngle - nBox.halfsize[nAxis]) > 0.0f ||
                    (-nAxisAngle + nBox.halfsize[nAxis]) < 0.0f)
                {
				    return float(-1);
                }
		    }
	    }

	    return nMin;
    }
};
