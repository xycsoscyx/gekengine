#pragma once

template <typename TYPE>
struct tvector3;

template <typename TYPE>
struct tmatrix4x4;

template <typename TYPE>
struct tquaternion
{
public:
    union
    {
        struct { TYPE xyzw[4]; };
        struct { TYPE x, y, z, w; };
    };

public:
    tquaternion(void)
    {
        x = TYPE(0);
        y = TYPE(0);
        z = TYPE(0);
        w = TYPE(1);
    }

    tquaternion(float nValue)
    {
        x = y = z = w = nValue;
    }

    tquaternion(float nX, float nY, float nZ, float nW)
    {
        x = nX;
        y = nY;
        z = nZ;
        w = nW;
    }

    tquaternion(const tquaternion<TYPE> &nQuaternion)
    {
        x = nQuaternion.x;
        y = nQuaternion.y;
        z = nQuaternion.z;
        w = nQuaternion.w;
    }

    tquaternion(const tmatrix4x4<TYPE> &nMatrix)
    {
        SetMatrix(nMatrix);
    }

    tquaternion(const tvector3<TYPE> &nGEKDEVICEAXIS, TYPE nAngle)
    {
        SetRotation(nGEKDEVICEAXIS, nAngle);
    }

    tquaternion(TYPE nX, TYPE nY, TYPE nZ)
    {
        SetEuler(nX, nY, nZ);
    }

    tquaternion(const tvector3<TYPE> &nEuler)
    {
        SetEuler(nEuler);
    }

    tquaternion(const tvector4<TYPE> &nVector)
    {
        x = nVector.x;
        y = nVector.y;
        z = nVector.z;
        w = nVector.w;
    }

    void SetIdentity(void)
    {
        x = y = z = TYPE(0);
        w = TYPE(1);
    }

    void SetLength(TYPE nLength)
    {
        nLength = (nLength / GetLength());
        x *= nLength;
        y *= nLength;
        z *= nLength;
        w *= nLength;
    }

    void SetEuler(const tvector3<TYPE> &nEuler)
    {
        SetEuler(nEuler.x, nEuler.y, nEuler.z);
    }

    void SetEuler(TYPE nX, TYPE nY, TYPE nZ)
    {
        TYPE nSinX = sin(nX * TYPE(0.5));
        TYPE nSinY = sin(nY * TYPE(0.5));
        TYPE nSinZ = sin(nZ * TYPE(0.5));
        TYPE nCosX = cos(nX * TYPE(0.5));
        TYPE nCosY = cos(nY * TYPE(0.5));
        TYPE nCosZ = cos(nZ * TYPE(0.5));
        x = ((nSinX * nCosY * nCosZ) - (nCosX * nSinY * nSinZ));
        y = ((nSinX * nCosY * nSinZ) + (nCosX * nSinY * nCosZ));
        z = ((nCosX * nCosY * nSinZ) - (nSinX * nSinY * nCosZ));
        w = ((nCosX * nCosY * nCosZ) + (nSinX * nSinY * nSinZ));
    }

    void SetRotation(const tvector3<TYPE> &nGEKDEVICEAXIS, TYPE nAngle)
    {
        tvector3<TYPE> nNormal(nGEKDEVICEAXIS.GetNormal());
        TYPE nSin = sin(nAngle * TYPE(0.5));
        x = (nNormal.x * nSin);
        y = (nNormal.y * nSin);
        z = (nNormal.z * nSin);
        w = cos(nAngle * TYPE(0.5));
    }

    void SetMatrix(const tmatrix4x4<TYPE> &nMatrix)
    {
        TYPE nTrace = (nMatrix.matrix[0][0] + nMatrix.matrix[1][1] + nMatrix.matrix[2][2] + TYPE(1));  
        if (nTrace > _EPSILON) 
        {
            TYPE nInverse = (TYPE(0.5) / sqrt(nTrace));
            w =  (TYPE(0.25) / nInverse);
            x = ((nMatrix.matrix[1][2] - nMatrix.matrix[2][1] ) * nInverse);
            y = ((nMatrix.matrix[2][0] - nMatrix.matrix[0][2] ) * nInverse);
            z = ((nMatrix.matrix[0][1] - nMatrix.matrix[1][0] ) * nInverse);
        } 
        else 
        {
            if ((nMatrix.matrix[0][0] > nMatrix.matrix[1][1])&&(nMatrix.matrix[0][0] > nMatrix.matrix[2][2])) 
            {
                TYPE nInverse = (TYPE(2) * sqrt(TYPE(1) + nMatrix.matrix[0][0] - nMatrix.matrix[1][1] - nMatrix.matrix[2][2]));
                x =  (TYPE(0.25) * nInverse);
                y = ((nMatrix.matrix[1][0] + nMatrix.matrix[0][1]) / nInverse);
                z = ((nMatrix.matrix[2][0] + nMatrix.matrix[0][2]) / nInverse);
                w = ((nMatrix.matrix[2][1] - nMatrix.matrix[1][2]) / nInverse);    
            } 
            else if (nMatrix.matrix[1][1] > nMatrix.matrix[2][2]) 
            {
                TYPE nInverse = TYPE(2) * (sqrt(TYPE(1) + nMatrix.matrix[1][1] - nMatrix.matrix[0][0] - nMatrix.matrix[2][2]));
                x = ((nMatrix.matrix[1][0] + nMatrix.matrix[0][1]) / nInverse);
                y =  (TYPE(0.25) * nInverse);
                z = ((nMatrix.matrix[2][1] + nMatrix.matrix[1][2]) / nInverse);
                w = ((nMatrix.matrix[2][0] - nMatrix.matrix[0][2]) / nInverse);   
            }
            else 
            {
                TYPE nInverse = TYPE(2) * (sqrt(TYPE(1) + nMatrix.matrix[2][2] - nMatrix.matrix[0][0] - nMatrix.matrix[1][1]));
                x = ((nMatrix.matrix[2][0] + nMatrix.matrix[0][2]) / nInverse);
                y = ((nMatrix.matrix[2][1] + nMatrix.matrix[1][2]) / nInverse);
                z =  (TYPE(0.25) * nInverse);
                w = ((nMatrix.matrix[1][0] - nMatrix.matrix[0][1]) / nInverse);
            }
        }

        Normalize();
    }

    float GetLengthSqr(void) const
    {
        return ((x * x) + (y * y) + (z * z) + (w * w));
    }

    TYPE GetLength(void) const
    {
        return sqrt((x * x) + (y * y) + (z * z) + (w * w));
    }

    tvector3<TYPE> GetEuler(void) const
    {
        TYPE sqw = (w * w);
        TYPE sqx = (x * x);
        TYPE sqy = (y * y);
        TYPE sqz = (z * z);
        return tvector3<TYPE>(TYPE(atan2(TYPE(2) * ((y * z) + (x * w)), (-sqx - sqy + sqz + sqw))),
                              TYPE(asin(-TYPE(2) * ((x * z) - (y * w)))),
                              TYPE(atan2(TYPE(2) * ((x * y) + (z * w)), ( sqx - sqy - sqz + sqw))));
    }

    tmatrix4x4<TYPE> GetMatrix(void) const
    {
        return tmatrix4x4<TYPE>(*this);
    }

    tquaternion<TYPE> GetNormal(void) const
    {
        TYPE nLength = (TYPE(1) / GetLength());
        return tquaternion<TYPE>((x * nLength), (y * nLength), (z * nLength), (w * nLength));
    }

    tquaternion<TYPE> GetInverse(void) const
    {
        return tquaternion<TYPE>(-x, -y, -z, w);
    }

    void Normalize(void)
    {
        TYPE nLength = (TYPE(1) / GetLength());
        x *= nLength;
        y *= nLength;
        z *= nLength;
        w *= nLength;
    }

    void Invert(void)
    {
        x = -x;
        y = -y;
        z = -z;
        w =  w;
    }

    float Dot(const tquaternion<TYPE> &nQuaternion) const
    {
        return ((x * nQuaternion.x) + (y * nQuaternion.y) + (z * nQuaternion.z) + (w * nQuaternion.w));
    }

    tquaternion<TYPE> Slerp(const tquaternion<TYPE> &nQuaternion, TYPE nFactor) const
    {
        tquaternion<TYPE> nOriginal(nQuaternion);
        TYPE nCos = nQuaternion.Dot(*this);
        if (nCos < TYPE(0))
        {
            nCos = -nCos;
            nOriginal.x = -nOriginal.x;
            nOriginal.y = -nOriginal.y;
            nOriginal.z = -nOriginal.z;
            nOriginal.w = -nOriginal.w;
        }

        TYPE nScale0;
        TYPE nScale1;
        if (abs(TYPE(1) - nCos) < _EPSILON)
        {
            nScale0 = (TYPE(1) - nFactor);
            nScale1 = nFactor;
        }
        else
        {
            TYPE nOmega = acos(nCos);
            TYPE nSinom = sin(nOmega);

            nScale0 = (sin((TYPE(1) - nFactor) * nOmega) / nSinom);
            nScale1 = (sin(nFactor * nOmega) / nSinom);
        }

        return tquaternion<TYPE>(((nScale0 * x) + (nScale1 * nOriginal.x)),
                                    ((nScale0 * y) + (nScale1 * nOriginal.y)),
                                    ((nScale0 * z) + (nScale1 * nOriginal.z)),
                                    ((nScale0 * w) + (nScale1 * nOriginal.w)));
    }

    tvector3<TYPE> operator * (const tvector3<TYPE> &nVector) const
    {
        return (tmatrix4x4<TYPE>(*this) * nVector);
    }

    tvector4<TYPE> operator * (const tvector4<TYPE> &nVector) const
    {
        return (tmatrix4x4<TYPE>(*this) * nVector);
    }

    tquaternion<TYPE> operator * (const tquaternion<TYPE> &nQuaternion) const
    {
        TYPE nW = ((w * nQuaternion.w) - (x * nQuaternion.x) - (y * nQuaternion.y) - (z * nQuaternion.z));
        TYPE nX = ((w * nQuaternion.x) + (x * nQuaternion.w) + (y * nQuaternion.z) - (z * nQuaternion.y));
        TYPE nY = ((w * nQuaternion.y) + (y * nQuaternion.w) + (z * nQuaternion.x) - (x * nQuaternion.z));
        TYPE nZ = ((w * nQuaternion.z) + (z * nQuaternion.w) + (x * nQuaternion.y) - (y * nQuaternion.x));
        return tquaternion<TYPE>(nX, nY, nZ, nW);
    }

    void operator *= (const tquaternion<TYPE> &nQuaternion)
    {
        TYPE nW = ((w * nQuaternion.w) - (x * nQuaternion.x) - (y * nQuaternion.y) - (z * nQuaternion.z));
        TYPE nX = ((w * nQuaternion.x) + (x * nQuaternion.w) + (y * nQuaternion.z) - (z * nQuaternion.y));
        TYPE nY = ((w * nQuaternion.y) + (y * nQuaternion.w) + (z * nQuaternion.x) - (x * nQuaternion.z));
        TYPE nZ = ((w * nQuaternion.z) + (z * nQuaternion.w) + (x * nQuaternion.y) - (y * nQuaternion.x));
        x = nX;
        y = nY;
        z = nZ;
        w = nW;
    }

    tquaternion<TYPE> operator = (const tvector4<TYPE> &nVector)
    {
        x = nVector.x;
        y = nVector.y;
        z = nVector.z;
        w = nVector.w;
        return *this;
    }

    tquaternion<TYPE> operator = (const tquaternion<TYPE> &nQuaternion)
    {
        x = nQuaternion.x;
        y = nQuaternion.y;
        z = nQuaternion.z;
        w = nQuaternion.w;
        return *this;
    }

    tquaternion<TYPE> operator = (const tmatrix4x4<TYPE> &nMatrix)
    {
        SetMatrix(nMatrix);
        return *this;
    }
};

template <typename TYPE>
std::ostream & operator << (std::ostream &kStream, const tquaternion<TYPE> &nValue)
{
    return (kStream << nValue.x << "," << nValue.y << "," << nValue.z << "," << nValue.w);
}
