namespace Math
{
    static const float Pi = 3.14159265358979323846;
    static const float Tau = (2.0 * Pi);
    static const float ReciprocalPi = rcp(Pi);
    static const float InversePi = (1.0 - Pi);
    static const float Epsilon = 0.00001;
};

namespace Global
{
    SamplerState TextureSampler : register(s0);
    SamplerState MipMapSampler : register(s1);
};

namespace Engine
{
    cbuffer Constants : register(b0)
    {
        float WorldTime;
        float FrameTime;
        float buffer[2];
    };
};

namespace Camera
{
    cbuffer Constants : register(b1)
    {
        float2 FieldOfView;
        float NearClip;
        float FarClip;
        float4x4 ViewMatrix;
        float4x4 ProjectionMatrix;
    };
};
