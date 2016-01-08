#include "GEKEngine"

#include "GEKGlobal.h"

static const float Exposure = 0.0;
static const float WhiteLevel = 5.0;
static const float ShoulderStrength = 0.22f;
static const float LinearStrength = 0.30f;
static const float LinearAngle = 0.10f;
static const float ToeStrength = 0.20f;
static const float ToeNumerator = 0.01f;
static const float ToeDenominator = 0.30f;
static const float LinearWhite = 5.0;
static const float LuminanceSaturation = 1.0;
static const float Bias = 0.5;

// Approximates luminance from an RGB value
float CalculateLuminance(float3 color)
{
    return max(dot(color, float3(0.299f, 0.587f, 0.114f)), 0.0001f);
}

// Logarithmic mapping
float3 ToneMapLogarithmic(float3 color)
{
    float pixelLuminance = CalculateLuminance(color);
    float toneMappedLuminance = (log10(1 + pixelLuminance) / log10(1 + WhiteLevel));
    return toneMappedLuminance * pow((color / pixelLuminance), LuminanceSaturation);
}

// Drago's Logarithmic mapping
float3 ToneMapDragoLogarithmic(float3 color)
{
    float pixelLuminance = CalculateLuminance(color);
    float toneMappedLuminance = log10(1 + pixelLuminance);
    toneMappedLuminance /= log10(1 + WhiteLevel);
    toneMappedLuminance /= log10(2 + (8 * ((pixelLuminance / WhiteLevel) * log10(Bias) / log10(0.5))));
    return toneMappedLuminance * pow((color / pixelLuminance), LuminanceSaturation);
}

// Exponential mapping
float3 ToneMapExponential(float3 color)
{
    float pixelLuminance = CalculateLuminance(color);
    float toneMappedLuminance = (1 - exp(-pixelLuminance / WhiteLevel));
    return toneMappedLuminance * pow((color / pixelLuminance), LuminanceSaturation);
}

// Applies Reinhard's basic tone mapping operator
float3 ToneMapReinhard(float3 color)
{
    float pixelLuminance = CalculateLuminance(color);
    float toneMappedLuminance = (pixelLuminance / (pixelLuminance + 1));
    return toneMappedLuminance * pow((color / pixelLuminance), LuminanceSaturation);
}

// Applies Reinhard's modified tone mapping operator
float3 ToneMapReinhardModified(float3 color)
{
    float pixelLuminance = CalculateLuminance(color);
    float toneMappedLuminance = (pixelLuminance * (1.0 + (pixelLuminance / (WhiteLevel * WhiteLevel))) / (1.0 + pixelLuminance));
    return toneMappedLuminance * pow((color / pixelLuminance), LuminanceSaturation);
}

// Applies the filmic curve from John Hable's presentation
float3 ToneMapFilmicALU(float3 color)
{
    color = max(0, (color - 0.004f));
    color = (color * ((6.2f * color) + 0.5)) / (color * ((6.2f * color) + 1.7f) + 0.06f);
    // result has 1/2.2 baked in
    return pow(color, 2.2f);
}

// Function used by the Uncharted 2 tone mapping curve
float3 Uncharted2Function(float3 x)
{
    float A = ShoulderStrength;
    float B = LinearStrength;
    float C = LinearAngle;
    float D = ToeStrength;
    float E = ToeNumerator;
    float F = ToeDenominator;
    return (((x * ((A * x) + (C * B)) + (D * E)) / (x * ((A * x) + B) + (D * F))) - (E / F));
}

// Applies the Uncharted 2 filmic tone mapping curve
float3 ToneMapFilmicUncharted2(float3 color)
{
    float3 numerator = Uncharted2Function(color);
    float3 denominator = Uncharted2Function(LinearWhite);
    return numerator / denominator;
}

// Determines the color based on exposure settings
float3 CalcExposedColor(float3 color, float avgLuminance, float threshold)
{
    // Use exposure setting
    float exposure = Exposure;
    exposure -= threshold;
    return (exp2(exposure) * color);
}

float4 mainPixelProgram(InputPixel inputPixel) : SV_TARGET0
{
    float averageLuminance = Resources::luminanceBuffer.SampleLevel(Global::linearSampler, inputPixel.texCoord, 10);
    float3 color = Resources::luminatedBuffer.Sample(Global::pointSampler, inputPixel.texCoord).rgb;

    color = CalcExposedColor(color, averageLuminance, 0.0);
    color = ToneMapFilmicUncharted2(color);
    color = pow(color, (1.0 / 2.2f));

    return float4(color, 1);
}