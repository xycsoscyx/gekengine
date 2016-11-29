#include GEKEngine

#include <GEKGlobal.hlsl>
#include <GEKUtility.hlsl>
#include <GEKLighting.hlsl>

float3 mainPixelProgram(InputPixel inputPixel) : SV_TARGET0
{
    float3 materialAlbedo = Resources::albedoBuffer[inputPixel.screen.xy];
	float2 materialInfo = Resources::materialBuffer[inputPixel.screen.xy];
    float materialRoughness = materialInfo.x;
    float materialMetallic = materialInfo.y;

    float surfaceDepth = Resources::depthBuffer[inputPixel.screen.xy];
    float3 surfacePosition = getPositionFromSample(inputPixel.texCoord, surfaceDepth);
    float3 surfaceNormal = getDecodedNormal(Resources::normalBuffer[inputPixel.screen.xy]);

    float3 surfaceIrradiance = getSurfaceIrradiance(inputPixel.screen.xy, surfacePosition, surfaceNormal, materialAlbedo, materialRoughness, materialMetallic);
	float surfaceAmbient = Resources::ambientBuffer[inputPixel.screen.xy];
	return (surfaceIrradiance + (materialAlbedo * surfaceAmbient * 0.005));
}
