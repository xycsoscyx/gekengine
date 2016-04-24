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
    cbuffer Constants : register(b0)
    {
        float worldTime;
        float frameTime;
        float buffer[2];
    };
};

namespace Camera
{
    cbuffer Constants : register(b1)
    {
        float2 fieldOfView;
        float minimumDistance;
        float maximumDistance;
        float4x4 viewMatrix;
        float4x4 projectionMatrix;
        float4x4 inverseProjectionMatrix;
    };
};

namespace Shader
{
    cbuffer Constants : register(b2)
    {
        float2 targetSize;
        float buffer[2];
    };
};