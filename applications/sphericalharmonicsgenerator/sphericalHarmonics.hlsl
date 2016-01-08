TextureCube environmentMap : register(t0);
SamplerState linearSampler : register(s0);
RWBuffer<float3> outputCoefficients : register(u0);

[numthreads(1, 1, 1)]
void mainComputeProgram(void)
{
    float3 coefficients[9] = 
    {
        float3(0.0, 0.0, 0.0),
        float3(0.0, 0.0, 0.0),
        float3(0.0, 0.0, 0.0),
        float3(0.0, 0.0, 0.0),
        float3(0.0, 0.0, 0.0),
        float3(0.0, 0.0, 0.0),
        float3(0.0, 0.0, 0.0),
        float3(0.0, 0.0, 0.0),
        float3(0.0, 0.0, 0.0),
    };

    float2 pixelSize;
    environmentMap.GetDimensions(pixelSize.x, pixelSize.y);
    pixelSize *= 0.5;

    float size = pixelSize.x;
    // step between two texels for range [0, 1]
    float inverseSize = 1.0 / pixelSize.x;
    // initial negative bound for range [-1, 1]
    float negativeBound = -1.0 + inverseSize;
    // step between two texels for range [-1, 1]
    float inverseSizeBy2 = 2.0 / pixelSize.x;

    float totalWeight = 0.0;
    for (int face = 0; face < 6; face++)
    {
        for (float y = 0.0; y < size; y += 1.0)
        {
            // texture coordinate V in range [-1 to 1]
            const float u = negativeBound + y * inverseSizeBy2;
            for (float x = 0.0; x < size; x += 1.0)
            {
                // texture coordinate U in range [-1 to 1]
                const float v = negativeBound + x * inverseSizeBy2;
                float3 normal;
                switch (face)
                {
                case 0:
                    normal.x = 1.0;
                    normal.y = 1.0 - (inverseSizeBy2 * y + inverseSize);
                    normal.z = 1.0 - (inverseSizeBy2 * x + inverseSize);
                    normal = -normal;
                    break;

                case 1:
                    normal.x = -1.0;
                    normal.y = 1.0 - (inverseSizeBy2 * y + inverseSize);
                    normal.z = -1.0 + (inverseSizeBy2 * x + inverseSize);
                    normal = -normal;
                    break;

                case 2:
                    normal.x = -1.0 + (inverseSizeBy2 * x + inverseSize);
                    normal.y = 1.0;
                    normal.z = -1.0 + (inverseSizeBy2 * y + inverseSize);
                    normal = -normal;
                    break;

                case 3:
                    normal.x = -1.0 + (inverseSizeBy2 * x + inverseSize);
                    normal.y = -1.0;
                    normal.z = 1.0 - (inverseSizeBy2 * y + inverseSize);
                    normal = -normal;
                    break;

                case 4:
                    normal.x = -1.0 + (inverseSizeBy2 * x + inverseSize);
                    normal.y = 1.0 - (inverseSizeBy2 * y + inverseSize);
                    normal.z = 1.0;
                    break;

                case 5:
                    normal.x = 1.0 - (inverseSizeBy2 * x + inverseSize);
                    normal.y = 1.0 - (inverseSizeBy2 * y + inverseSize);
                    normal.z = -1.0;
                    break;
                };

                normal = normalize(normal);

                // scale factor depending on distance from center of the face
                const float edgeDifference = 4.0f / ((1.0 + v*v + u*u) * sqrt(1.0 + v*v + u*u));
                totalWeight += edgeDifference;

                // get color from texture and map to range [0, 1]
                float3 color = environmentMap.SampleLevel(linearSampler, normal, 0).xyz * edgeDifference;

                // calculate coefficients for first 3 bands of spherical harmonics
                float p_0_0 = 0.282094791773878140;
                float p_1_0 = 0.488602511902919920 * normal.z;
                float p_1_1 = -0.488602511902919920;
                float p_2_0 = 0.946174695757560080 * normal.z * normal.z - 0.315391565252520050;
                float p_2_1 = -1.092548430592079200 * normal.z;
                float p_2_2 = 0.546274215296039590;
                coefficients[0] += (p_0_0) * color;
                coefficients[1] += (p_1_1 * normal.y) * color;
                coefficients[2] += (p_1_0) * color;
                coefficients[3] += (p_1_1 * normal.x) * color;
                coefficients[4] += (p_2_2 * (normal.x * normal.y + normal.y * normal.x)) * color;
                coefficients[5] += (p_2_1 * normal.y) * color;
                coefficients[6] += (p_2_0) * color;
                coefficients[7] += (p_2_1 * normal.x) * color;
                coefficients[8] += (p_2_2 * (normal.x * normal.x - normal.y * normal.y)) * color;
            }
        }
    }

    const float normalizer = (4.0 * 3.14159265358979323846) / totalWeight;
    for (int order = 0; order < 9; order++)
    {
        outputCoefficients[order] = coefficients[order] * normalizer;
    }
}