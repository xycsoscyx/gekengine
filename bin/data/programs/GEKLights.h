namespace Lighting
{
    struct Light
    {
        float3  position;
        float   distance;
        float   range;
        float3  color;
    };

    cbuffer Data : register(b1)
    {
        uint    lightCount : packoffset(c0);
        uint3   padding    : packoffset(c0.y);
    };

    StructuredBuffer<Light> lightList : register(t0);
};
