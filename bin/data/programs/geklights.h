struct LIGHT
{
    float3  m_nPosition;
    float   m_nRange;
    float3  m_nColor;
    float   m_nInvRange;
};

cbuffer LIGHTBUFFER                     : register(b2)
{
    uint    gs_nNumLights               : packoffset(c0);
    uint3   gs_nLightPadding            : packoffset(c0.y);
};

StructuredBuffer<LIGHT> gs_aLights      : register(t0);
