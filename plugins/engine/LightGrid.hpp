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
#define LIGHT_GRID_MAX_DIM_X 16
#define LIGHT_GRID_MAX_DIM_Y 8

    template <typename LIGHTDATA>
    class LightGrid
    {
    public:
        struct ScreenBox
        {
            Math::UInt2 min;
            Math::UInt2 max;
            uint32_t lightIndex;
        };

        using ScreenBoxList = std::vector<ScreenBox>;
        using LightList = concurrency::concurrent_vector<LIGHTDATA>;

    private:
        Math::UInt2 m_tileSize;
        Math::UInt2 m_gridDim;

        std::vector<Math::Float2> m_gridMinMaxZ;
        int m_gridOffsets[LIGHT_GRID_MAX_DIM_X * LIGHT_GRID_MAX_DIM_Y];
        int m_gridCounts[LIGHT_GRID_MAX_DIM_X * LIGHT_GRID_MAX_DIM_Y];
        std::vector<int> m_tileLightIndexLists;
        bool m_minMaxGridValid;

        LightList &m_viewSpaceLights;
        ScreenBoxList m_screenRects;

    private:
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
                    // Left side boundary
                    clipMin = std::max(clipMin, c);
                }
                else
                {                          // Right side boundary
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
            // Early out with empty rectangle if the lightData is too far behind the view frustum
            Math::Float4 clipRegion(1.0f, 1.0f, -1.0f, -1.0f);
            if (lightPosView.z - lightRadius <= -cameraNear)
            {
                Math::Float2 clipMin(-1.0f, -1.0f);
                Math::Float2 clipMax(1.0f, 1.0f);
                updateClipRegion(lightPosView.x, lightPosView.z, lightRadius, projection._11, clipMin.x, clipMax.x);
                updateClipRegion(lightPosView.y, lightPosView.z, lightRadius, projection._22, clipMin.y, clipMax.y);
                clipRegion = Math::Float4(clipMin.x, clipMin.y, clipMax.x, clipMax.y);
            }

            return clipRegion;
        }

        void buildRects(const Math::UInt2 resolution, const Math::Float4x4 &projection, float nearClip)
        {
            m_screenRects.clear();
            for (uint32_t i = 0; i < m_viewSpaceLights.size(); ++i)
            {
                const auto &lightData = m_viewSpaceLights[i];
                ScreenBox screenBox = findScreenSpaceBounds(projection, lightData.position, lightData.range, resolution.x, resolution.y, nearClip);
                if (screenBox.min.x < screenBox.max.x && screenBox.min.y < screenBox.max.y)
                {
                    screenBox.lightIndex = i;
                    m_screenRects.push_back(screenBox);
                }
            }
        }

    public:
        LightGrid(LightList &lights)
            : m_viewSpaceLights(lights)
        {
        }

        ScreenBox findScreenSpaceBounds(const Math::Float4x4 &projection, Math::Float3 pt, float rad, int width, int height, float nearClip)
        {
            Math::Float4 reg = computeClipRegion(pt, rad, nearClip, projection);
            reg = -reg;

            std::swap(reg.x, reg.z);
            std::swap(reg.y, reg.w);
            reg *= 0.5f;
            reg += 0.5f;

            static const Math::Float4 zeros = { 0.0f, 0.0f, 0.0f, 0.0f };
            static const Math::Float4 ones = { 1.0f, 1.0f, 1.0f, 1.0f };
            reg = Math::clamp(reg, zeros, ones);

            ScreenBox result;
            result.min.x = int(reg.x * float(width));
            result.min.y = int(reg.y * float(height));
            result.max.x = int(reg.z * float(width));
            result.max.y = int(reg.w * float(height));

            return result;
        }

        bool testDepthBounds(const Math::Float2 &zRange, const LIGHTDATA& lightData)
        {
            float lightMin = lightData.position.z + lightData.range;
            float lightMax = lightData.position.z - lightData.range;
            return (zRange.y < lightMin && zRange.x > lightMax);
        }

        void build(const Math::UInt2 tileSize, const Math::UInt2 resolution, const Math::Float4x4 &projection, float nearClip, const std::vector<Math::Float2> &gridMinMaxZ)
        {
            m_gridMinMaxZ = gridMinMaxZ;
            m_minMaxGridValid = !gridMinMaxZ.empty();

            const Math::Float2 *gridMinMaxZPtr = m_minMaxGridValid ? m_gridMinMaxZ.data() : nullptr;

            m_tileSize = tileSize;
            m_gridDim = (resolution + tileSize - 1U) / tileSize;
            buildRects(resolution, projection, nearClip);

            memset(m_gridOffsets, 0, sizeof(m_gridOffsets));
            memset(m_gridCounts, 0, sizeof(m_gridCounts));

#define GRID_OFFSETS(_x_,_y_) (m_gridOffsets[_x_ + _y_ * LIGHT_GRID_MAX_DIM_X])
#define GRID_COUNTS(_x_,_y_) (m_gridCounts[_x_ + _y_ * LIGHT_GRID_MAX_DIM_X])

            int totalus = 0;
            for (size_t i = 0; i < m_screenRects.size(); ++i)
            {
                auto &screenBox = m_screenRects[i];
                auto &lightData = m_viewSpaceLights[i];
                Math::UInt2 l = Math::clamp(screenBox.min / tileSize, Math::UInt2(0, 0), m_gridDim + 1U);
                Math::UInt2 u = Math::clamp((screenBox.max + tileSize - 1U) / tileSize, Math::UInt2(0, 0), m_gridDim + 1U);
                for (uint32_t y = l.y; y < u.y; ++y)
                {
                    for (uint32_t x = l.x; x < u.x; ++x)
                    {
                        if (!m_minMaxGridValid || testDepthBounds(gridMinMaxZPtr[y * m_gridDim.x + x], lightData))
                        {
                            GRID_COUNTS(x, y) += 1;
                            ++totalus;
                        }
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
                    GRID_OFFSETS(x, y) = offset + count;
                    offset += count;
                }
            }

            if (m_screenRects.size() && !m_tileLightIndexLists.empty())
            {
                int *data = &m_tileLightIndexLists[0];
                for (size_t i = 0; i < m_screenRects.size(); ++i)
                {
                    uint32_t lightId = uint32_t(i);
                    auto &lightData = m_viewSpaceLights[i];
                    auto &screenBox = m_screenRects[i];
                    Math::UInt2 l = Math::clamp(screenBox.min / tileSize, Math::UInt2(0, 0), m_gridDim + 1U);
                    Math::UInt2 u = Math::clamp((screenBox.max + tileSize - 1U) / tileSize, Math::UInt2(0, 0), m_gridDim + 1U);
                    for (uint32_t y = l.y; y < u.y; ++y)
                    {
                        for (uint32_t x = l.x; x < u.x; ++x)
                        {
                            if (!m_minMaxGridValid || testDepthBounds(gridMinMaxZPtr[y * m_gridDim.x + x], lightData))
                            {
                                uint32_t offset = GRID_OFFSETS(x, y) - 1;
                                data[offset] = lightId;
                                GRID_OFFSETS(x, y) = offset;
                            }
                        }
                    }
                }
            }
        }

        void prune(const std::vector<Math::Float2> &gridMinMaxZ)
        {
            m_gridMinMaxZ = gridMinMaxZ;
            m_minMaxGridValid = !gridMinMaxZ.empty();
            if (!m_minMaxGridValid || m_tileLightIndexLists.empty())
            {
                return;
            }

            const Math::Float2 *gridMinMaxZPtr = m_minMaxGridValid ? m_gridMinMaxZ.data() : nullptr;
            int *lightInds = &m_tileLightIndexLists[0];
#define GRID_OFFSETS(_x_,_y_) (m_gridOffsets[_x_ + _y_ * LIGHT_GRID_MAX_DIM_X])
#define GRID_COUNTS(_x_,_y_) (m_gridCounts[_x_ + _y_ * LIGHT_GRID_MAX_DIM_X])

            int totalus = 0;
            for (uint32_t y = 0; y < m_gridDim.y; ++y)
            {
                for (uint32_t x = 0; x < m_gridDim.x; ++x)
                {
                    uint32_t count = GRID_COUNTS(x, y);
                    uint32_t offset = GRID_OFFSETS(x, y);

                    for (uint32_t i = 0; i < count; ++i)
                    {
                        const auto &l = m_viewSpaceLights[lightInds[offset + i]];
                        if (!testDepthBounds(gridMinMaxZPtr[y * m_gridDim.x + x], l))
                        {
                            std::swap(lightInds[offset + i], lightInds[offset + count - 1]);
                            --count;
                        }
                    }

                    totalus += count;
                    GRID_COUNTS(x, y) = count;
                }
            }
#undef GRID_COUNTS
#undef GRID_OFFSETS
        }

        void pruneFarOnly(float aNear, const std::vector<Math::Float2> &gridMinMaxZ)
        {
            m_gridMinMaxZ = gridMinMaxZ;
            m_minMaxGridValid = !gridMinMaxZ.empty();
            if (!m_minMaxGridValid || m_tileLightIndexLists.empty())
            {
                return;
            }

            for (std::vector<Math::Float2>::iterator it = m_gridMinMaxZ.begin(); it != m_gridMinMaxZ.end(); ++it)
            {
                it->x = -aNear;
            }

            const Math::Float2 *gridMinMaxZPtr = m_minMaxGridValid ? m_gridMinMaxZ.data() : nullptr;
            int *lightInds = &m_tileLightIndexLists[0];

#	define GRID_OFFSETS(_x_,_y_) (m_gridOffsets[_x_ + _y_ * LIGHT_GRID_MAX_DIM_X])
#	define GRID_COUNTS(_x_,_y_) (m_gridCounts[_x_ + _y_ * LIGHT_GRID_MAX_DIM_X])

            int totalus = 0;
            for (uint32_t y = 0; y < m_gridDim.y; ++y)
            {
                for (uint32_t x = 0; x < m_gridDim.x; ++x)
                {
                    uint32_t count = GRID_COUNTS(x, y);
                    uint32_t offset = GRID_OFFSETS(x, y);

                    for (uint32_t i = 0; i < count; ++i)
                    {
                        const auto &l = m_viewSpaceLights[lightInds[offset + i]];
                        if (!testDepthBounds(gridMinMaxZPtr[y * m_gridDim.x + x], l))
                        {
                            std::swap(lightInds[offset + i], lightInds[offset + count - 1]);
                            --count;
                        }
                    }

                    totalus += count;
                    GRID_COUNTS(x, y) = count;
                }
            }
#	undef GRID_COUNTS
#	undef GRID_OFFSETS
        }
    };
}; // namespace Gek
