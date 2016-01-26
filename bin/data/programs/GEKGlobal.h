namespace Math
{
    static const float Pi = 3.14159265358979323846;
    static const float Tau = (2.0 * 3.14159265358979323846);
    static const float ReciprocalPi = (1.0 / Pi);
    static const float InversePi = (1.0 - Pi);
    static const float TwoPi = 2.0 * Pi;
};

namespace Global
{
    SamplerState pointSampler : register(s0);
    SamplerState linearSampler : register(s1);
};

namespace Camera
{
    cbuffer buffer : register(b0)
    {
        float2   fieldOfView             : packoffset(c0);
        float    minimumDistance         : packoffset(c0.z);
        float    maximumDistance         : packoffset(c0.w);
        float4x4 viewMatrix              : packoffset(c1);
        float4x4 projectionMatrix        : packoffset(c5);
        float4x4 inverseProjectionMatrix : packoffset(c9);
    };
};
