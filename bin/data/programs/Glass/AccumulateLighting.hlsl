#include GEKEngine

#include <GEKGlobal.hlsl>
#include <GEKUtility.hlsl>
#include <GEKLighting.hlsl>

float3 SampleBilinear(in float2 screenCoord, in int mipMapLevel)
{
    return Resources::glassBuffer.Load(int3((screenCoord * Shader::TargetPixelSize), mipMapLevel));
}

// from http://www.java-gaming.org/index.php?topic=35123.0
float4 cubic(float v)
{
    float4 n = float4(1.0, 2.0, 3.0, 4.0) - v;
    float4 s = n * n * n;
    float x = s.x;
    float y = s.y - 4.0 * s.x;
    float z = s.z - 4.0 * s.y + 6.0 * s.x;
    float w = 6.0 - x - y - z;
    return float4(x, y, z, w) * (1.0 / 6.0);
}

// Catmull-Rom spline actually passes through control points
float4 cubicCatmullRom(float x) // cubic_catmullrom(float x)
{
    const float s = 0.5; // potentially adjustable parameter
    float x2 = x * x;
    float x3 = x2 * x;
    float4 w;
    w.x = -s*x3 + 2 * s*x2 - s*x + 0;
    w.y = (2 - s)*x3 + (s - 3)*x2 + 1;
    w.z = (s - 2)*x3 + (3 - 2 * s)*x2 + s*x + 0;
    w.w = s*x3 - s*x2 + 0;
    return w;
}

float3 SampleBicubic(in float2 screenCoord, in int mipMapLevel)
{
    float mipMapScale = pow(2.0, mipMapLevel);
    screenCoord *= (1.0 / mipMapScale);

    screenCoord -= 0.5;

    float2 screenCoordFraction = frac(screenCoord);
    screenCoord -= screenCoordFraction;

    float4 xCubicFactor = cubic(screenCoordFraction.x);
    float4 yCubicFactor = cubic(screenCoordFraction.y);

    float4 centerCoord = screenCoord.xxyy + float2(-0.5, +1.5).xyxy;

    float4 scaleFactor = float4(xCubicFactor.xz + xCubicFactor.yw, yCubicFactor.xz + yCubicFactor.yw);
    float4 texCoord = centerCoord + float4(xCubicFactor.yw, yCubicFactor.yw) / scaleFactor;

    float3 texSample0 = Resources::glassBuffer.Load(int3(texCoord.xz, mipMapLevel));
    float3 texSample1 = Resources::glassBuffer.Load(int3(texCoord.yz, mipMapLevel));
    float3 texSample2 = Resources::glassBuffer.Load(int3(texCoord.xw, mipMapLevel));
    float3 texSample3 = Resources::glassBuffer.Load(int3(texCoord.yw, mipMapLevel));

    float scaleFactorX = scaleFactor.x / (scaleFactor.x + scaleFactor.y);
    float scaleFactorY = scaleFactor.z / (scaleFactor.z + scaleFactor.w);

    return lerp(
        lerp(texSample3, texSample2, scaleFactorX),
        lerp(texSample1, texSample0, scaleFactorX), scaleFactorY);
}
// http://vec3.ca/bicubic-filtering-in-fewer-taps/
float3 SampleBicubicOptimized(in float2 screenCoord, in int mipMapLevel)
{
    float mipMapScale = pow(2.0, mipMapLevel);
    screenCoord *= (1.0 / mipMapScale);

    // Calculate the center of the texel to avoid any filtering
    float2 centerCoord = floor(screenCoord - 0.5) + 0.5;
    float2 screenCoordFraction = screenCoord - centerCoord;
    float2 screenCoordFractionSquared = screenCoordFraction * screenCoordFraction;
    float2 screenCoordFractionCubed = screenCoordFraction * screenCoordFractionSquared;

    // Calculate the filter weights (B-Spline Weighting Function)
    float2 bSplineWeight0 = screenCoordFractionSquared - 0.5 * (screenCoordFractionCubed + screenCoordFraction);
    float2 bSplineWeight1 = 1.5 * screenCoordFractionCubed - 2.5 * screenCoordFractionSquared + 1.0;
    float2 bSplineWeight3 = 0.5 * (screenCoordFractionCubed - screenCoordFractionSquared);
    float2 bSplineWeight2 = 1.0 - bSplineWeight0 - bSplineWeight1 - bSplineWeight3;

    // Calculate the texture coordinates
    float2 scalingFactor0 = bSplineWeight0 + bSplineWeight1;
    float2 scalingFactor1 = bSplineWeight2 + bSplineWeight3;

    float2 coordWeightFactor0 = bSplineWeight1 / (bSplineWeight0 + bSplineWeight1);
    float2 coordWeightFactor1 = bSplineWeight3 / (bSplineWeight2 + bSplineWeight3);

    float2 texCoord0 = centerCoord - 1.0 + coordWeightFactor0;
    float2 texCoord1 = centerCoord + 1.0 + coordWeightFactor1;

    // Sample the texture
    return 
        (Resources::glassBuffer.Load(int3(float2(texCoord0.x, texCoord0.y), mipMapLevel)) * scalingFactor0.x
       + Resources::glassBuffer.Load(int3(float2(texCoord1.x, texCoord0.y), mipMapLevel)) * scalingFactor1.x) * scalingFactor0.y
      + (Resources::glassBuffer.Load(int3(float2(texCoord0.x, texCoord1.y), mipMapLevel)) * scalingFactor0.x
       + Resources::glassBuffer.Load(int3(float2(texCoord1.x, texCoord1.y), mipMapLevel)) * scalingFactor1.x) * scalingFactor1.y;
}

// https://gist.github.com/TheRealMJP/c83b8c0f46b63f3a88a5986f4fa982b1
float3 SampleTextureCatmullRom(in float2 screenCoord, in int mipMapLevel)
{
    float mipMapScale = pow(2.0, mipMapLevel);
    screenCoord *= (1.0 / mipMapScale);

    // We're going to sample a a 4x4 grid of texels surrounding the target UV coordinate. We'll do this by rounding
    // down the sample location to get the exact center of our "starting" texel. The starting texel will be at
    // location [1, 1] in the grid, where [0, 0] is the top left corner.
    float2 centerCoord = floor(screenCoord - 0.5) + 0.5;

    // Compute the fractional offset from our starting texel to our original sample location, which we'll
    // feed into the Catmull-Rom spline function to get our filter weights.
    float2 sceenCoordFraction = screenCoord - centerCoord;

    // Compute the Catmull-Rom weights using the fractional offset that we calculated earlier.
    // These equations are pre-expanded based on our knowledge of where the texels will be located,
    // which lets us avoid having to evaluate a piece-wise function.
    float2 catmullRomWeight0 = sceenCoordFraction * (-0.5 + sceenCoordFraction * (1.0 - 0.5 * sceenCoordFraction));
    float2 catmullRomWeight1 = 1.0 + sceenCoordFraction * sceenCoordFraction * (-2.5 + 1.5 * sceenCoordFraction);
    float2 catmullRomWeight2 = sceenCoordFraction * (0.5 + sceenCoordFraction * (2.0 - 1.5 * sceenCoordFraction));
    float2 catmullRomWeight3 = sceenCoordFraction * sceenCoordFraction * (-0.5 + 0.5 * sceenCoordFraction);

    // Work out weighting factors and sampling offsets that will let us use bilinear filtering to
    // simultaneously evaluate the middle 2 samples from the 4x4 grid.
    float2 catmullRomWeight12 = catmullRomWeight1 + catmullRomWeight2;
    float2 catmullRomOffset12 = catmullRomWeight2 / (catmullRomWeight1 + catmullRomWeight2);

    // Compute the final UV coordinates we'll use for sampling the texture
    float2 texCoord0 = centerCoord - 1;
    float2 texCoord3 = centerCoord + 2;
    float2 screenCoord12 = centerCoord + catmullRomOffset12;

    float3 result = 0.0f;
    result += Resources::glassBuffer.Load(int3(float2(texCoord0.x, texCoord0.y), mipMapLevel)) * catmullRomWeight0.x * catmullRomWeight0.y;
    result += Resources::glassBuffer.Load(int3(float2(screenCoord12.x, texCoord0.y), mipMapLevel)) * catmullRomWeight12.x * catmullRomWeight0.y;
    result += Resources::glassBuffer.Load(int3(float2(texCoord3.x, texCoord0.y), mipMapLevel)) * catmullRomWeight3.x * catmullRomWeight0.y;

    result += Resources::glassBuffer.Load(int3(float2(texCoord0.x, screenCoord12.y), mipMapLevel)) * catmullRomWeight0.x * catmullRomWeight12.y;
    result += Resources::glassBuffer.Load(int3(float2(screenCoord12.x, screenCoord12.y), mipMapLevel)) * catmullRomWeight12.x * catmullRomWeight12.y;
    result += Resources::glassBuffer.Load(int3(float2(texCoord3.x, screenCoord12.y), mipMapLevel)) * catmullRomWeight3.x * catmullRomWeight12.y;

    result += Resources::glassBuffer.Load(int3(float2(texCoord0.x, texCoord3.y), mipMapLevel)) * catmullRomWeight0.x * catmullRomWeight3.y;
    result += Resources::glassBuffer.Load(int3(float2(screenCoord12.x, texCoord3.y), mipMapLevel)) * catmullRomWeight12.x * catmullRomWeight3.y;
    result += Resources::glassBuffer.Load(int3(float2(texCoord3.x, texCoord3.y), mipMapLevel)) * catmullRomWeight3.x * catmullRomWeight3.y;

    return result;
}

float3 mainPixelProgram(InputPixel inputPixel) : SV_TARGET0
{
    // final images will be sRGB format and converted to linear automatically
    const float4 albedo = Resources::albedo.Sample(Global::TextureSampler, inputPixel.texCoord.xy);

    float3 surfacePosition = inputPixel.position;

    const float3x3 viewBasis = float3x3(inputPixel.tangent, inputPixel.biTangent, inputPixel.normal);
    //float3x3 viewBasis = getCoTangentFrame(inputPixel.position, inputPixel.normal, inputPixel.screenCoord);

    float3 surfaceNormal;
    // assume normals are stored as 3Dc format, so generate the Z value
    surfaceNormal.xy = Resources::normal.Sample(Global::TextureSampler, inputPixel.texCoord.xy).xy;
    surfaceNormal.xy = ((surfaceNormal.xy * 2.0) - 1.0);
    surfaceNormal.y *= -1.0; // grrr, inverted y axis, WHY?!?
    surfaceNormal.z = sqrt(1.0 - dot(surfaceNormal.xy, surfaceNormal.xy));
    surfaceNormal = mul(surfaceNormal, viewBasis);
    surfaceNormal = normalize(inputPixel.isFrontFacing ? surfaceNormal : -surfaceNormal);

    float3 materialAlbedo = albedo.rgb;
    float materialRoughness = Resources::roughness.Sample(Global::TextureSampler, inputPixel.texCoord.xy);
    float materialMetallic = Resources::metallic.Sample(Global::TextureSampler, inputPixel.texCoord.xy);
    float3 surfaceIrradiance = getSurfaceIrradiance(inputPixel.screen.xy, surfacePosition, surfaceNormal, materialAlbedo, 0.25, 0.75);

    float glassLevel = (materialRoughness * 5.0);
    int mipMapLevel = floor(glassLevel);
    float glassFactor = (glassLevel - mipMapLevel);

    float2 screenCoord = inputPixel.screen.xy * Shader::TargetPixelSize;
    float3 glassLevelA = SampleBicubic(inputPixel.screen.xy, mipMapLevel);
    float3 glassLevelB = SampleBicubic(inputPixel.screen.xy, (mipMapLevel + 1));
    float3 glassColor = lerp(glassLevelA, glassLevelB, glassFactor);
    return (surfaceIrradiance + (glassColor * materialAlbedo));
}
