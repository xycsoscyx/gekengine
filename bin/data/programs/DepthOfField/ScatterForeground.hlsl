#include GEKEngine

#include <GEKGlobal.hlsl>
#include <GEKUtility.hlsl>

float calculateGaussianWeight(in float offset)
{
	static const float g = (1.0f / (sqrt(Math::Tau) * Defines::gaussianSigma));
	static const float d = rcp(2.0 * pow(Defines::gaussianSigma, 2.0));
	return (g * exp(-pow(offset, 2.0) * d)) / 2.0;
}

float4 mainPixelProgram(in InputPixel inputPixel) : SV_TARGET0
{
	float4 finalValue = 0.0;
	float totalWeight = 0.0;

	[unroll]
	for (int2 offset = -Defines::gaussianRadius; offset.x <= Defines::gaussianRadius; offset.x++)
	{
		[unroll]
		for (offset.y = -Defines::gaussianRadius; offset.y <= Defines::gaussianRadius; offset.y++)
		{
            const int2 sampleOffset = offset;
            const int2 sampleCoord = (inputPixel.screen.xy + sampleOffset);
            const float sampleWeight = calculateGaussianWeight(offset);

            const float sampleCircleOfConfusion = Resources::circleOfConfusion.Load(sampleCoord);
            const float3 sampleColor = Resources::foregroundBuffer.Load(sampleCoord);

			finalValue += (float4(sampleColor, saturate(-sampleCircleOfConfusion)) * sampleWeight);
			totalWeight += sampleWeight;
		}
	}

	return (finalValue * rcp(totalWeight + Math::Epsilon));
}