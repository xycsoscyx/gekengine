namespace Math
{
    static const float Pi = 3.14159265358979323846;
    static const float Tau = (2.0 * Pi);
    static const float ReciprocalPi = (1.0 / Pi);
    static const float InversePi = (1.0 - Pi);
    static const float Epsilon = 0.0001;
};

namespace Global
{
    SamplerState pointSampler : register(s0);
    SamplerState linearClampSampler : register(s1);
    SamplerState linearWrapSampler : register(s2);
};

namespace Engine
{
    cbuffer buffer : register(b0)
    {
        float    worldTime               : packoffset(c0);
        float    frameTime               : packoffset(c0.y);
    };
};

namespace Camera
{
    cbuffer buffer : register(b1)
    {
        float2   fieldOfView             : packoffset(c0);
        float    minimumDistance         : packoffset(c0.z);
        float    maximumDistance         : packoffset(c0.w);
        float4x4 viewMatrix              : packoffset(c1);
        float4x4 projectionMatrix        : packoffset(c5);
        float4x4 inverseProjectionMatrix : packoffset(c9);
    };
};

namespace Shader
{
    cbuffer buffer : register(b2)
    {
        float    width                   : packoffset(c0);
        float    height                  : packoffset(c0.y);
    };
};
