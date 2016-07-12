struct PixelInfo
{
    uint albedoBuffer;
    half materialBuffer;
    half2 normalBuffer;
    uint next;

    half buffer;
};

#include "GEKEngine"

#include "GEKGlobal.hlsl"
#include "GEKUtility.hlsl"

uint encodeAlbedo(float4 value)
{
    value = min(max(value, 0.0f), 1.0f);
    value = value * 255 + 0.5f;
    value = floor(value);
    return (((uint)value.x) |
           (((uint)value.y) << 8) |
           (((uint)value.z) << 16) |
           (((uint)value.w) << 24));
}

float4 decodeAlbedo(uint value)
{
    return float4((float)(value & 0x000000ff) / 255,
                  (float)((value >> 8) & 0x000000ff) / 255,
                  (float)((value >> 16) & 0x000000ff) / 255,
                  (float)((value >> 24) & 0x000000ff) / 255);
}

half encodeHalf(float A/*one bit*/, float B/*5 bits*/, float C/*10 bits*/)//all inputs in 0-1 range
{
    float s = A * 2 - 1;// -1/+1
    float e = lerp(-13, 14, B);
    float f = lerp(1024, 2047, C);
    return s*f*pow(2, e);
}

half encodeMaterial(float roughness, float metalness)
{
    return encodeHalf(0.0, metalness, roughness);
}

void mainPixelProgram(InputPixel inputPixel)
{
    // final images will be in sRGB format and converted to linear automatically
    float4 albedo = (Resources::albedo.Sample(Global::linearWrapSampler, inputPixel.texCoord) * inputPixel.color);
    
    [branch]
    if(albedo.a < 0.5)
    {
        discard;
    }

    float3x3 viewBasis = getCoTangentFrame(inputPixel.viewPosition, inputPixel.viewNormal, inputPixel.texCoord);

    float3 normal;
    // assume normals are stored as 3Dc format, so generate the Z value
    normal.xy = ((Resources::normal.Sample(Global::linearWrapSampler, inputPixel.texCoord) * 2.0) - 1.0);
    normal.z = sqrt(1.0 - dot(normal.xy, normal.xy));
    normal = (mul(normal, viewBasis)) * (inputPixel.frontFacing ? 1.0 : -1.0);

    uint oldPixelCount;
    uint pixelCount = UnorderedAccess::pixelListBuffer.IncrementCounter();
    InterlockedExchange(UnorderedAccess::startIndexBuffer[inputPixel.position.xy], pixelCount, oldPixelCount);

    PixelInfo pixelInfo;
    pixelInfo.albedoBuffer = encodeAlbedo(albedo);
    pixelInfo.materialBuffer = encodeMaterial(Resources::roughness.Sample(Global::linearWrapSampler, inputPixel.texCoord),
        Resources::metalness.Sample(Global::linearWrapSampler, inputPixel.texCoord));
    pixelInfo.normalBuffer = encodeNormal(normal);
    pixelInfo.next = oldPixelCount;
    pixelInfo.buffer = 0;

    UnorderedAccess::pixelListBuffer[pixelCount] = pixelInfo;
}
