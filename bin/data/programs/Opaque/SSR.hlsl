#include "gekengine.h"

Texture2D           gs_pAlbedoBuffer        : register(t1);
Texture2D<half2>    gs_pNormalBuffer        : register(t2);
Texture2D<float>    gs_pDepthBuffer         : register(t3);
Texture2D           gs_pInfoBuffer          : register(t4);
Texture2D           gs_pOutputBuffer        : register(t5);

float4 MainPixelProgram(INPUT kInput) : SV_TARGET
{
    float4 nScreen = gs_pOutputBuffer.Sample(gs_pPointSampler, kInput.texcoord);
    if (gs_pAlbedoBuffer.Sample(gs_pPointSampler, kInput.texcoord).w < 1.0f)
    {
        float4 nCenterInfo = gs_pInfoBuffer.Sample(gs_pPointSampler, kInput.texcoord);
            float nReflectivity = nCenterInfo.w;
        if (nReflectivity > 0.0f)
        {
            float nCenterDepth = gs_pDepthBuffer.Sample(gs_pPointSampler, kInput.texcoord);
            float3 nCenterPosition = GetViewPosition(kInput.texcoord, nCenterDepth);
                float3 nCenterNormal = DecodeNormal(gs_pNormalBuffer.Sample(gs_pPointSampler, kInput.texcoord));

                float3 nViewNormal = normalize(nCenterPosition);
                float3 nReflection = reflect(nViewNormal, nCenterNormal);
                float nViewDotNormal = dot(nViewNormal, nCenterNormal);

            // FRESNEL
            nReflectivity = (nReflectivity * pow((1.0 - abs(nViewDotNormal)), 1.0));

            float nStep = 0;

            float3 nRayPosition = nCenterPosition;
                float nRealStep = gs_nNumSteps;

            float4 nTexCoord;
            float nSampleDepth;
            float3 nSamplePosition;

            // FOLLOWING THE REFLECTED RAY 
            while (nStep < gs_nNumSteps)
            {
                nRayPosition += (nReflection.xyz * gs_nStepSize);
                nTexCoord = mul(gs_nProjectionMatrix, float4(nRayPosition, 1));
                nTexCoord.xy = ((((nTexCoord.xy * float2(1.0f, -1.0f)) / nTexCoord.w) * 0.5f) + 0.5f);
                nSampleDepth = gs_pDepthBuffer.Sample(gs_pPointSampler, nTexCoord.xy);
                nSamplePosition = GetViewPosition(nTexCoord.xy, nSampleDepth);
                if (nSamplePosition.z <= nRayPosition.z)
                {
                    nRealStep = nStep;
                    nStep = gs_nNumSteps;
                }
                else
                {
                    nStep++;
                }
            };

            // IF WE HAVE HIT SOMETHING - FOLLOWING THE RAY BACKWARD USING SMALL STEP FOR BETTER PRECISION
            if (nRealStep < gs_nNumSteps)
            {
                nStep = 0;
                while (nStep < gs_nNumStepsBack)
                {
                    nRayPosition -= (nReflection.xyz * gs_nStepSize / gs_nNumStepsBack);
                    nTexCoord = mul(gs_nProjectionMatrix, float4(nRayPosition, 1));
                    nTexCoord.xy = ((((nTexCoord.xy * float2(1.0f, -1.0f)) / nTexCoord.w) * 0.5f) + 0.5f);
                    nSampleDepth = gs_pDepthBuffer.Sample(gs_pPointSampler, nTexCoord.xy);
                    nSamplePosition = GetViewPosition(nTexCoord.xy, nSampleDepth);
                    if (nSamplePosition.z > nRayPosition.z)
                    {
                        nStep = gs_nNumStepsBack;
                    }
                    else
                    {
                        nStep++;
                    }
                };
            }

            // FADING REFLECTION DEPENDING ON RAY LENGTH
            nReflectivity = (nReflectivity * (1.0 - (nRealStep / gs_nNumSteps)));

            // FADING REFLECTION ON SCREEN BORDER // AVOIDS ABRUPT REFLECTION ENDINGS
            float2 tAtt = nTexCoord.xy;
                if (tAtt.x > 0.5) tAtt.x = (1.0 - tAtt.x);
            if (tAtt.y > 0.5) tAtt.y = (1.0 - tAtt.y);

            nReflectivity = (nReflectivity * clamp((tAtt.x * 10.0), 0.0, 1.0));
            nReflectivity = (nReflectivity * clamp((tAtt.y * 10.0), 0.0, 1.0));

            // AVOIDING REFLECTION OF OBJECTS IN FOREGROUND 
            nReflectivity = (nReflectivity * 1.0 / (1.0 + (abs(nSamplePosition.z - nRayPosition.z) * 20)));

            // COMBINING REFLECTION TO THE ORIGINAL PIXEL COLOR
            if (nReflectivity > 0.0)
            {
                nScreen = lerp(nScreen, gs_pOutputBuffer.Sample(gs_pPointSampler, nTexCoord.xy), nReflectivity);
            }
        }
    }

    return nScreen;
}
