#include GEKEngine

#include <GEKGlobal.hlsl>
#include <GEKUtility.hlsl>

// https://mynameismjp.wordpress.com/2010/04/30/a-closer-look-at-tone-mapping/
#define TechniqueNone               0
#define TechniqueLogarithmic        1
#define TechniqueExponential        2
#define TechniqueDragoLogarithmic   3
#define TechniqueReinhard           4
#define TechniqueReinhardModified   5
#define TechniqueFilmicALU          6
#define TechniqueFilmicU2           7

namespace Defines
{
    static const uint TechniqueMode = TechniqueFilmicALU;
    static const uint AutoExposureMode = 2;
    static const float LuminanceSaturation = 1.0;
    static const float KeyValue = 0.18;
    static const float Exposure = 0.0;
    static const float WhiteLevel = 5.0;
    static const float Bias = 0.5;
    static const float ShoulderStrength = 0.2;
    static const float LinearStrength = 0.5;
    static const float LinearAngle = 0.42;
    static const float ToeStrength = 0.64;
    static const float ToeNumerator = 0.19;
    static const float ToeDenominator = 0.9;
    static const float LinearWhite = 11.2;
}; // namespace Defines

float3 getExposedColor(float3 color, float averageLuminance, float threshold, out float exposure)
{
    exposure = 0.0;
    if (Defines::AutoExposureMode >= 1 && Defines::AutoExposureMode <= 2)
    {
        float KeyValue = 0.0;
        if (Defines::AutoExposureMode == 1)
        {
            KeyValue = Defines::KeyValue;
        }
        else if (Defines::AutoExposureMode == 2)
        {
            KeyValue = 1.03 - (2.0 / (2.0 + log10(averageLuminance + 1.0)));
        }

        const float linearexposure = (KeyValue / averageLuminance);
        exposure = log2(max(linearexposure, Math::Epsilon));
    }
    else
    {
        exposure = Defines::Exposure;
    }

    exposure -= threshold;
    return exp2(exposure) * color;
}

// Logarithmic mapping
float3 getToneMapLogarithmic(float3 color)
{
    const float pixelLuminance = GetLuminance(color);
    const float toneMappedLuminance = (log10(1.0 + pixelLuminance) / log10(1.0 + Defines::WhiteLevel));
    return toneMappedLuminance * pow((color / pixelLuminance), Defines::LuminanceSaturation);
}

// Drago's Logarithmic mapping
float3 getToneMapDragoLogarithmic(float3 color)
{
    const float pixelLuminance = GetLuminance(color);
    float toneMappedLuminance = log10(1.0 + pixelLuminance);
    //toneMappedLuminance /= log10(1.0 + Defines::WhiteLevel);
    //toneMappedLuminance /= log10(2.0 + 8.0 * ((pixelLuminance / Defines::WhiteLevel) * log10(Defines::Bias) / log10(0.5f)));
    toneMappedLuminance /= log10(2.0 + 8.0 * (pow((pixelLuminance / Defines::WhiteLevel), log10(Defines::Bias) / log10(0.5f))));
	
	return toneMappedLuminance * pow(color / pixelLuminance, Defines::LuminanceSaturation);
}

// Exponential mapping
float3 getToneMapExponential(float3 color)
{
    const float pixelLuminance = GetLuminance(color);
    float toneMappedLuminance = 1 - exp(-pixelLuminance / Defines::WhiteLevel);
    return toneMappedLuminance * pow(color / pixelLuminance, Defines::LuminanceSaturation);
}

// Applies Reinhard's basic tone mapping operator
float3 getToneMapReinhard(float3 color)
{
    const float pixelLuminance = GetLuminance(color);
    float toneMappedLuminance = pixelLuminance / (pixelLuminance + 1.0);
    return toneMappedLuminance * pow(color / pixelLuminance, Defines::LuminanceSaturation);
}

// Applies Reinhard's modified tone mapping operator
float3 getToneMapReinhardModified(float3 color)
{
    const float pixelLuminance = GetLuminance(color);
    float toneMappedLuminance = pixelLuminance * (1.0 + pixelLuminance / (Defines::WhiteLevel * Defines::WhiteLevel)) / (1.0 + pixelLuminance);
    return toneMappedLuminance * pow(color / pixelLuminance, Defines::LuminanceSaturation);
}

// Applies the filmic curve from John Hable's presentation
float3 getToneMapFilmicALU(float3 color)
{
    color = max(0.0, (color - 0.004));
    color = (color * (6.2 * color + 0.5)) / (color * (6.2 * color + 1.7) + 0.06);
    return pow(color, 2.2f); // result has 1/2.2 baked in
}

// Function used by the Uncharte2D tone mapping curve
float3 getUncharted2Curve(float3 x)
{
    static const float A = Defines::ShoulderStrength;
	static const float B = Defines::LinearStrength;
	static const float C = Defines::LinearAngle;
	static const float D = Defines::ToeStrength;
	static const float E = Defines::ToeNumerator;
	static const float F = Defines::ToeDenominator;
    return (((x*(A*x + C*B) + D*E) / (x*(A*x + B) + D*F)) - E / F);
}

// Applies the Uncharted 2 filmic tone mapping curve
float3 getToneMapFilmicU2(float3 color)
{
    const float3 numerator = getUncharted2Curve(color);
	static const float3 denominator = getUncharted2Curve(Defines::LinearWhite);
    return (numerator / denominator);
}

float3 getToneMappedColor(float3 color, float averageLuminance, float threshold, out float exposure)
{
    const float pixelLuminance = GetLuminance(color);
    color = getExposedColor(color, averageLuminance, threshold, exposure);
    switch (Defines::TechniqueMode)
    {
    case TechniqueLogarithmic:
        color = getToneMapLogarithmic(color);
        break;

    case TechniqueExponential:
        color = getToneMapExponential(color);
        break;

    case TechniqueDragoLogarithmic:
        color = getToneMapDragoLogarithmic(color);
        break;

    case TechniqueReinhard:
        color = getToneMapReinhard(color);
        break;

    case TechniqueReinhardModified:
        color = getToneMapReinhardModified(color);
        break;

    case TechniqueFilmicALU:
        color = getToneMapFilmicALU(color);
        break;

    case TechniqueFilmicU2:
        color = getToneMapFilmicU2(color);
        break;
    };

    return color;
}

float3 mainPixelProgram(InputPixel inputPixel) : SV_TARGET0
{
    const float3 baseColor = Resources::inputBuffer[inputPixel.screen.xy].xyz;
    const float averageLuminance = Resources::averageLuminanceBuffer[0];

    float exposure = 0.0;
    return getToneMappedColor(baseColor, averageLuminance, 0.0, exposure);
}