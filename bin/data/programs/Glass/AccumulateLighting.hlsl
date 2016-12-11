#include GEKEngine

#include <GEKGlobal.hlsl>
#include <GEKUtility.hlsl>
#include <GEKLighting.hlsl>

// 4x4 bicubic filter using 4 bilinear texture lookups 
// See GPU Gems 2: "Fast Third-Order Texture Filtering", Sigg & Hadwiger:
// http://http.developer.nvidia.com/GPUGems2/gpugems2_chapter20.html


// w0, w1, w2, and w3 are the four cubic B-spline basis functions
float w0(float a)
{
    return (1.0 / 6.0)*(a*(a*(-a + 3.0) - 3.0) + 1.0);
}

float w1(float a)
{
    return (1.0 / 6.0)*(a*a*(3.0*a - 6.0) + 4.0);
}

float w2(float a)
{
    return (1.0 / 6.0)*(a*(a*(-3.0*a + 3.0) + 3.0) + 1.0);
}

float w3(float a)
{
    return (1.0 / 6.0)*(a*a*a);
}

// g0 and g1 are the two amplitude functions
float g0(float a)
{
    return w0(a) + w1(a);
}

float g1(float a)
{
    return w2(a) + w3(a);
}

// h0 and h1 are the two offset functions
float h0(float a)
{
    return -1.0 + w1(a) / (w0(a) + w1(a));
}

float h1(float a)
{
    return 1.0 + w3(a) / (w2(a) + w3(a));
}

float3 getCubicFilter(float2 texCoord, int glassLevel)
{
    int levelCount;
    float2 glassSize;
    Resources::glassBuffer.GetDimensions(glassLevel, glassSize.x, glassSize.y, levelCount);
    float2 glassPixelSize = 1.0 / glassSize;

    texCoord = texCoord*glassSize + 0.5;
    float2 iuv = floor(texCoord);
    float2 fuv = frac(texCoord);

    float g0x = g0(fuv.x);
    float g1x = g1(fuv.x);
    float h0x = h0(fuv.x);
    float h1x = h1(fuv.x);
    float h0y = h0(fuv.y);
    float h1y = h1(fuv.y);

    float2 p0 = (float2(iuv.x + h0x, iuv.y + h0y) - 0.5) * glassPixelSize;
    float2 p1 = (float2(iuv.x + h1x, iuv.y + h0y) - 0.5) * glassPixelSize;
    float2 p2 = (float2(iuv.x + h0x, iuv.y + h1y) - 0.5) * glassPixelSize;
    float2 p3 = (float2(iuv.x + h1x, iuv.y + h1y) - 0.5) * glassPixelSize;

    return
        g0(fuv.y) * (g0x * Resources::glassBuffer.SampleLevel(Global::LinearClampSampler, p0, glassLevel) + g1x * Resources::glassBuffer.SampleLevel(Global::LinearClampSampler, p1, glassLevel)) +
        g1(fuv.y) * (g0x * Resources::glassBuffer.SampleLevel(Global::LinearClampSampler, p2, glassLevel) + g1x * Resources::glassBuffer.SampleLevel(Global::LinearClampSampler, p3, glassLevel));
}

float3 mainPixelProgram(InputPixel inputPixel) : SV_TARGET0
{
    // final images will be sRGB format and converted to linear automatically
    const float4 albedo = Resources::albedo.Sample(Global::LinearWrapSampler, inputPixel.texCoord);

    float3 surfacePosition = inputPixel.position;

    const float3x3 viewBasis = float3x3(inputPixel.tangent, inputPixel.biTangent, inputPixel.normal);
    //float3x3 viewBasis = getCoTangentFrame(inputPixel.position, inputPixel.normal, inputPixel.texCoord);

    float3 surfaceNormal;
    // assume normals are stored as 3Dc format, so generate the Z value
    surfaceNormal.xy = Resources::normal.Sample(Global::LinearWrapSampler, inputPixel.texCoord);
    surfaceNormal.xy = ((surfaceNormal.xy * 2.0) - 1.0);
    surfaceNormal.y *= -1.0; // grrr, inverted y axis, WHY?!?
    surfaceNormal.z = sqrt(1.0 - dot(surfaceNormal.xy, surfaceNormal.xy));
    surfaceNormal = mul(surfaceNormal, viewBasis);
    surfaceNormal = normalize(inputPixel.isFrontFacing ? surfaceNormal : -surfaceNormal);

    float3 materialAlbedo = albedo.rgb;
    float materialRoughness = Resources::roughness.Sample(Global::LinearWrapSampler, inputPixel.texCoord);
    float materialMetallic = Resources::metallic.Sample(Global::LinearWrapSampler, inputPixel.texCoord);

    float2 glassTexCoord = (inputPixel.screen.xy * Shader::TargetPixelSize);

    float glassRoughness = (materialRoughness * 5.0);
    int glassLevel = int(floor(glassRoughness));
    float3 glassColor = getCubicFilter(glassTexCoord, glassLevel);

    [branch]
    if (materialRoughness < 1.0)
    {
        float lerpLevel = frac(glassRoughness);
        glassColor = lerp(glassColor, getCubicFilter(glassTexCoord, (glassLevel + 1)), lerpLevel);
    }

    float3 surfaceIrradiance = getSurfaceIrradiance(inputPixel.screen.xy, surfacePosition, surfaceNormal, materialAlbedo, 0.1, 1.0);
    return surfaceIrradiance + glassColor * materialAlbedo;
}
