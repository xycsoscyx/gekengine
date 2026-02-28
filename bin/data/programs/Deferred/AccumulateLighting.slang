#include <GEKEngine>

#include <GEKGlobal.slang>
#include <GEKUtility.slang>
#include <GEKLighting.slang>

[shader("fragment")]
float3 mainPixelProgram(InputPixel inputPixel) : SV_TARGET0
{
	const uint2 screenCoord = uint2(inputPixel.screen.xy);
	float2 materialInfo = Resources::materialBuffer[screenCoord];
    float materialRoughness = materialInfo.x;
    float materialMetallic = materialInfo.y;

    float surfaceDepth = Resources::depthBuffer[screenCoord];
    float3 surfacePosition = GetPositionFromSampleDepth(inputPixel.texCoord, surfaceDepth);
    float3 surfaceNormal = GetDecodedNormal(Resources::normalBuffer[screenCoord]);

    float3 surfaceIrradiance = getSurfaceIrradiance(inputPixel.screen.xy, surfacePosition, surfaceNormal, materialAlbedo, materialRoughness, materialMetallic);
	float surfaceAmbient = Resources::ambientBuffer[screenCoord];
    return surfaceIrradiance;
}
