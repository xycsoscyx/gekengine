#pragma once

struct float3;

struct obb;

struct ray
{
public:
    float3 origin;
    float3 normal;

public:
    ray(void)
    {
    }

    ray(const float3 &nOrigin, const float3 &nNormal)
        : origin(nOrigin)
        , normal(nNormal)
    {
    }

    ray operator = (const ray &nRay)
    {
        origin = nRay.origin;
        normal = nRay.normal;
        return (*this);
    }

    float GetDistance(const obb &nBox) const
    {
	    float nMin = 0.0f;
	    float nMax = 100000.0f;
	    float3 nDelta(nBox.position - origin);
        float4x4 nBoxMatrix(nBox.rotation);
        float3 nHalfSize(nBox.halfsize);

        for(UINT32 nAxis = 0; nAxis < 3; ++nAxis)
        {
		    float3 nMatrixAxis(nBoxMatrix.r[nAxis]);
		    float nAxisAngle = nMatrixAxis.Dot(nDelta);
		    float nRayAngle = normal.Dot(nMatrixAxis);
		    if (fabs(nRayAngle) > 0.001.0f)
            {
                float nDelta1 = ((nAxisAngle - nHalfSize.xyz[nAxis]) / nRayAngle);
			    float nDelta2 = ((nAxisAngle + nHalfSize.xyz[nAxis]) / nRayAngle);
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
				    return -1.0f;
                }
		    }
            else
            {
			    if ((-nAxisAngle - nHalfSize.xyz[nAxis]) > 0.0f ||
                    (-nAxisAngle + nHalfSize.xyz[nAxis]) < 0.0f)
                {
				    return -1.0f;
                }
		    }
	    }

	    return nMin;
    }
};
