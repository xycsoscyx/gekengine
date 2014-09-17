#pragma once

struct float3;

struct float4x4;

struct quaternion
{
public:
    union
    {
        struct { float xyzw[4]; };
        struct { float x, y, z, w; };
    };

public:
    quaternion(void)
    {
        x = 0.0f;
        y = 0.0f;
        z = 0.0f;
        w = 1.0f;
    }

    quaternion(float nValue)
    {
        x = y = z = w = nValue;
    }

    quaternion(float nX, float nY, float nZ, float nW)
    {
        x = nX;
        y = nY;
        z = nZ;
        w = nW;
    }

    quaternion(const quaternion &nQuaternion)
    {
        x = nQuaternion.x;
        y = nQuaternion.y;
        z = nQuaternion.z;
        w = nQuaternion.w;
    }

    quaternion(const float4x4 &nMatrix)
    {
        SetMatrix(nMatrix);
    }

    quaternion(const float3 &nAxis, float nAngle)
    {
        SetRotation(nAxis, nAngle);
    }

    quaternion(float nX, float nY, float nZ)
    {
        SetEuler(nX, nY, nZ);
    }

    quaternion(const float3 &nEuler)
    {
        SetEuler(nEuler);
    }

    quaternion(const float4 &nVector)
    {
        x = nVector.x;
        y = nVector.y;
        z = nVector.z;
        w = nVector.w;
    }

    void SetIdentity(void)
    {
        x = y = z = 0.0f;
        w = 1.0f;
    }

    void SetLength(float nLength)
    {
        nLength = (nLength / GetLength());
        x *= nLength;
        y *= nLength;
        z *= nLength;
        w *= nLength;
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
        Concurrency::precise_math::sincos(nX * 0.5f, &nSinX, &nSinY);
        Concurrency::precise_math::sincos(nY * 0.5f, &nSinY, &nSinY);
        Concurrency::precise_math::sincos(nZ * 0.5f, &nSinZ, &nSinZ);
        x = ((nSinX * nCosY * nCosZ) - (nCosX * nSinY * nSinZ));
        y = ((nSinX * nCosY * nSinZ) + (nCosX * nSinY * nCosZ));
        z = ((nCosX * nCosY * nSinZ) - (nSinX * nSinY * nCosZ));
        w = ((nCosX * nCosY * nCosZ) + (nSinX * nSinY * nSinZ));
    }

    void SetRotation(const float3 &nAxis, float nAngle)
    {
        float nSin;
        Concurrency::precise_math::sincos(nAngle * 0.5f, &nSin, &w);
        float3 nNormal(nAxis.GetNormal());
        x = (nNormal.x * nSin);
        y = (nNormal.y * nSin);
        z = (nNormal.z * nSin);
    }

    void SetMatrix(const float4x4 &nMatrix)
    {
        float nTrace = (nMatrix.matrix[0][0] + nMatrix.matrix[1][1] + nMatrix.matrix[2][2] + 1.0f);  
        if (nTrace > _EPSILON) 
        {
            float nInverse = (0.5f / sqrt(nTrace));
            w =  (0.25f / nInverse);
            x = ((nMatrix.matrix[1][2] - nMatrix.matrix[2][1] ) * nInverse);
            y = ((nMatrix.matrix[2][0] - nMatrix.matrix[0][2] ) * nInverse);
            z = ((nMatrix.matrix[0][1] - nMatrix.matrix[1][0] ) * nInverse);
        } 
        else 
        {
            if ((nMatrix.matrix[0][0] > nMatrix.matrix[1][1])&&(nMatrix.matrix[0][0] > nMatrix.matrix[2][2])) 
            {
                float nInverse = (2.0f * sqrt(1.0f + nMatrix.matrix[0][0] - nMatrix.matrix[1][1] - nMatrix.matrix[2][2]));
                x =  (0.25f * nInverse);
                y = ((nMatrix.matrix[1][0] + nMatrix.matrix[0][1]) / nInverse);
                z = ((nMatrix.matrix[2][0] + nMatrix.matrix[0][2]) / nInverse);
                w = ((nMatrix.matrix[2][1] - nMatrix.matrix[1][2]) / nInverse);    
            } 
            else if (nMatrix.matrix[1][1] > nMatrix.matrix[2][2]) 
            {
                float nInverse = 2.0f * (sqrt(1.0f + nMatrix.matrix[1][1] - nMatrix.matrix[0][0] - nMatrix.matrix[2][2]));
                x = ((nMatrix.matrix[1][0] + nMatrix.matrix[0][1]) / nInverse);
                y =  (0.25f * nInverse);
                z = ((nMatrix.matrix[2][1] + nMatrix.matrix[1][2]) / nInverse);
                w = ((nMatrix.matrix[2][0] - nMatrix.matrix[0][2]) / nInverse);   
            }
            else 
            {
                float nInverse = 2.0f * (sqrt(1.0f + nMatrix.matrix[2][2] - nMatrix.matrix[0][0] - nMatrix.matrix[1][1]));
                x = ((nMatrix.matrix[2][0] + nMatrix.matrix[0][2]) / nInverse);
                y = ((nMatrix.matrix[2][1] + nMatrix.matrix[1][2]) / nInverse);
                z =  (0.25f * nInverse);
                w = ((nMatrix.matrix[1][0] - nMatrix.matrix[0][1]) / nInverse);
            }
        }

        Normalize();
    }

    float GetLengthSqr(void) const
    {
        return ((x * x) + (y * y) + (z * z) + (w * w));
    }

    float GetLength(void) const
    {
        return sqrt((x * x) + (y * y) + (z * z) + (w * w));
    }

    float3 GetEuler(void) const
    {
        float sqw = (w * w);
        float sqx = (x * x);
        float sqy = (y * y);
        float sqz = (z * z);
        return float3(Concurrency::precise_math::atan2(2.0f * ((y * z) + (x * w)), (-sqx - sqy + sqz + sqw)),
                      Concurrency::precise_math::asin(-2.0f * ((x * z) - (y * w))),
                      Concurrency::precise_math::atan2(2.0f * ((x * y) + (z * w)), ( sqx - sqy - sqz + sqw)));
    }

    float4x4 GetMatrix(void) const
    {
        return float4x4(*this);
    }

    quaternion GetNormal(void) const
    {
        float nLength = (1.0f / GetLength());
        return quaternion((x * nLength), (y * nLength), (z * nLength), (w * nLength));
    }

    quaternion GetInverse(void) const
    {
        return quaternion(-x, -y, -z, w);
    }

    void Normalize(void)
    {
        float nLength = (1.0f / GetLength());
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

    float Dot(const quaternion &nQuaternion) const
    {
        return ((x * nQuaternion.x) + (y * nQuaternion.y) + (z * nQuaternion.z) + (w * nQuaternion.w));
    }

    quaternion Slerp(const quaternion &nQuaternion, float nFactor) const
    {
        quaternion nOriginal(nQuaternion);
        float nCos = nQuaternion.Dot(*this);
        if (nCos < 0.0f)
        {
            nCos = -nCos;
            nOriginal.x = -nOriginal.x;
            nOriginal.y = -nOriginal.y;
            nOriginal.z = -nOriginal.z;
            nOriginal.w = -nOriginal.w;
        }

        float nScale0;
        float nScale1;
        if (abs(1.0f - nCos) < _EPSILON)
        {
            nScale0 = (1.0f - nFactor);
            nScale1 = nFactor;
        }
        else
        {
            float nOmega = Concurrency::precise_math::acos(nCos);
            float nSinom = Concurrency::precise_math::sin(nOmega);

            nScale0 = (Concurrency::precise_math::sin((1.0f - nFactor) * nOmega) / nSinom);
            nScale1 = (Concurrency::precise_math::sin(nFactor * nOmega) / nSinom);
        }

        return quaternion(((nScale0 * x) + (nScale1 * nOriginal.x)),
                                    ((nScale0 * y) + (nScale1 * nOriginal.y)),
                                    ((nScale0 * z) + (nScale1 * nOriginal.z)),
                                    ((nScale0 * w) + (nScale1 * nOriginal.w)));
    }

    float3 operator * (const float3 &nVector) const
    {
        return (float4x4(*this) * nVector);
    }

    float4 operator * (const float4 &nVector) const
    {
        return (float4x4(*this) * nVector);
    }

    quaternion operator * (const quaternion &nQuaternion) const
    {
        float nW = ((w * nQuaternion.w) - (x * nQuaternion.x) - (y * nQuaternion.y) - (z * nQuaternion.z));
        float nX = ((w * nQuaternion.x) + (x * nQuaternion.w) + (y * nQuaternion.z) - (z * nQuaternion.y));
        float nY = ((w * nQuaternion.y) + (y * nQuaternion.w) + (z * nQuaternion.x) - (x * nQuaternion.z));
        float nZ = ((w * nQuaternion.z) + (z * nQuaternion.w) + (x * nQuaternion.y) - (y * nQuaternion.x));
        return quaternion(nX, nY, nZ, nW);
    }

    void operator *= (const quaternion &nQuaternion)
    {
        float nW = ((w * nQuaternion.w) - (x * nQuaternion.x) - (y * nQuaternion.y) - (z * nQuaternion.z));
        float nX = ((w * nQuaternion.x) + (x * nQuaternion.w) + (y * nQuaternion.z) - (z * nQuaternion.y));
        float nY = ((w * nQuaternion.y) + (y * nQuaternion.w) + (z * nQuaternion.x) - (x * nQuaternion.z));
        float nZ = ((w * nQuaternion.z) + (z * nQuaternion.w) + (x * nQuaternion.y) - (y * nQuaternion.x));
        x = nX;
        y = nY;
        z = nZ;
        w = nW;
    }

    quaternion operator = (const float4 &nVector)
    {
        x = nVector.x;
        y = nVector.y;
        z = nVector.z;
        w = nVector.w;
        return *this;
    }

    quaternion operator = (const quaternion &nQuaternion)
    {
        x = nQuaternion.x;
        y = nQuaternion.y;
        z = nQuaternion.z;
        w = nQuaternion.w;
        return *this;
    }

    quaternion operator = (const float4x4 &nMatrix)
    {
        SetMatrix(nMatrix);
        return *this;
    }
};
