#pragma once

template <typename TYPE>
struct tmatrix4x4
{
public:
    union
    {
        struct { TYPE data[16]; };
        struct { TYPE matrix[4][4]; };

        struct
        {
            TYPE _11, _12, _13, _14;    
            TYPE _21, _22, _23, _24;
            TYPE _31, _32, _33, _34;
            TYPE _41, _42, _43, _44;
        };

        struct
        {
            union
            {
                struct
                {
                    tvector4<TYPE> rx;
                    tvector4<TYPE> ry;
                    tvector4<TYPE> rz;
                };

                struct
                {
                    tvector4<TYPE> r[3];
                };
            };
            union
            {
                struct
                {
                    tvector4<TYPE> rw;
                };

                struct
                {
                    tvector3<TYPE> t;
                    TYPE w;
                };

                struct
                {
                    TYPE x, y, z, w;
                };
            };
        };
    };

public:
    tmatrix4x4(void)
    {
        SetIdentity();
    }

    tmatrix4x4(const tmatrix4x4<TYPE> &nMatrix)
    {
        _11 = nMatrix._11;    _12 = nMatrix._12;    _13 = nMatrix._13;    _14 = nMatrix._14;
        _21 = nMatrix._21;    _22 = nMatrix._22;    _23 = nMatrix._23;    _24 = nMatrix._24;
        _31 = nMatrix._31;    _32 = nMatrix._32;    _33 = nMatrix._33;    _34 = nMatrix._34;
        _41 = nMatrix._41;    _42 = nMatrix._42;    _43 = nMatrix._43;    _44 = nMatrix._44;
    }

    tmatrix4x4(TYPE f11, TYPE f12, TYPE f13, TYPE f14,
               TYPE f21, TYPE f22, TYPE f23, TYPE f24,
               TYPE f31, TYPE f32, TYPE f33, TYPE f34,
               TYPE f41, TYPE f42, TYPE f43, TYPE f44)
    {
        _11 = f11;    _12 = f12;    _13 = f13;    _14 = f14;
        _21 = f21;    _22 = f22;    _23 = f23;    _24 = f24;
        _31 = f31;    _32 = f32;    _33 = f33;    _34 = f34;
        _41 = f41;    _42 = f42;    _43 = f43;    _44 = f44;
    }

    tmatrix4x4(const tvector3<TYPE> &nEuler)
    {
        SetEuler(nEuler);
    }

    tmatrix4x4(TYPE nX, TYPE nY, TYPE nZ)
    {
        SetEuler(nX, nY, nZ);
    }

    tmatrix4x4(const tvector3<TYPE> &nGEKDEVICEAXIS, TYPE nAngle)
    {
        SetRotation(nGEKDEVICEAXIS, nAngle);
    }

    tmatrix4x4(const tquaternion<TYPE> &nRotation)
    {
        SetQuaternion(nRotation);
    }

    tmatrix4x4(const tquaternion<TYPE> &nRotation, const tvector3<TYPE> &nTranslation)
    {
        SetQuaternion(nRotation);
        t = nTranslation;
    }

    void SetZero(void)
    {
        _11 = _12 = _13 = _14 = TYPE(0);
        _21 = _22 = _23 = _24 = TYPE(0);
        _31 = _32 = _33 = _34 = TYPE(0);
        _41 = _42 = _43 = _44 = TYPE(0);
    }

    void SetIdentity(void)
    {
        _11 = _22 = _33 = _44 = TYPE(1);
        _12 = _13 = _14 = TYPE(0);
        _21 = _23 = _24 = TYPE(0);
        _31 = _32 = _34 = TYPE(0);
        _41 = _42 = _43 = TYPE(0);
    }

    void SetScaling(TYPE nScale)
    {
        _11 = nScale;
        _22 = nScale;
        _33 = nScale;
    }

    void SetScaling(const tvector3<TYPE> &nScale)
    {
        _11 = nScale.x;
        _22 = nScale.y;
        _33 = nScale.z;
    }

    void SetTranslation(const tvector3<TYPE> &nTranslation)
    {
        _41 = nTranslation.x;
        _42 = nTranslation.y;
        _43 = nTranslation.z;
    }

    void SetEuler(const tvector3<TYPE> &nEuler)
    {
        SetEuler(nEuler.x, nEuler.y, nEuler.z);
    }

    void SetEuler(TYPE nX, TYPE nY, TYPE nZ)
    {
        TYPE nCosX     = cos(nX);
        TYPE nSinX     = sin(nX);
        TYPE nCosY     = cos(nY);
        TYPE nSinY     = sin(nY);
        TYPE nCosZ     = cos(nZ);
        TYPE nSinZ     = sin(nZ);
        TYPE nCosX_SinY = nCosX * nSinY;
        TYPE nSinX_SinY = nSinX * nSinY;

        matrix[0][0] =  nCosY * nCosZ;
        matrix[1][0] = -nCosY * nSinZ;
        matrix[2][0] =  nSinY;
        matrix[3][0] = TYPE(0);

        matrix[0][1] =  nSinX_SinY * nCosZ + nCosX * nSinZ;
        matrix[1][1] = -nSinX_SinY * nSinZ + nCosX * nCosZ;
        matrix[2][1] = -nSinX * nCosY;
        matrix[3][1] = TYPE(0);

        matrix[0][2] = -nCosX_SinY * nCosZ + nSinX * nSinZ;
        matrix[1][2] =  nCosX_SinY * nSinZ + nSinX * nCosZ;
        matrix[2][2] =  nCosX * nCosY;
        matrix[3][2] = TYPE(0);

        matrix[0][3] = TYPE(0);
        matrix[1][3] = TYPE(0);
        matrix[2][3] = TYPE(0);
        matrix[3][3] = TYPE(1);
    }

    void SetRotation(const tvector3<TYPE> &nGEKDEVICEAXIS, TYPE nAngle)
    {
        TYPE nCos = cos(nAngle);
        TYPE nSin = sin(nAngle);

        matrix[0][0] = (            nCos + nGEKDEVICEAXIS.x * nGEKDEVICEAXIS.x * (TYPE(1) - nCos));
        matrix[0][1] = (  nGEKDEVICEAXIS.z * nSin + nGEKDEVICEAXIS.y * nGEKDEVICEAXIS.x * (TYPE(1) - nCos));
        matrix[0][2] = (- nGEKDEVICEAXIS.y * nSin + nGEKDEVICEAXIS.z * nGEKDEVICEAXIS.x * (TYPE(1) - nCos));
        matrix[0][3] = TYPE(0);

        matrix[1][0] = (- nGEKDEVICEAXIS.z * nSin + nGEKDEVICEAXIS.x * nGEKDEVICEAXIS.y * (TYPE(1) - nCos));
        matrix[1][1] = (            nCos + nGEKDEVICEAXIS.y * nGEKDEVICEAXIS.y * (TYPE(1) - nCos));
        matrix[1][2] = (  nGEKDEVICEAXIS.x * nSin + nGEKDEVICEAXIS.z * nGEKDEVICEAXIS.y * (TYPE(1) - nCos));
        matrix[1][3] = TYPE(0);

        matrix[2][0] = (  nGEKDEVICEAXIS.y * nSin + nGEKDEVICEAXIS.x * nGEKDEVICEAXIS.z * (TYPE(1) - nCos));
        matrix[2][1] = (- nGEKDEVICEAXIS.x * nSin + nGEKDEVICEAXIS.y * nGEKDEVICEAXIS.z * (TYPE(1) - nCos));
        matrix[2][2] = (            nCos + nGEKDEVICEAXIS.z * nGEKDEVICEAXIS.z * (TYPE(1) - nCos));
        matrix[2][3] = TYPE(0);

        matrix[3][0] = TYPE(0);
        matrix[3][1] = TYPE(0);
        matrix[3][2] = TYPE(0);
        matrix[3][3] = TYPE(1);
    }

    void SetQuaternion(const tquaternion<TYPE> &nQuaternion)
    {
        SetIdentity();

        TYPE nSquareX = (nQuaternion.x * nQuaternion.x);
        TYPE nSquareY = (nQuaternion.y * nQuaternion.y);
        TYPE nSquareZ = (nQuaternion.z * nQuaternion.z);
        TYPE nSquareW = (nQuaternion.w * nQuaternion.w);

        TYPE nInverse = (TYPE(1) / (nSquareX + nSquareY + nSquareZ + nSquareW));
        matrix[0][0] = (( nSquareX - nSquareY - nSquareZ + nSquareW) * nInverse);
        matrix[1][1] = ((-nSquareX + nSquareY - nSquareZ + nSquareW) * nInverse);
        matrix[2][2] = ((-nSquareX - nSquareY + nSquareZ + nSquareW) * nInverse);

        TYPE nXY = (nQuaternion.x * nQuaternion.y);
        TYPE nZW = (nQuaternion.z * nQuaternion.w);
        matrix[0][1] = (TYPE(2) * (nXY + nZW) * nInverse);
        matrix[1][0] = (TYPE(2) * (nXY - nZW) * nInverse);

        nXY = (nQuaternion.x * nQuaternion.z);
        nZW = (nQuaternion.y * nQuaternion.w);
        matrix[0][2] = (TYPE(2) * (nXY - nZW) * nInverse);
        matrix[2][0] = (TYPE(2) * (nXY + nZW) * nInverse);

        nXY = (nQuaternion.y * nQuaternion.z);
        nZW = (nQuaternion.x * nQuaternion.w);
        matrix[1][2] = (TYPE(2) * (nXY + nZW) * nInverse);
        matrix[2][1] = (TYPE(2) * (nXY - nZW) * nInverse);
    }

    void SetXAngle(TYPE nAngle)
    {
        TYPE nCos = cos(nAngle);
        TYPE nSin = sin(nAngle);
        matrix[0][0] = TYPE(1); matrix[0][1] = TYPE(0); matrix[0][2] = TYPE(0); matrix[0][3] = TYPE(0);
        matrix[1][0] = TYPE(0); matrix[1][1] = nCos;    matrix[1][2] = nSin;    matrix[1][3] = TYPE(0);
        matrix[2][0] = TYPE(0); matrix[2][1] =-nSin;    matrix[2][2] = nCos;    matrix[2][3] = TYPE(0);
        matrix[3][0] = TYPE(0); matrix[3][1] = TYPE(0); matrix[3][2] = TYPE(0); matrix[3][3] = TYPE(1);
    }

    void SetYAngle(TYPE nAngle)
    {
        TYPE nCos = cos(nAngle);
        TYPE nSin = sin(nAngle);
        matrix[0][0] = nCos;    matrix[0][1] = TYPE(0); matrix[0][2] =-nSin;    matrix[0][3] = TYPE(0);
        matrix[1][0] = TYPE(0); matrix[1][1] = TYPE(1); matrix[1][2] = TYPE(0); matrix[1][3] = TYPE(0);
        matrix[2][0] = nSin;    matrix[2][1] = TYPE(0); matrix[2][2] = nCos;    matrix[2][3] = TYPE(0);
        matrix[3][0] = TYPE(0); matrix[3][1] = TYPE(0); matrix[3][2] = TYPE(0); matrix[3][3] = TYPE(1);
    }

    void SetZAngle(TYPE nAngle)
    {
        TYPE nCos = cos(nAngle);
        TYPE nSin = sin(nAngle);
        matrix[0][0] = nCos;    matrix[0][1] = nSin;    matrix[0][2] = TYPE(0); matrix[0][3] = TYPE(0);
        matrix[1][0] =-nSin;    matrix[1][1] = nCos;    matrix[1][2] = TYPE(0); matrix[1][3] = TYPE(0);
        matrix[2][0] = TYPE(0); matrix[2][1] = TYPE(0); matrix[2][2] = TYPE(1); matrix[2][3] = TYPE(0);
        matrix[3][0] = TYPE(0); matrix[3][1] = TYPE(0); matrix[3][2] = TYPE(0); matrix[3][3] = TYPE(1);
    }

    void SetOrthographic(TYPE nMinX, TYPE nMinY, TYPE nMaxX, TYPE nMaxY, TYPE nMinZ, TYPE nMaxZ)
    {
        TYPE nScaleX = ( TYPE(2) / (nMaxX - nMinX));
        TYPE nScaleY = ( TYPE(2) / (nMinY - nMaxY));
        TYPE nScaleZ = (-TYPE(2) / (nMaxZ - nMinZ));
        TYPE nTranslationX = -((nMaxX + nMinX) / (nMaxX - nMinX));
        TYPE nTranslationY = -((nMinY + nMaxY) / (nMinY - nMaxY));
        TYPE nTranslationZ = -((nMaxZ + nMinZ) / (nMaxZ - nMinZ));

        SetIdentity();
        SetScaling(float3(nScaleX, nScaleY, nScaleZ));
        SetTranslation(float3(nTranslationX, nTranslationY, nTranslationZ));
    }

    void SetPerspective(TYPE nFOV, TYPE nAspect, TYPE nNear, TYPE nFar)
    {
	    TYPE nX = (TYPE(1) / tan(nFOV / TYPE(2)));
	    TYPE nY = (nX * nAspect); 
	    TYPE nDistance = (nFar - nNear);

	    matrix[0][0] = nX;
	    matrix[0][1] = TYPE(0);
	    matrix[0][2] = TYPE(0);
	    matrix[0][3] = TYPE(0);

	    matrix[1][0] = TYPE(0);
	    matrix[1][1] = nY;
	    matrix[1][2] = TYPE(0);
	    matrix[1][3] = TYPE(0);

	    matrix[2][0] = TYPE(0);
	    matrix[2][1] = TYPE(0);
	    matrix[2][2] = ((nFar + nNear) / nDistance);
	    matrix[2][3] = TYPE(1);

	    matrix[3][0] = TYPE(0);
	    matrix[3][1] = TYPE(0);
	    matrix[3][2] =-((TYPE(2) * nFar * nNear) / nDistance);
	    matrix[3][3] = TYPE(0);

    }

    void LookAt(const tvector3<TYPE> &nViewDirection, const tvector3<TYPE> &nWorldYGEKDEVICEAXIS)
    {
        tvector3<TYPE> nGEKDEVICEAXISZ(nViewDirection.GetNormal());
        tvector3<TYPE> nGEKDEVICEAXISX(nWorldYGEKDEVICEAXIS.Cross(nGEKDEVICEAXISZ).GetNormal());
        tvector3<TYPE> nGEKDEVICEAXISY(nGEKDEVICEAXISZ.Cross(nGEKDEVICEAXISX).GetNormal());

        SetIdentity();

        _11 = nGEKDEVICEAXISX.x;
        _21 = nGEKDEVICEAXISX.y;
        _31 = nGEKDEVICEAXISX.z;

        _12 = nGEKDEVICEAXISY.x;
        _22 = nGEKDEVICEAXISY.y;
        _32 = nGEKDEVICEAXISY.z;

        _13 = nGEKDEVICEAXISZ.x;
        _23 = nGEKDEVICEAXISZ.y;
        _33 = nGEKDEVICEAXISZ.z;

        Invert();
    }

    tvector3<TYPE> GetEuler(void)
    {
        tvector3<TYPE> nEuler;
        nEuler.y = asin(_31);

        TYPE nX, nY;
        TYPE nCos = cos(nEuler.y);
        if (abs(nCos) > 0.005)
        {
            nX = (_33 / nCos);
            nY =-(_32 / nCos);
            nEuler.x = atan2(nY, nX);

            nX = (_11 / nCos);
            nY =-(_21 / nCos);
            nEuler.z = atan2(nY, nX);
        }
        else
        {
            nEuler.x = TYPE(0);

            nX = _22;
            nY = _12;
            nEuler.y = atan2(nY, nX);
        }

        if (nEuler.x < TYPE(0))
        {
            nEuler.x += _2_PI;
        }

        if (nEuler.y < TYPE(0))
        {
            nEuler.y += _2_PI;
        }

        if (nEuler.z < TYPE(0))
        {
            nEuler.z += _2_PI;
        }

        return nEuler;
    }

    tvector3<TYPE> GetScaling(void) const
    {
        return tvector3<TYPE>(_11, _22, _33);
    }

    TYPE GetDeterminant(void) const
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

    tmatrix4x4<TYPE> GetTranspose(void) const
    {
        return tmatrix4x4<TYPE>(_11, _21, _31, _41,
                               _12, _22, _32, _42,
                               _13, _23, _33, _43,
                               _14, _24, _34, _44);
    }

    tmatrix4x4<TYPE> GetInverse(void) const
    {
        TYPE nDerminant = GetDeterminant();
        if (abs(nDerminant) < _EPSILON) 
        {
            return tmatrix4x4<TYPE>();
        }
        else
        {
            nDerminant = (TYPE(1) / nDerminant);

            tmatrix4x4<TYPE> nMatrix;
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

    void operator *= (const tmatrix4x4<TYPE> &nMatrix)
    {
        (*this) = ((*this) * nMatrix);
    }

    tmatrix4x4<TYPE> operator * (const tmatrix4x4<TYPE> &nMatrix) const
    {
        tmatrix4x4<TYPE> nTranspose(nMatrix.GetTranspose());
        return tmatrix4x4<TYPE>(rx.Dot(nTranspose.rx), rx.Dot(nTranspose.ry), rx.Dot(nTranspose.rz), rx.Dot(nTranspose.rw),
                               ry.Dot(nTranspose.rx), ry.Dot(nTranspose.ry), ry.Dot(nTranspose.rz), ry.Dot(nTranspose.rw),
                               rz.Dot(nTranspose.rx), rz.Dot(nTranspose.ry), rz.Dot(nTranspose.rz), rz.Dot(nTranspose.rw),
                               rw.Dot(nTranspose.rx), rw.Dot(nTranspose.ry), rw.Dot(nTranspose.rz), rw.Dot(nTranspose.rw));
    }

    void OldMulEqual(const tmatrix4x4<TYPE> &nMatrix)
    {
        *this = tmatrix4x4<TYPE>(((_11 * nMatrix._11) + (_12 * nMatrix._21) + (_13 * nMatrix._31) + (_14 * nMatrix._41)),
                                ((_11 * nMatrix._12) + (_12 * nMatrix._22) + (_13 * nMatrix._32) + (_14 * nMatrix._42)),
                                ((_11 * nMatrix._13) + (_12 * nMatrix._23) + (_13 * nMatrix._33) + (_14 * nMatrix._43)),
                                ((_11 * nMatrix._14) + (_12 * nMatrix._24) + (_13 * nMatrix._34) + (_14 * nMatrix._44)),

                                ((_21 * nMatrix._11) + (_22 * nMatrix._21) + (_23 * nMatrix._31) + (_24 * nMatrix._41)),
                                ((_21 * nMatrix._12) + (_22 * nMatrix._22) + (_23 * nMatrix._32) + (_24 * nMatrix._42)),
                                ((_21 * nMatrix._13) + (_22 * nMatrix._23) + (_23 * nMatrix._33) + (_24 * nMatrix._43)),
                                ((_21 * nMatrix._14) + (_22 * nMatrix._24) + (_23 * nMatrix._34) + (_24 * nMatrix._44)),

                                ((_31 * nMatrix._11) + (_32 * nMatrix._21) + (_33 * nMatrix._31) + (_34 * nMatrix._41)),
                                ((_31 * nMatrix._12) + (_32 * nMatrix._22) + (_33 * nMatrix._32) + (_34 * nMatrix._42)),
                                ((_31 * nMatrix._13) + (_32 * nMatrix._23) + (_33 * nMatrix._33) + (_34 * nMatrix._43)),
                                ((_31 * nMatrix._14) + (_32 * nMatrix._24) + (_33 * nMatrix._34) + (_34 * nMatrix._44)),

                                ((_41 * nMatrix._11) + (_42 * nMatrix._21) + (_43 * nMatrix._31) + (_44 * nMatrix._41)),
                                ((_41 * nMatrix._12) + (_42 * nMatrix._22) + (_43 * nMatrix._32) + (_44 * nMatrix._42)),
                                ((_41 * nMatrix._13) + (_42 * nMatrix._23) + (_43 * nMatrix._33) + (_44 * nMatrix._43)),
                                ((_41 * nMatrix._14) + (_42 * nMatrix._24) + (_43 * nMatrix._34) + (_44 * nMatrix._44)));
    }

    tmatrix4x4<TYPE> OldMul(const tmatrix4x4<TYPE> &nMatrix) const
    {
        return tmatrix4x4<TYPE>(((_11 * nMatrix._11) + (_12 * nMatrix._21) + (_13 * nMatrix._31) + (_14 * nMatrix._41)),
                               ((_11 * nMatrix._12) + (_12 * nMatrix._22) + (_13 * nMatrix._32) + (_14 * nMatrix._42)),
                               ((_11 * nMatrix._13) + (_12 * nMatrix._23) + (_13 * nMatrix._33) + (_14 * nMatrix._43)),
                               ((_11 * nMatrix._14) + (_12 * nMatrix._24) + (_13 * nMatrix._34) + (_14 * nMatrix._44)),

                               ((_21 * nMatrix._11) + (_22 * nMatrix._21) + (_23 * nMatrix._31) + (_24 * nMatrix._41)),
                               ((_21 * nMatrix._12) + (_22 * nMatrix._22) + (_23 * nMatrix._32) + (_24 * nMatrix._42)),
                               ((_21 * nMatrix._13) + (_22 * nMatrix._23) + (_23 * nMatrix._33) + (_24 * nMatrix._43)),
                               ((_21 * nMatrix._14) + (_22 * nMatrix._24) + (_23 * nMatrix._34) + (_24 * nMatrix._44)),

                               ((_31 * nMatrix._11) + (_32 * nMatrix._21) + (_33 * nMatrix._31) + (_34 * nMatrix._41)),
                               ((_31 * nMatrix._12) + (_32 * nMatrix._22) + (_33 * nMatrix._32) + (_34 * nMatrix._42)),
                               ((_31 * nMatrix._13) + (_32 * nMatrix._23) + (_33 * nMatrix._33) + (_34 * nMatrix._43)),
                               ((_31 * nMatrix._14) + (_32 * nMatrix._24) + (_33 * nMatrix._34) + (_34 * nMatrix._44)),

                               ((_41 * nMatrix._11) + (_42 * nMatrix._21) + (_43 * nMatrix._31) + (_44 * nMatrix._41)),
                               ((_41 * nMatrix._12) + (_42 * nMatrix._22) + (_43 * nMatrix._32) + (_44 * nMatrix._42)),
                               ((_41 * nMatrix._13) + (_42 * nMatrix._23) + (_43 * nMatrix._33) + (_44 * nMatrix._43)),
                               ((_41 * nMatrix._14) + (_42 * nMatrix._24) + (_43 * nMatrix._34) + (_44 * nMatrix._44)));
    }

    tmatrix4x4<TYPE> operator = (const tmatrix4x4<TYPE> &nMatrix)
    {
        _11 = nMatrix._11;    _12 = nMatrix._12;    _13 = nMatrix._13;    _14 = nMatrix._14;
        _21 = nMatrix._21;    _22 = nMatrix._22;    _23 = nMatrix._23;    _24 = nMatrix._24;
        _31 = nMatrix._31;    _32 = nMatrix._32;    _33 = nMatrix._33;    _34 = nMatrix._34;
        _41 = nMatrix._41;    _42 = nMatrix._42;    _43 = nMatrix._43;    _44 = nMatrix._44;
        return *this;
    }

    tmatrix4x4<TYPE> operator = (const tquaternion<TYPE> &nQuaternion)
    {
        SetQuaternion(nQuaternion);
        return *this;
    }

    tvector3<TYPE> operator * (const tvector3<TYPE> &nVector) const
    {
        return tvector3<TYPE>(((nVector.x * _11) + (nVector.y * _21) + (nVector.z * _31)),
                            ((nVector.x * _12) + (nVector.y * _22) + (nVector.z * _32)),
                            ((nVector.x * _13) + (nVector.y * _23) + (nVector.z * _33)));
    }

    tvector4<TYPE> operator * (const tvector4<TYPE> &nVector) const
    {
        return tvector4<TYPE>(((nVector.x * _11) + (nVector.y * _21) + (nVector.z * _31) + (nVector.w * _41)),
                            ((nVector.x * _12) + (nVector.y * _22) + (nVector.z * _32) + (nVector.w * _42)),
                            ((nVector.x * _13) + (nVector.y * _23) + (nVector.z * _33) + (nVector.w * _43)),
                            ((nVector.x * _14) + (nVector.y * _24) + (nVector.z * _34) + (nVector.w * _44)));
    }

    tmatrix4x4<TYPE> operator * (TYPE nScalar) const
    {
        return tmatrix4x4<TYPE>((_11 * nScalar), (_12 * nScalar), (_13 * nScalar), (_14 * nScalar),
                               (_21 * nScalar), (_22 * nScalar), (_23 * nScalar), (_24 * nScalar),
                               (_31 * nScalar), (_32 * nScalar), (_33 * nScalar), (_34 * nScalar),
                               (_41 * nScalar), (_42 * nScalar), (_43 * nScalar), (_44 * nScalar));
    }

    tmatrix4x4<TYPE> operator + (const tmatrix4x4<TYPE> &nMatrix) const
    {
        return tmatrix4x4<TYPE>(_11 + nMatrix._11, _12 + nMatrix._12, _13 + nMatrix._13, _14 + nMatrix._14,
                               _21 + nMatrix._21, _22 + nMatrix._22, _23 + nMatrix._23, _24 + nMatrix._24,
                               _31 + nMatrix._31, _32 + nMatrix._32, _33 + nMatrix._33, _34 + nMatrix._34,
                               _41 + nMatrix._41, _42 + nMatrix._42, _43 + nMatrix._43, _44 + nMatrix._44);
    }

    void operator += (const tmatrix4x4<TYPE> &nMatrix)
    {
        _11 += nMatrix._11; _12 += nMatrix._12; _13 += nMatrix._13; _14 += nMatrix._14;
        _21 += nMatrix._21; _22 += nMatrix._22; _23 += nMatrix._23; _24 += nMatrix._24;
        _31 += nMatrix._31; _32 += nMatrix._32; _33 += nMatrix._33; _34 += nMatrix._34;
        _41 += nMatrix._41; _42 += nMatrix._42; _43 += nMatrix._43; _44 += nMatrix._44;
    }
};
