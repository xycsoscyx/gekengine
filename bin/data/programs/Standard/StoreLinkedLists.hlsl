struct PixelInfo
{
    uint albedo;
    uint materialNormal;
    float depth;
    int next;
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
    normal.xy = Resources::normal.Sample(Global::linearWrapSampler, inputPixel.texCoord);
    normal.xy = ((normal.xy * 2.0) - 1.0);
    normal.z = sqrt(1.0 - dot(normal.xy, normal.xy));
    normal = mul(normal, viewBasis);
    normal = (inputPixel.frontFacing ? normal : -normal);

    int nextPixelIndex;
    int pixelIndex = UnorderedAccess::pixelListBuffer.IncrementCounter();
    InterlockedExchange(UnorderedAccess::startIndexBuffer[inputPixel.position.xy], pixelIndex, nextPixelIndex);

    PixelInfo pixelInfo;
    pixelInfo.albedo = packFloat4(albedo);
    pixelInfo.materialNormal = packFloat4(float4(Resources::roughness.Sample(Global::linearWrapSampler, inputPixel.texCoord),
                                                 Resources::metalness.Sample(Global::linearWrapSampler, inputPixel.texCoord),
                                                 encodeNormal(normal)));
    pixelInfo.depth = inputPixel.viewPosition.z;
    pixelInfo.next = nextPixelIndex;

    UnorderedAccess::pixelListBuffer[pixelIndex] = pixelInfo;
}
