TextureCube<float3> environmentMap : register(t0);
SamplerState linearSampler : register(s0);
RWBuffer<float3> outputCoefficients : register(u0);

groupshared float3 coefficients[9];

void updateCoefficients(float3 environmentSample, float x, float y, float z)
{
    for (int channel = 0; channel < 3; channel++)
    {
        /* L_{00}.  Note that Y_{00} = 0.282095 */
        float constant = 0.282095;
        coefficients[0][channel] += environmentSample[channel] * constant;

        /* L_{1m}. -1 <= m <= 1.  The linear terms */
        constant = 0.488603;
        coefficients[1][channel] += environmentSample[channel] * (constant * y);   /* Y_{1-1} = 0.488603 y  */
        coefficients[2][channel] += environmentSample[channel] * (constant * z);   /* Y_{10}  = 0.488603 z  */
        coefficients[3][channel] += environmentSample[channel] * (constant * x);   /* Y_{11}  = 0.488603 x  */

        /* The Quadratic terms, L_{2m} -2 <= m <= 2 */

        /* First, L_{2-2}, L_{2-1}, L_{21} corresponding to xy,yz,xz */
        constant = 1.092548;
        coefficients[4][channel] += environmentSample[channel] * (constant * x * y); /* Y_{2-2} = 1.092548 xy */
        coefficients[5][channel] += environmentSample[channel] * (constant * y * z); /* Y_{2-1} = 1.092548 yz */
        coefficients[7][channel] += environmentSample[channel] * (constant * x * z); /* Y_{21}  = 1.092548 xz */

        /* L_{20}.  Note that Y_{20} = 0.315392 (3z^2 - 1) */
        constant = 0.315392;
        coefficients[6][channel] += environmentSample[channel] * (constant * (3 * z * z - 1));

        /* L_{22}.  Note that Y_{22} = 0.546274 (x^2 - y^2) */
        constant = 0.546274;
        coefficients[8][channel] += environmentSample[channel] * (constant * (x * x - y * y));
    }
}

[numthreads(1, 1, 1)]
void mainComputeProgram(uint3 screenPosition : SV_DispatchThreadID, uint3 tilePosition : SV_GroupID, uint pixelIndex : SV_GroupIndex)
{
    for (int order = 0; order < 9; order++)
    {
        coefficients[order] = float3(0.0f, 0.0f, 0.0f);
    }

    float size;
    environmentMap.GetDimensions(size, size);
    float stepSize = ((1.0f / size) * 2.0f);

    for (float x = -1.0f; x <= 1.0f; x += stepSize)
    {
        for (float y = -1.0f; y <= 1.0f; y += stepSize)
        {
            for (float z = -1.0f; z <= 1.0f; z += stepSize)
            {
                float3 environmentSample = environmentMap.SampleLevel(linearSampler, float3(x, y, z), 0);
                updateCoefficients(environmentSample, x, y, z);
            }
        }
    }

    for (int order = 0; order < 9; order++)
    {
        outputCoefficients[order] = coefficients[order];
    }
}