struct PixelInfo
{
    uint albedoBuffer;
    half materialBuffer;
    half2 normalBuffer;
    half buffer;
    uint next;
};

#include "GEKEngine"

#include "GEKGlobal.hlsl"
#include "GEKUtility.hlsl"

uint encodeAlbedo(float3 albedo)
{
    return 0;
}

half encodeMaterial(float roughness, float metalness)
{
    return 0;
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

    PixelInfo pixelInfo;
    pixelInfo.albedoBuffer = encodeAlbedo(albedo.xyz);
    pixelInfo.materialBuffer = encodeMaterial(Resources::roughness.Sample(Global::linearWrapSampler, inputPixel.texCoord), 
                                              Resources::metalness.Sample(Global::linearWrapSampler, inputPixel.texCoord));
    pixelInfo.normalBuffer = encodeNormal(normal);
    pixelInfo.buffer = 0;

    uint oldPixelCount;
    uint pixelCount = UnorderedAccess::pixelListBuffer.IncrementCounter();
    InterlockedExchange(UnorderedAccess::startIndexBuffer[inputPixel.position.xy], pixelCount, oldPixelCount);

    pixelInfo.next = oldPixelCount;
    UnorderedAccess::pixelListBuffer[pixelCount] = pixelInfo;
}
