#include "GEKFilter"

#include "GEKGlobal.hlsl"
#include "GEKUtility.hlsl"

static const float width = Shader::targetSize.x;
static const float height = Shader::targetSize.y;
static const float2 texel = Shader::pixelSize;

int samples = 3; //samples on the first ring
int rings = 5; //ring count

float range = 4.0; //focal range
float maxblur = 1.25; //clamp value of max blur

float threshold = 0.5; //highlight threshold;
float gain = 10.0; //highlight gain;

float bias = 0.4; //bokeh edge bias
float fringe = 0.5; //bokeh chromatic aberration/fringing

bool noise = true; //use noise instead of pattern for sample dithering
float namount = 0.0001; //dither amount

bool depthblur = true; //blur the depth buffer?
float dbsize = 2.0; //depthblursize

/*
    next part is experimental
    not looking good with small sample and ring count
    looks okay starting from samples = 4, rings = 4
*/
bool pentagon = false; //use pentagon as bokeh shape?
float feather = 0.4; //pentagon shape feather

float getPentagonalShape(float2 texCoord) //pentagonal shape
{
    static const float scale = float(rings) - 1.3;
    static const float4  HS0 = float4(1.0, 0.0, 0.0, 1.0);
    static const float4  HS1 = float4(0.309016994, 0.951056516, 0.0, 1.0);
    static const float4  HS2 = float4(-0.809016994, 0.587785252, 0.0, 1.0);
    static const float4  HS3 = float4(-0.809016994, -0.587785252, 0.0, 1.0);
    static const float4  HS4 = float4(0.309016994, -0.951056516, 0.0, 1.0);
    static const float4  HS5 = float4(0.0, 0.0, 1.0, 1.0);

    static const float4  one = 1.0;

    float4 P = float4(texCoord, scale.xx);

    float4 dist = 0.0;
    float inorout = -4.0;

    dist.x = dot(P, HS0);
    dist.y = dot(P, HS1);
    dist.z = dot(P, HS2);
    dist.w = dot(P, HS3);

    dist = smoothstep(-feather, feather, dist);

    inorout += dot(dist, one);

    dist.x = dot(P, HS4);
    dist.y = HS5.w - abs(P.z);

    dist = smoothstep(-feather, feather, dist);
    inorout += dist.x;

    return clamp(inorout, 0.0, 1.0);
}

float getBlurredDepth(float2 texCoord) //blurring depth
{
    float d = 0.0;
    float kernel[9];
    float2 offset[9];

    float2 wh = texel * dbsize;

    offset[0] = float2(-wh.x, -wh.y);
    offset[1] = float2(0.0, -wh.y);
    offset[2] = float2(wh.x, -wh.y);

    offset[3] = float2(-wh.x, 0.0);
    offset[4] = float2(0.0, 0.0);
    offset[5] = float2(wh.x, 0.0);

    offset[6] = float2(-wh.x, wh.y);
    offset[7] = float2(0.0, wh.y);
    offset[8] = float2(wh.x, wh.y);

    kernel[0] = 1.0 / 16.0;   kernel[1] = 2.0 / 16.0;   kernel[2] = 1.0 / 16.0;
    kernel[3] = 2.0 / 16.0;   kernel[4] = 4.0 / 16.0;   kernel[5] = 2.0 / 16.0;
    kernel[6] = 1.0 / 16.0;   kernel[7] = 2.0 / 16.0;   kernel[8] = 1.0 / 16.0;

    for (int i = 0; i < 9; i++)
    {
        float tmp = Resources::depthBuffer.SampleLevel(Global::pointSampler, texCoord + offset[i], 0).r;
        d += tmp * kernel[i];
    }

    return d;
}


float3 color(float2 texCoord, float blur) //processing the sample
{
    float3 col = 0.0;

    col.r = Resources::finalCopy.SampleLevel(Global::pointSampler, texCoord + float2(0.0, 1.0)*texel*fringe*blur, 0).r;
    col.g = Resources::finalCopy.SampleLevel(Global::pointSampler, texCoord + float2(-0.866, -0.5)*texel*fringe*blur, 0).g;
    col.b = Resources::finalCopy.SampleLevel(Global::pointSampler, texCoord + float2(0.866, -0.5)*texel*fringe*blur, 0).b;

    static const float3 lumcoeff = float3(0.299, 0.587, 0.114);
    float lum = dot(col.rgb, lumcoeff);
    float thresh = max((lum - threshold)*gain, 0.0);
    return col + lerp(0.0, col, thresh*blur);
}

float2 rand(in float2 coord) //generating noise/pattern texture for dithering
{
    float noiseX = ((frac(1.0 - coord.x*(width / 2.0))*0.25) + (frac(coord.y*(height / 2.0))*0.75))*2.0 - 1.0;
    float noiseY = ((frac(1.0 - coord.x*(width / 2.0))*0.75) + (frac(coord.y*(height / 2.0))*0.25))*2.0 - 1.0;
    if (noise)
    {
        noiseX = clamp(frac(sin(dot(coord, float2(12.9898, 78.233))) * 43758.5453), 0.0, 1.0)*2.0 - 1.0;
        noiseY = clamp(frac(sin(dot(coord, float2(12.9898, 78.233)*2.0)) * 43758.5453), 0.0, 1.0)*2.0 - 1.0;
    }

    return float2(noiseX, noiseY);
}

float3 mainPixelProgram(InputPixel inputPixel) : SV_TARGET0
{
    float depth = Resources::depthBuffer.SampleLevel(Global::pointSampler, inputPixel.texCoord, 0).x;
    if (depthblur)
    {
        depth = getBlurredDepth(inputPixel.texCoord);
    }

    float focalDepth = Resources::averageFocalDepth[0];
    float blur = clamp((abs(depth - focalDepth) / range)*100.0, -maxblur, maxblur);
    float2 noise = rand(inputPixel.texCoord)*namount*blur;

    float noiseWidth = (1.0 / width)*blur + noise.x;
    float noiseHeight = (1.0 / height)*blur + noise.y;

    float3 col = Resources::finalCopy.SampleLevel(Global::pointSampler, inputPixel.texCoord, 0);
    float s = 1.0;

    for (float i = 1.0; i <= rings; i++)
    {
        float ringsamples = i * float(samples);
        for (float j = 0.0; j < ringsamples; j++)
        {
            float step = Math::Tau / float(ringsamples);
            float pw = (cos(j*step)*i);
            float ph = (sin(j*step)*i);
            float p = 1.0;
            if (pentagon)
            {
                p = getPentagonalShape(float2(pw, ph));
            }

            col += color(inputPixel.texCoord + float2(pw*noiseWidth, ph*noiseHeight), blur)*lerp(1.0, (i / float(rings)), bias)*p;
            s += 1.0*lerp(1.0, (i / float(rings)), bias)*p;
        }
    }

    return (col / s);
}