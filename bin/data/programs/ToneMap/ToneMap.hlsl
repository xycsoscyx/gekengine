#include GEKEngine

#include <GEKGlobal.hlsl>
#include <GEKUtility.hlsl>

float3 getExposedColor(float3 color, float averageLuminance, float threshold, out float exposure)
{
    exposure = 0.0;
    switch (Options::AutoExposure::Selection)
    {
    case Options::AutoExposure::None:
        exposure = Options::ConstantExposure;
        break;

    default:
        if (true)
        {
            float keyValue = 0.0;
            switch (Options::AutoExposure::Selection)
            {
            case Options::AutoExposure::Constant:
                keyValue = Options::ConstantKeyValue;
                break;

            case Options::AutoExposure::Average:
                keyValue = 1.03 - (2.0 / (2.0 + log10(averageLuminance + 1.0)));
                break;
            };

            const float linearexposure = (keyValue / averageLuminance);
            exposure = log2(max(linearexposure, Math::Epsilon));
        }

        break;
    };

    exposure -= threshold;
    return exp2(exposure) * color;
}

// Logarithmic mapping
float3 getToneMapLogarithmic(float3 color)
{
    const float pixelLuminance = GetLuminance(color);
    const float toneMappedLuminance = (log10(1.0 + pixelLuminance) / log10(1.0 + Options::WhiteLevel));
    return toneMappedLuminance * pow((color / pixelLuminance), Options::LuminanceSaturation);
}

// Drago's Logarithmic mapping
float3 getToneMapDragoLogarithmic(float3 color)
{
    const float pixelLuminance = GetLuminance(color);
    float toneMappedLuminance = log10(1.0 + pixelLuminance);
    //toneMappedLuminance /= log10(1.0 + Options::WhiteLevel);
    //toneMappedLuminance /= log10(2.0 + 8.0 * ((pixelLuminance / Options::WhiteLevel) * log10(Options::Bias) / log10(0.5f)));
    toneMappedLuminance /= log10(2.0 + 8.0 * (pow((pixelLuminance / Options::WhiteLevel), log10(Options::Bias) / log10(0.5f))));
	
	return toneMappedLuminance * pow(color / pixelLuminance, Options::LuminanceSaturation);
}

// Exponential mapping
float3 getToneMapExponential(float3 color)
{
    const float pixelLuminance = GetLuminance(color);
    float toneMappedLuminance = 1 - exp(-pixelLuminance / Options::WhiteLevel);
    return toneMappedLuminance * pow(color / pixelLuminance, Options::LuminanceSaturation);
}

// Applies Reinhard's basic tone mapping operator
float3 getToneMapReinhard(float3 color)
{
    const float pixelLuminance = GetLuminance(color);
    float toneMappedLuminance = pixelLuminance / (pixelLuminance + 1.0);
    return toneMappedLuminance * pow(color / pixelLuminance, Options::LuminanceSaturation);
}

// Applies Reinhard's modified tone mapping operator
float3 getToneMapReinhardModified(float3 color)
{
    const float pixelLuminance = GetLuminance(color);
    float toneMappedLuminance = pixelLuminance * (1.0 + pixelLuminance / (Options::WhiteLevel * Options::WhiteLevel)) / (1.0 + pixelLuminance);
    return toneMappedLuminance * pow(color / pixelLuminance, Options::LuminanceSaturation);
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
    static const float A = Options::ShoulderStrength;
	static const float B = Options::LinearStrength;
	static const float C = Options::LinearAngle;
	static const float D = Options::ToeStrength;
	static const float E = Options::ToeNumerator;
	static const float F = Options::ToeDenominator;
    return (((x*(A*x + C*B) + D*E) / (x*(A*x + B) + D*F)) - E / F);
}

// Applies the Uncharted 2 filmic tone mapping curve
float3 getToneMapFilmicU2(float3 color)
{
    const float3 numerator = getUncharted2Curve(color);
	static const float3 denominator = getUncharted2Curve(Options::LinearWhite);
    return (numerator / denominator);
}

float3 getToneMappedColor(float3 color, float averageLuminance, float threshold, out float exposure)
{
    const float pixelLuminance = GetLuminance(color);
    color = getExposedColor(color, averageLuminance, threshold, exposure);
    switch (Options::ToneMapper::Selection)
    {
    case Options::ToneMapper::Logarithmic:
        color = getToneMapLogarithmic(color);
        break;

    case Options::ToneMapper::Exponential:
        color = getToneMapExponential(color);
        break;

    case Options::ToneMapper::DragoLogarithmic:
        color = getToneMapDragoLogarithmic(color);
        break;

    case Options::ToneMapper::Reinhard:
        color = getToneMapReinhard(color);
        break;

    case Options::ToneMapper::ReinhardModified:
        color = getToneMapReinhardModified(color);
        break;

    case Options::ToneMapper::FilmicALU:
        color = getToneMapFilmicALU(color);
        break;

    case Options::ToneMapper::FilmicU2:
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