struct LIGHT
{
    float3  m_nPosition;
    float   m_nRange;
    float3  m_nColor;
    float   m_nInvRange;
};

#ifdef _TYPE_COMPUTE
cbuffer LIGHTBUFFER                     : register(b1)
#else
cbuffer LIGHTBUFFER                     : register(b2)
#endif
{
    uint    gs_nNumLights               : packoffset(c0);
    uint3   gs_nLightPadding            : packoffset(c0.y);
};

StructuredBuffer<LIGHT> gs_aLights      : register(t0);
