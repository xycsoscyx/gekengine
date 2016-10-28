#pragma once

#include "GEK\Math\Convert.hpp"

namespace Gek
{
    void updateClipRegionRoot(float nc,          // Tangent plane x/y normal coordinate (view space)
        float lc,          // Light x/y coordinate (view space)
        float lz,          // Light z coordinate (view space)
        float lightRadius,
        float cameraScale, // Project scale for coordinate (_11 or _22 for x/y respectively)
        float &clipMin,
        float &clipMax)
    {
        float nz = (lightRadius - nc * lc) / lz;
        float pz = (lc * lc + lz * lz - lightRadius * lightRadius) / (lz - (nz / nc) * lc);

        if (pz < 0.0f)
        {
            float c = -nz * cameraScale / nc;
            if (nc < 0.0f)
            {
                clipMin = std::max(clipMin, c);
            }
            else
            {
                clipMax = std::min(clipMax, c);
            }
        }
    }

    void updateClipRegion(float lc,          // Light x/y coordinate (view space)
        float lz,          // Light z coordinate (view space)
        float lightRadius,
        float cameraScale, // Project scale for coordinate (_11 or _22 for x/y respectively)
        float &clipMin,
        float &clipMax)
    {
        float rSq = lightRadius * lightRadius;
        float lcSqPluslzSq = lc * lc + lz * lz;
        float d = rSq * lc * lc - lcSqPluslzSq * (rSq - lz * lz);

        if (d >= 0.0f)
        {
            float a = lightRadius * lc;
            float b = sqrt(d);
            float nx0 = (a + b) / lcSqPluslzSq;
            float nx1 = (a - b) / lcSqPluslzSq;

            updateClipRegionRoot(nx0, lc, lz, lightRadius, cameraScale, clipMin, clipMax);
            updateClipRegionRoot(nx1, lc, lz, lightRadius, cameraScale, clipMin, clipMax);
        }
    }

    Math::Float4 computeClipRegion(const Math::Float3 &lightPosView, float lightRadius, float cameraNear, const Math::Float4x4 &projection)
    {
        Math::Float4 clipRegion(1.0f, 1.0f, -1.0f, -1.0f);
        if (lightPosView.z - lightRadius <= -cameraNear)
        {
            Math::Float2 clipMin(-1.0f, -1.0f);
            Math::Float2 clipMax(1.0f, 1.0f);
            updateClipRegion(lightPosView.x, lightPosView.z, lightRadius, projection._11, clipMin.x, clipMax.x);
            updateClipRegion(lightPosView.y, lightPosView.z, lightRadius, projection._22, clipMin.y, clipMax.y);
            clipRegion = Math::make(clipMin, clipMax);
        }

        return clipRegion;
    }
}; // namespace Gek