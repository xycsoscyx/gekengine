#pragma once 

#include "GEK\Math\Common.hpp"
#include "GEK\Math\Vector2.hpp"
#include "GEK\Math\Matrix4x4.hpp"
#include "GEK\Shapes\Sphere.hpp"
#include <concurrent_vector.h>
#include <algorithm>
#include <vector>

namespace Gek
{
    template <uint32_t TILE_X_COUNT, uint32_t TILE_Y_COUNT>
    class LightGrid
    {
    public:
        struct ScreenRect
        {
            Math::UInt2 min;
            Math::UInt2 max;
        };

        using ScreenRects = std::vector<ScreenRect>;
        using Lights = concurrency::concurrent_vector<Shapes::Sphere>;

    protected:
        Math::UInt2 m_tileSize;
        Math::UInt2 m_gridDim;
        int m_gridOffsets[TILE_X_COUNT * TILE_Y_COUNT];
        int m_gridCounts[TILE_X_COUNT * TILE_Y_COUNT];
        std::vector<int> m_tileLightIndexLists;
        uint32_t m_maxTileLightCount;

        Lights m_viewSpaceLights;
        ScreenRects m_screenRects;

    protected:
        void buildRects(const Math::UInt2 resolution, const Lights &lights, const Math::Float4x4 &modelView, const Math::Float4x4 &projection, float near)
        {
            m_viewSpaceLights.clear();
            m_screenRects.clear();
            for (uint32_t i = 0; i < lights.size(); ++i)
            {
                const Shapes::Sphere &l = lights[i];
                Math::Float3 vp = modelView.transform(l.position);
                ScreenRect rect = findScreenSpaceBounds(projection, vp, l.radius, resolution.x, resolution.y, near);
                if (rect.min.x < rect.max.x && rect.min.y < rect.max.y)
                {
                    m_screenRects.push_back(rect);
                    m_viewSpaceLights.push_back(Shapes::Sphere(vp, l.radius));
                }
            }
        }

        // nc: Tangent plane x/y normal coordinate (view space)
        // lc: Light x/y coordinate (view space)
        // lz: Light z coordinate (view space)
        // cameraScale: Project scale for coordinate (_11 or _22 for x/y respectively)
        void updateClipRegionRoot(float nc, float lc, float lz, float lightRadius, float cameraScale, float &clipMin, float &clipMax)
        {
            float nz = (lightRadius - nc * lc) / lz;
            float pz = (lc * lc + lz * lz - lightRadius * lightRadius) / (lz - (nz / nc) * lc);
            if (pz < 0.0f)
            {
                float c = -nz * cameraScale / nc;
                if (nc < 0.0f)
                {
                    // Left side boundary
                    clipMin = std::max(clipMin, c);
                }
                else
                {                          // Right side boundary
                    clipMax = std::min(clipMax, c);
                }
            }
        }

        // lc: Light x/y coordinate (view space)
        // lz: Light z coordinate (view space)
        // cameraScale: Project scale for coordinate (_11 or _22 for x/y respectively)
        void updateClipRegion(float lc, float lz, float lightRadius, float cameraScale, float &clipMin, float &clipMax)
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
                clipRegion.set(clipMin.x, clipMin.y, clipMax.x, clipMax.y);
            }

            return clipRegion;
        }

        bool testDepthBounds(const Math::Float2 &zRange, const Shapes::Sphere& light)
        {
            float lightMin = light.position.z + light.radius;
            float lightMax = light.position.z - light.radius;
            return (zRange.y < lightMin && zRange.x > lightMax);
        }

    public:
        LightGrid(void)
        {
        }

        void build(const Math::UInt2 tileSize, const Math::UInt2 resolution, const Lights &lights, const Math::Float4x4 &modelView, const Math::Float4x4 &projection, float near)
        {
            m_tileSize = tileSize;
            m_gridDim = (resolution + tileSize - 1) / tileSize;
            m_maxTileLightCount = 0;

            buildRects(resolution, lights, modelView, projection, near);

            memset(m_gridOffsets, 0, sizeof(m_gridOffsets));
            memset(m_gridCounts, 0, sizeof(m_gridCounts));

#define GRID_OFFSETS(_x_,_y_) (m_gridOffsets[_x_ + _y_ * TILE_X_COUNT])
#define GRID_COUNTS(_x_,_y_) (m_gridCounts[_x_ + _y_ * TILE_X_COUNT])
            int totalus = 0;
            for (size_t i = 0; i < m_screenRects.size(); ++i)
            {
                ScreenRect r = m_screenRects[i];
                Shapes::Sphere light = m_viewSpaceLights[i];

                Math::UInt2 l = Math::clamp(r.min / tileSize, Math::UInt2(0, 0), m_gridDim + 1);
                Math::UInt2 u = Math::clamp((r.max + tileSize - 1) / tileSize, Math::UInt2(0, 0), m_gridDim + 1);

                for (uint32_t y = l.y; y < u.y; ++y)
                {
                    for (uint32_t x = l.x; x < u.x; ++x)
                    {
                        GRID_COUNTS(x, y) += 1;
                        ++totalus;
                    }
                }
            }

            m_tileLightIndexLists.resize(totalus);

            uint32_t offset = 0;
            for (uint32_t y = 0; y < m_gridDim.y; ++y)
            {
                for (uint32_t x = 0; x < m_gridDim.x; ++x)
                {
                    uint32_t count = GRID_COUNTS(x, y);
                    // set offset to be just past end, then decrement while filling in
                    GRID_OFFSETS(x, y) = offset + count;
                    offset += count;

                    // for debug/profiling etc.
                    m_maxTileLightCount = std::max(m_maxTileLightCount, count);
                }
            }

            if (m_screenRects.size() && !m_tileLightIndexLists.empty())
            {
                int *data = m_tileLightIndexLists.data();
                for (size_t i = 0; i < m_screenRects.size(); ++i)
                {
                    uint32_t lightId = uint32_t(i);

                    Shapes::Sphere light = m_viewSpaceLights[i];
                    ScreenRect r = m_screenRects[i];

                    Math::UInt2 l = Math::clamp(r.min / tileSize, Math::UInt2(0, 0), m_gridDim + 1);
                    Math::UInt2 u = Math::clamp((r.max + tileSize - 1) / tileSize, Math::UInt2(0, 0), m_gridDim + 1);

                    for (uint32_t y = l.y; y < u.y; ++y)
                    {
                        for (uint32_t x = l.x; x < u.x; ++x)
                        {
                            // store reversely into next free slot
                            uint32_t offset = GRID_OFFSETS(x, y) - 1;
                            data[offset] = lightId;
                            GRID_OFFSETS(x, y) = offset;
                        }
                    }
                }
            }
#undef GRID_COUNTS
#undef GRID_OFFSETS
        }

        const Math::UInt2 &getTileSize(void) const
        {
            return m_tileSize;
        }

        const Math::UInt2 &getGridDim(void) const
        {
            return m_gridDim;
        }

        uint32_t getMaxTileLightCount(void) const
        {
            return m_maxTileLightCount;
        }

        uint32_t getTotalTileLightIndexListLength(void) const
        {
            return uint32_t(m_tileLightIndexLists.size());
        }

        int tileLightCount(uint32_t x, uint32_t y) const
        {
            return m_gridCounts[x + y * TILE_X_COUNT];
        }

        const int *tileLightIndexList(uint32_t x, uint32_t y) const
        {
            return &m_tileLightIndexLists[m_gridOffsets[x + y * TILE_X_COUNT]];
        }

        const int *tileDataPtr(void) const
        {
            return m_gridOffsets;
        }

        const int *tileCountsDataPtr(void) const
        {
            return m_gridCounts;
        }

        const int *tileLightIndexListsPtr(void) const
        {
            return m_tileLightIndexLists.data();
        }

        const Lights &getViewSpaceLights(void) const
        {
            return m_viewSpaceLights;
        }

        bool empty(void) const
        {
            return m_screenRects.empty();
        }

        ScreenRect findScreenSpaceBounds(const Math::Float4x4 &projection, Math::Float3 pt, float rad, int width, int height, float near)
        {
            Math::Float4 reg = computeClipRegion(pt, rad, near, projection);
            reg = -reg;

            std::swap(reg.x, reg.z);
            std::swap(reg.y, reg.w);
            reg *= 0.5f;
            reg += 0.5f;

            reg = Math::saturate(reg);

            LightGrid::ScreenRect result;
            result.min.x = int(reg.x * float(width));
            result.min.y = int(reg.y * float(height));
            result.max.x = int(reg.z * float(width));
            result.max.y = int(reg.w * float(height));

            return result;
        }

    };
}; // namespace Gek
