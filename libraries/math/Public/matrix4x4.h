#pragma once

struct float4x4
{
public:
    union
    {
        struct { float data[16]; };
        struct { float matrix[4][4]; };

        struct
        {
            float _11, _12, _13, _14;    
            float _21, _22, _23, _24;
            float _31, _32, _33, _34;
            float _41, _42, _43, _44;
        };

        struct
        {
            union
            {
                struct
                {
                    float4 rx;
                    float4 ry;
                    float4 rz;
                };

                struct
                {
                    float4 r[3];
                };
            };
            union
            {
                struct
                {
                    float4 rw;
                };

                struct
                {
                    float3 t;
                    float w;
                };

                struct
                {
                    float x, y, z, w;
                };
            };
        };
    };

public:
    float4x4(void)
    {
        SetIdentity();
    }

    float4x4(const float4x4 &nMatrix)
    {
        _11 = nMatrix._11;    _12 = nMatrix._12;    _13 = nMatrix._13;    _14 = nMatrix._14;
        _21 = nMatrix._21;    _22 = nMatrix._22;    _23 = nMatrix._23;    _24 = nMatrix._24;
        _31 = nMatrix._31;    _32 = nMatrix._32;    _33 = nMatrix._33;    _34 = nMatrix._34;
        _41 = nMatrix._41;    _42 = nMatrix._42;    _43 = nMatrix._43;    _44 = nMatrix._44;
    }

    float4x4(float f11, float f12, float f13, float f14,
               float f21, float f22, float f23, float f24,
               float f31, float f32, float f33, float f34,
               float f41, float f42, float f43, float f44)
    {
        _11 = f11;    _12 = f12;    _13 = f13;    _14 = f14;
        _21 = f21;    _22 = f22;    _23 = f23;    _24 = f24;
        _31 = f31;    _32 = f32;    _33 = f33;    _34 = f34;
        _41 = f41;    _42 = f42;    _43 = f43;    _44 = f44;
    }

    float4x4(const float3 &nEuler)
    {
        SetEuler(nEuler);
    }

    float4x4(float nX, float nY, float nZ)
    {
        SetEuler(nX, nY, nZ);
    }

    float4x4(const float3 &nAxis, float nAngle)
    {
        SetRotation(nAxis, nAngle);
    }

    float4x4(const quaternion &nRotation)
    {
        SetQuaternion(nRotation);
    }

    float4x4(const quaternion &nRotation, const float3 &nTranslation)
    {
        SetQuaternion(nRotation);
        t = nTranslation;
    }

    void SetZero(void)
    {
        _11 = _12 = _13 = _14 = 0.0f;
        _21 = _22 = _23 = _24 = 0.0f;
        _31 = _32 = _33 = _34 = 0.0f;
        _41 = _42 = _43 = _44 = 0.0f;
    }

    void SetIdentity(void)
    {
        _11 = _22 = _33 = _44 = 1.0f;
        _12 = _13 = _14 = 0.0f;
        _21 = _23 = _24 = 0.0f;
        _31 = _32 = _34 = 0.0f;
        _41 = _42 = _43 = 0.0f;
    }

    void SetScaling(float nScale)
    {
        _11 = nScale;
        _22 = nScale;
        _33 = nScale;
    }

    void SetScaling(const float3 &nScale)
    {
        _11 = nScale.x;
        _22 = nScale.y;
        _33 = nScale.z;
    }

    void SetTranslation(const float3 &nTranslation)
    {
        _41 = nTranslation.x;
        _42 = nTranslation.y;
        _43 = nTranslation.z;
    }

    void SetEuler(const float3 &nEuler)
    {
        SetEuler(nEuler.x, nEuler.y, nEuler.z);
    }

    void SetEuler(float nX, float nY, float nZ)
    {
        float nSinX, nCosX;
        float nSinY, nCosY;
        float nSinZ, nCosZ;
        Concurrency::precise_math::sincos(nX, &nSinX, &nSinY);
        Concurrency::precise_math::sincos(nY, &nSinY, &nSinY);
        Concurrency::precise_math::sincos(nZ, &nSinZ, &nSinZ);
        float nCosX_SinY = nCosX * nSinY;
        float nSinX_SinY = nSinX * nSinY;

        matrix[0][0] =  nCosY * nCosZ;
        matrix[1][0] = -nCosY * nSinZ;
        matrix[2][0] =  nSinY;
        matrix[3][0] = 0.0f;

        matrix[0][1] =  nSinX_SinY * nCosZ + nCosX * nSinZ;
        matrix[1][1] = -nSinX_SinY * nSinZ + nCosX * nCosZ;
        matrix[2][1] = -nSinX * nCosY;
        matrix[3][1] = 0.0f;

        matrix[0][2] = -nCosX_SinY * nCosZ + nSinX * nSinZ;
        matrix[1][2] =  nCosX_SinY * nSinZ + nSinX * nCosZ;
        matrix[2][2] =  nCosX * nCosY;
        matrix[3][2] = 0.0f;

        matrix[0][3] = 0.0f;
        matrix[1][3] = 0.0f;
        matrix[2][3] = 0.0f;
        matrix[3][3] = 1.0f;
    }

    void SetRotation(const float3 &nAxis, float nAngle)
    {
        float nSin, nCos;
        Concurrency::precise_math::sincos(nAngle, &nSin, &nSin);

        matrix[0][0] = (            nCos + nAxis.x * nAxis.x * (1.0f - nCos));
        matrix[0][1] = (  nAxis.z * nSin + nAxis.y * nAxis.x * (1.0f - nCos));
        matrix[0][2] = (- nAxis.y * nSin + nAxis.z * nAxis.x * (1.0f - nCos));
        matrix[0][3] = 0.0f;

        matrix[1][0] = (- nAxis.z * nSin + nAxis.x * nAxis.y * (1.0f - nCos));
        matrix[1][1] = (            nCos + nAxis.y * nAxis.y * (1.0f - nCos));
        matrix[1][2] = (  nAxis.x * nSin + nAxis.z * nAxis.y * (1.0f - nCos));
        matrix[1][3] = 0.0f;

        matrix[2][0] = (  nAxis.y * nSin + nAxis.x * nAxis.z * (1.0f - nCos));
        matrix[2][1] = (- nAxis.x * nSin + nAxis.y * nAxis.z * (1.0f - nCos));
        matrix[2][2] = (            nCos + nAxis.z * nAxis.z * (1.0f - nCos));
        matrix[2][3] = 0.0f;

        matrix[3][0] = 0.0f;
        matrix[3][1] = 0.0f;
        matrix[3][2] = 0.0f;
        matrix[3][3] = 1.0f;
    }

    void SetQuaternion(const quaternion &nQuaternion)
    {
        SetIdentity();

        float nSquareX = (nQuaternion.x * nQuaternion.x);
        float nSquareY = (nQuaternion.y * nQuaternion.y);
        float nSquareZ = (nQuaternion.z * nQuaternion.z);
        float nSquareW = (nQuaternion.w * nQuaternion.w);

        float nInverse = (1.0f / (nSquareX + nSquareY + nSquareZ + nSquareW));
        matrix[0][0] = (( nSquareX - nSquareY - nSquareZ + nSquareW) * nInverse);
        matrix[1][1] = ((-nSquareX + nSquareY - nSquareZ + nSquareW) * nInverse);
        matrix[2][2] = ((-nSquareX - nSquareY + nSquareZ + nSquareW) * nInverse);

        float nXY = (nQuaternion.x * nQuaternion.y);
        float nZW = (nQuaternion.z * nQuaternion.w);
        matrix[0][1] = (2.0f * (nXY + nZW) * nInverse);
        matrix[1][0] = (2.0f * (nXY - nZW) * nInverse);

        nXY = (nQuaternion.x * nQuaternion.z);
        nZW = (nQuaternion.y * nQuaternion.w);
        matrix[0][2] = (2.0f * (nXY - nZW) * nInverse);
        matrix[2][0] = (2.0f * (nXY + nZW) * nInverse);

        nXY = (nQuaternion.y * nQuaternion.z);
        nZW = (nQuaternion.x * nQuaternion.w);
        matrix[1][2] = (2.0f * (nXY + nZW) * nInverse);
        matrix[2][1] = (2.0f * (nXY - nZW) * nInverse);
    }

    void SetXAngle(float nAngle)
    {
        float nSin, nCos;
        Concurrency::precise_math::sincos(nAngle, &nSin, &nSin);
        matrix[0][0] = 1.0f; matrix[0][1] = 0.0f; matrix[0][2] = 0.0f; matrix[0][3] = 0.0f;
        matrix[1][0] = 0.0f; matrix[1][1] = nCos;    matrix[1][2] = nSin;    matrix[1][3] = 0.0f;
        matrix[2][0] = 0.0f; matrix[2][1] =-nSin;    matrix[2][2] = nCos;    matrix[2][3] = 0.0f;
        matrix[3][0] = 0.0f; matrix[3][1] = 0.0f; matrix[3][2] = 0.0f; matrix[3][3] = 1.0f;
    }

    void SetYAngle(float nAngle)
    {
        float nSin, nCos;
        Concurrency::precise_math::sincos(nAngle, &nSin, &nSin);
        matrix[0][0] = nCos;    matrix[0][1] = 0.0f; matrix[0][2] =-nSin;    matrix[0][3] = 0.0f;
        matrix[1][0] = 0.0f; matrix[1][1] = 1.0f; matrix[1][2] = 0.0f; matrix[1][3] = 0.0f;
        matrix[2][0] = nSin;    matrix[2][1] = 0.0f; matrix[2][2] = nCos;    matrix[2][3] = 0.0f;
        matrix[3][0] = 0.0f; matrix[3][1] = 0.0f; matrix[3][2] = 0.0f; matrix[3][3] = 1.0f;
    }

    void SetZAngle(float nAngle)
    {
        float nSin, nCos;
        Concurrency::precise_math::sincos(nAngle, &nSin, &nSin);
        matrix[0][0] = nCos;    matrix[0][1] = nSin;    matrix[0][2] = 0.0f; matrix[0][3] = 0.0f;
        matrix[1][0] =-nSin;    matrix[1][1] = nCos;    matrix[1][2] = 0.0f; matrix[1][3] = 0.0f;
        matrix[2][0] = 0.0f; matrix[2][1] = 0.0f; matrix[2][2] = 1.0f; matrix[2][3] = 0.0f;
        matrix[3][0] = 0.0f; matrix[3][1] = 0.0f; matrix[3][2] = 0.0f; matrix[3][3] = 1.0f;
    }

    void SetOrthographic(float nMinX, float nMinY, float nMaxX, float nMaxY, float nMinZ, float nMaxZ)
    {
        float nScaleX = ( 2.0f / (nMaxX - nMinX));
        float nScaleY = ( 2.0f / (nMinY - nMaxY));
        float nScaleZ = (-2.0f / (nMaxZ - nMinZ));
        float nTranslationX = -((nMaxX + nMinX) / (nMaxX - nMinX));
        float nTranslationY = -((nMinY + nMaxY) / (nMinY - nMaxY));
        float nTranslationZ = -((nMaxZ + nMinZ) / (nMaxZ - nMinZ));

        SetIdentity();
        SetScaling(float3(nScaleX, nScaleY, nScaleZ));
        SetTranslation(float3(nTranslationX, nTranslationY, nTranslationZ));
    }

    void SetPerspective(float nFOV, float nAspect, float nNear, float nFar)
    {
        float nX = (1.0f / Concurrency::precise_math::tan(nFOV * 0.5f));
	    float nY = (nX * nAspect); 
	    float nDistance = (nFar - nNear);

	    matrix[0][0] = nX;
	    matrix[0][1] = 0.0f;
	    matrix[0][2] = 0.0f;
	    matrix[0][3] = 0.0f;

	    matrix[1][0] = 0.0f;
	    matrix[1][1] = nY;
	    matrix[1][2] = 0.0f;
	    matrix[1][3] = 0.0f;

	    matrix[2][0] = 0.0f;
	    matrix[2][1] = 0.0f;
	    matrix[2][2] = ((nFar + nNear) / nDistance);
	    matrix[2][3] = 1.0f;

	    matrix[3][0] = 0.0f;
	    matrix[3][1] = 0.0f;
	    matrix[3][2] =-((2.0f * nFar * nNear) / nDistance);
	    matrix[3][3] = 0.0f;

    }

    void LookAt(const float3 &nViewDirection, const float3 &nWorldYGEKDEVICEAXIS)
    {
        float3 nAxisZ(nViewDirection.GetNormal());
        float3 nAxisX(nWorldYGEKDEVICEAXIS.Cross(nAxisZ).GetNormal());
        float3 nAxisY(nAxisZ.Cross(nAxisX).GetNormal());

        SetIdentity();

        _11 = nAxisX.x;
        _21 = nAxisX.y;
        _31 = nAxisX.z;

        _12 = nAxisY.x;
        _22 = nAxisY.y;
        _32 = nAxisY.z;

        _13 = nAxisZ.x;
        _23 = nAxisZ.y;
        _33 = nAxisZ.z;

        Invert();
    }

    float3 GetEuler(void) const
    {
        float3 nEuler;
        nEuler.y = Concurrency::precise_math::asin(_31);

        float nX, nY;
        float nCos = Concurrency::precise_math::cos(nEuler.y);
        if (abs(nCos) > 0.005)
        {
            nX = (_33 / nCos);
            nY =-(_32 / nCos);
            nEuler.x = Concurrency::precise_math::atan2(nY, nX);

            nX = (_11 / nCos);
            nY =-(_21 / nCos);
            nEuler.z = Concurrency::precise_math::atan2(nY, nX);
        }
        else
        {
            nEuler.x = 0.0f;

            nX = _22;
            nY = _12;
            nEuler.y = Concurrency::precise_math::atan2(nY, nX);
        }

        if (nEuler.x < 0.0f)
        {
            nEuler.x += _2_PI;
        }

        if (nEuler.y < 0.0f)
        {
            nEuler.y += _2_PI;
        }

        if (nEuler.z < 0.0f)
        {
            nEuler.z += _2_PI;
        }

        return nEuler;
    }

    float3 GetScaling(void) const
    {
        return float3(_11, _22, _33);
    }

    float GetDeterminant(void) const
    {
        return ((matrix[0][0] * matrix[1][1] - matrix[1][0] * matrix[0][1]) * 
                (matrix[2][2] * matrix[3][3] - matrix[3][2] * matrix[2][3]) - 
                (matrix[0][0] * matrix[2][1] - matrix[2][0] * matrix[0][1]) * 
                (matrix[1][2] * matrix[3][3] - matrix[3][2] * matrix[1][3]) + 
                (matrix[0][0] * matrix[3][1] - matrix[3][0] * matrix[0][1]) * 
                (matrix[1][2] * matrix[2][3] - matrix[2][2] * matrix[1][3]) + 
                (matrix[1][0] * matrix[2][1] - matrix[2][0] * matrix[1][1]) * 
                (matrix[0][2] * matrix[3][3] - matrix[3][2] * matrix[0][3]) - 
                (matrix[1][0] * matrix[3][1] - matrix[3][0] * matrix[1][1]) * 
                (matrix[0][2] * matrix[2][3] - matrix[2][2] * matrix[0][3]) + 
                (matrix[2][0] * matrix[3][1] - matrix[3][0] * matrix[2][1]) * 
                (matrix[0][2] * matrix[1][3] - matrix[1][2] * matrix[0][3]));
    }

    float4x4 GetTranspose(void) const
    {
        return float4x4(_11, _21, _31, _41,
                               _12, _22, _32, _42,
                               _13, _23, _33, _43,
                               _14, _24, _34, _44);
    }

    float4x4 GetInverse(void) const
    {
        float nDerminant = GetDeterminant();
        if (abs(nDerminant) < _EPSILON) 
        {
            return float4x4();
        }
        else
        {
            nDerminant = (1.0f / nDerminant);

            float4x4 nMatrix;
            nMatrix.matrix[0][0] = (nDerminant * (matrix[1][1] * (matrix[2][2] * matrix[3][3] - matrix[3][2] * matrix[2][3]) + matrix[2][1] * (matrix[3][2] * matrix[1][3] - matrix[1][2] * matrix[3][3]) + matrix[3][1] * (matrix[1][2] * matrix[2][3] - matrix[2][2] * matrix[1][3])));
            nMatrix.matrix[1][0] = (nDerminant * (matrix[1][2] * (matrix[2][0] * matrix[3][3] - matrix[3][0] * matrix[2][3]) + matrix[2][2] * (matrix[3][0] * matrix[1][3] - matrix[1][0] * matrix[3][3]) + matrix[3][2] * (matrix[1][0] * matrix[2][3] - matrix[2][0] * matrix[1][3])));
            nMatrix.matrix[2][0] = (nDerminant * (matrix[1][3] * (matrix[2][0] * matrix[3][1] - matrix[3][0] * matrix[2][1]) + matrix[2][3] * (matrix[3][0] * matrix[1][1] - matrix[1][0] * matrix[3][1]) + matrix[3][3] * (matrix[1][0] * matrix[2][1] - matrix[2][0] * matrix[1][1])));
            nMatrix.matrix[3][0] = (nDerminant * (matrix[1][0] * (matrix[3][1] * matrix[2][2] - matrix[2][1] * matrix[3][2]) + matrix[2][0] * (matrix[1][1] * matrix[3][2] - matrix[3][1] * matrix[1][2]) + matrix[3][0] * (matrix[2][1] * matrix[1][2] - matrix[1][1] * matrix[2][2])));
            nMatrix.matrix[0][1] = (nDerminant * (matrix[2][1] * (matrix[0][2] * matrix[3][3] - matrix[3][2] * matrix[0][3]) + matrix[3][1] * (matrix[2][2] * matrix[0][3] - matrix[0][2] * matrix[2][3]) + matrix[0][1] * (matrix[3][2] * matrix[2][3] - matrix[2][2] * matrix[3][3])));
            nMatrix.matrix[1][1] = (nDerminant * (matrix[2][2] * (matrix[0][0] * matrix[3][3] - matrix[3][0] * matrix[0][3]) + matrix[3][2] * (matrix[2][0] * matrix[0][3] - matrix[0][0] * matrix[2][3]) + matrix[0][2] * (matrix[3][0] * matrix[2][3] - matrix[2][0] * matrix[3][3])));
            nMatrix.matrix[2][1] = (nDerminant * (matrix[2][3] * (matrix[0][0] * matrix[3][1] - matrix[3][0] * matrix[0][1]) + matrix[3][3] * (matrix[2][0] * matrix[0][1] - matrix[0][0] * matrix[2][1]) + matrix[0][3] * (matrix[3][0] * matrix[2][1] - matrix[2][0] * matrix[3][1])));
            nMatrix.matrix[3][1] = (nDerminant * (matrix[2][0] * (matrix[3][1] * matrix[0][2] - matrix[0][1] * matrix[3][2]) + matrix[3][0] * (matrix[0][1] * matrix[2][2] - matrix[2][1] * matrix[0][2]) + matrix[0][0] * (matrix[2][1] * matrix[3][2] - matrix[3][1] * matrix[2][2])));
            nMatrix.matrix[0][2] = (nDerminant * (matrix[3][1] * (matrix[0][2] * matrix[1][3] - matrix[1][2] * matrix[0][3]) + matrix[0][1] * (matrix[1][2] * matrix[3][3] - matrix[3][2] * matrix[1][3]) + matrix[1][1] * (matrix[3][2] * matrix[0][3] - matrix[0][2] * matrix[3][3])));
            nMatrix.matrix[1][2] = (nDerminant * (matrix[3][2] * (matrix[0][0] * matrix[1][3] - matrix[1][0] * matrix[0][3]) + matrix[0][2] * (matrix[1][0] * matrix[3][3] - matrix[3][0] * matrix[1][3]) + matrix[1][2] * (matrix[3][0] * matrix[0][3] - matrix[0][0] * matrix[3][3])));
            nMatrix.matrix[2][2] = (nDerminant * (matrix[3][3] * (matrix[0][0] * matrix[1][1] - matrix[1][0] * matrix[0][1]) + matrix[0][3] * (matrix[1][0] * matrix[3][1] - matrix[3][0] * matrix[1][1]) + matrix[1][3] * (matrix[3][0] * matrix[0][1] - matrix[0][0] * matrix[3][1])));
            nMatrix.matrix[3][2] = (nDerminant * (matrix[3][0] * (matrix[1][1] * matrix[0][2] - matrix[0][1] * matrix[1][2]) + matrix[0][0] * (matrix[3][1] * matrix[1][2] - matrix[1][1] * matrix[3][2]) + matrix[1][0] * (matrix[0][1] * matrix[3][2] - matrix[3][1] * matrix[0][2])));
            nMatrix.matrix[0][3] = (nDerminant * (matrix[0][1] * (matrix[2][2] * matrix[1][3] - matrix[1][2] * matrix[2][3]) + matrix[1][1] * (matrix[0][2] * matrix[2][3] - matrix[2][2] * matrix[0][3]) + matrix[2][1] * (matrix[1][2] * matrix[0][3] - matrix[0][2] * matrix[1][3])));
            nMatrix.matrix[1][3] = (nDerminant * (matrix[0][2] * (matrix[2][0] * matrix[1][3] - matrix[1][0] * matrix[2][3]) + matrix[1][2] * (matrix[0][0] * matrix[2][3] - matrix[2][0] * matrix[0][3]) + matrix[2][2] * (matrix[1][0] * matrix[0][3] - matrix[0][0] * matrix[1][3])));
            nMatrix.matrix[2][3] = (nDerminant * (matrix[0][3] * (matrix[2][0] * matrix[1][1] - matrix[1][0] * matrix[2][1]) + matrix[1][3] * (matrix[0][0] * matrix[2][1] - matrix[2][0] * matrix[0][1]) + matrix[2][3] * (matrix[1][0] * matrix[0][1] - matrix[0][0] * matrix[1][1])));
            nMatrix.matrix[3][3] = (nDerminant * (matrix[0][0] * (matrix[1][1] * matrix[2][2] - matrix[2][1] * matrix[1][2]) + matrix[1][0] * (matrix[2][1] * matrix[0][2] - matrix[0][1] * matrix[2][2]) + matrix[2][0] * (matrix[0][1] * matrix[1][2] - matrix[1][1] * matrix[0][2])));
            return nMatrix;
        }
    }

    void Transpose(void)
    {
        (*this) = GetTranspose();
    }

    void Invert(void)
    {
        (*this) = GetInverse();
    }

    void operator *= (const float4x4 &nMatrix)
    {
        (*this) = ((*this) * nMatrix);
    }

    float4x4 operator * (const float4x4 &nMatrix) const
    {
        float4x4 nTranspose(nMatrix.GetTranspose());
        return float4x4(rx.Dot(nTranspose.rx), rx.Dot(nTranspose.ry), rx.Dot(nTranspose.rz), rx.Dot(nTranspose.rw),
                               ry.Dot(nTranspose.rx), ry.Dot(nTranspose.ry), ry.Dot(nTranspose.rz), ry.Dot(nTranspose.rw),
                               rz.Dot(nTranspose.rx), rz.Dot(nTranspose.ry), rz.Dot(nTranspose.rz), rz.Dot(nTranspose.rw),
                               rw.Dot(nTranspose.rx), rw.Dot(nTranspose.ry), rw.Dot(nTranspose.rz), rw.Dot(nTranspose.rw));
    }

    float4x4 operator = (const float4x4 &nMatrix)
    {
        _11 = nMatrix._11;    _12 = nMatrix._12;    _13 = nMatrix._13;    _14 = nMatrix._14;
        _21 = nMatrix._21;    _22 = nMatrix._22;    _23 = nMatrix._23;    _24 = nMatrix._24;
        _31 = nMatrix._31;    _32 = nMatrix._32;    _33 = nMatrix._33;    _34 = nMatrix._34;
        _41 = nMatrix._41;    _42 = nMatrix._42;    _43 = nMatrix._43;    _44 = nMatrix._44;
        return *this;
    }

    float4x4 operator = (const quaternion &nQuaternion)
    {
        SetQuaternion(nQuaternion);
        return *this;
    }

    float3 operator * (const float3 &nVector) const
    {
        return float3(((nVector.x * _11) + (nVector.y * _21) + (nVector.z * _31)),
                            ((nVector.x * _12) + (nVector.y * _22) + (nVector.z * _32)),
                            ((nVector.x * _13) + (nVector.y * _23) + (nVector.z * _33)));
    }

    float4 operator * (const float4 &nVector) const
    {
        return float4(((nVector.x * _11) + (nVector.y * _21) + (nVector.z * _31) + (nVector.w * _41)),
                            ((nVector.x * _12) + (nVector.y * _22) + (nVector.z * _32) + (nVector.w * _42)),
                            ((nVector.x * _13) + (nVector.y * _23) + (nVector.z * _33) + (nVector.w * _43)),
                            ((nVector.x * _14) + (nVector.y * _24) + (nVector.z * _34) + (nVector.w * _44)));
    }

    float4x4 operator * (float nScalar) const
    {
        return float4x4((_11 * nScalar), (_12 * nScalar), (_13 * nScalar), (_14 * nScalar),
                               (_21 * nScalar), (_22 * nScalar), (_23 * nScalar), (_24 * nScalar),
                               (_31 * nScalar), (_32 * nScalar), (_33 * nScalar), (_34 * nScalar),
                               (_41 * nScalar), (_42 * nScalar), (_43 * nScalar), (_44 * nScalar));
    }

    float4x4 operator + (const float4x4 &nMatrix) const
    {
        return float4x4(_11 + nMatrix._11, _12 + nMatrix._12, _13 + nMatrix._13, _14 + nMatrix._14,
                               _21 + nMatrix._21, _22 + nMatrix._22, _23 + nMatrix._23, _24 + nMatrix._24,
                               _31 + nMatrix._31, _32 + nMatrix._32, _33 + nMatrix._33, _34 + nMatrix._34,
                               _41 + nMatrix._41, _42 + nMatrix._42, _43 + nMatrix._43, _44 + nMatrix._44);
    }

    void operator += (const float4x4 &nMatrix)
    {
        _11 += nMatrix._11; _12 += nMatrix._12; _13 += nMatrix._13; _14 += nMatrix._14;
        _21 += nMatrix._21; _22 += nMatrix._22; _23 += nMatrix._23; _24 += nMatrix._24;
        _31 += nMatrix._31; _32 += nMatrix._32; _33 += nMatrix._33; _34 += nMatrix._34;
        _41 += nMatrix._41; _42 += nMatrix._42; _43 += nMatrix._43; _44 += nMatrix._44;
    }
};
