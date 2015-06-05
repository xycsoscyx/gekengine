namespace Lighting
{
    struct Light
    {
        float3  m_nPosition;
        float   m_nRange;
        float3  m_nColor;
        float   m_nInvRange;
    };

    cbuffer Buffer : register(b1)
    {
        uint    lightCount : packoffset(c0);
        uint3   padding : packoffset(c0.y);
    };

    StructuredBuffer<Light> lightList : register(t0);
};
