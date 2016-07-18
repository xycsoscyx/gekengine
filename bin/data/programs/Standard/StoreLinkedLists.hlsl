struct PixelInfo
{
    uint albedo;
    uint material;
    uint normal;
    uint next;
};

#include "GEKEngine"

#include "GEKGlobal.hlsl"
#include "GEKUtility.hlsl"

void mainPixelProgram(InputPixel inputPixel)
{
    // final images will be in sRGB format and converted to linear automatically
    float4 albedo = (Resources::albedo.Sample(Global::linearWrapSampler, inputPixel.texCoord) * inputPixel.color);

    float3x3 viewBasis = getCoTangentFrame(inputPixel.viewPosition, inputPixel.viewNormal, inputPixel.texCoord);

    float3 normal;
    // assume normals are stored as 3Dc format, so generate the Z value
    normal.xy = ((Resources::normal.Sample(Global::linearWrapSampler, inputPixel.texCoord) * 2.0) - 1.0);
    normal.z = sqrt(1.0 - dot(normal.xy, normal.xy));
    normal = (mul(normal, viewBasis)) * (inputPixel.frontFacing ? 1.0 : -1.0);

    uint nextPixelIndex;
    uint pixelIndex = UnorderedAccess::pixelListBuffer.IncrementCounter();
    InterlockedExchange(UnorderedAccess::startIndexBuffer[inputPixel.position.xy], pixelIndex, nextPixelIndex);

    PixelInfo pixelInfo;
    pixelInfo.albedo = packFloat4(albedo);
    pixelInfo.material = packFloat2(float2(Resources::roughness.Sample(Global::linearWrapSampler, inputPixel.texCoord),
                                           Resources::metalness.Sample(Global::linearWrapSampler, inputPixel.texCoord)));
    pixelInfo.normal = packFloat2(encodeNormal(normal));
    pixelInfo.next = nextPixelIndex;

    UnorderedAccess::pixelListBuffer[pixelIndex] = pixelInfo;
}
