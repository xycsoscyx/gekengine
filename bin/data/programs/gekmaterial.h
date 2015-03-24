cbuffer MATERIALBUFFER                  : register(b2)
{
    float4  gs_nMaterialColor           : packoffset(c0);
    bool    gs_bMaterialFullBright      : packoffset(c1);
    float3  gs_nMaterialPadding         : packoffset(c1.y);
};
