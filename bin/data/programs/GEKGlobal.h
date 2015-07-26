namespace Math
{
    static const float Pi = 3.14159265358979323846f;
    static const float ReciprocalPi = rcp(Pi);
    static const float TwoPi = 2.0f * Pi;
};

namespace Global
{
    SamplerState pointSampler : register(s0)
    {
        Filter = MIN_MAG_MIP_POINT;
        AddressU = Clamp;
        AddressV = Clamp;
    };

    SamplerState linearSampler : register(s1)
    {
        Filter = MIN_MAG_MIP_LINEAR;
        //MaxAnisotropy = StrToUINT32(kRender.GetAttribute(L"anisotropy"));
        //Filter = ANISOTROPIC;
        AddressU = Wrap;
        AddressV = Wrap;
    };
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
