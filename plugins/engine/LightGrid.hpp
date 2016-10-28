#pragma once

#include "GEK\Math\Vector2.hpp"
#include "GEK\Math\Vector3.hpp"
#include "GEK\Math\Vector4.hpp"
#include "GEK\Math\Matrix4x4.hpp"
#include "GEK\Math\Convert.hpp"
#include "ClipRegion.hpp"

namespace Gek
{
    template <size_t GridWidth, size_t GridHeight, typename LightData>
    class LightGrid
    {
    public:
        struct ScreenBounds
        {
            Math::UInt2 min;
            Math::UInt2 max;
        };

        using ScreenBoundsList = std::vector<ScreenBounds>;
        using LightDataList = std::vector<LightData>;
        using ConcurrentLightDataList = concurrency::concurrent_vector<LightData>;

    protected:
        Math::UInt2 gridDimensions = Math::UInt2(GridWidth, GridHeight);

        bool usingDepthRangeList;
        std::vector<Math::Float2> depthRangeList;
        uint32_t gridOffsetList[GridWidth * GridHeight];
        uint32_t gridCountList[GridWidth * GridHeight];
        std::vector<int> tileIndexList;

        LightDataList lightDataList;
        ScreenBoundsList screenBoundsList;

    protected:
        void buildRects(const Math::UInt2 resolution, const ConcurrentLightDataList &concurrentLightDataList, const Math::Float4x4 &projectionMatrix, float nearClip)
        {
            lightDataList.clear();
            screenBoundsList.clear();

            for (uint32_t lightIndex = 0; lightIndex < concurrentLightDataList.size(); ++lightIndex)
            {
                auto &lightData = concurrentLightDataList[lightIndex];
                ScreenBounds screenBounds = findScreenSpaceBounds(projectionMatrix, lightData.position, lightData.range, resolution.x, resolution.y, nearClip);
                if (screenBounds.min.x < screenBounds.max.x && screenBounds.min.y < screenBounds.max.y)
                {
                    screenBoundsList.push_back(screenBounds);
                    lightDataList.push_back(lightData);
                }
            }
        }

        bool testDepthBounds(const Math::Float2 &depthRange, const LightData &lightData)
        {
            float lightMinimum = lightData.position.z + lightData.range;
            float lightMaximum = lightData.position.z - lightData.range;
            return (depthRange.y < lightMinimum && depthRange.x > lightMaximum);
        }

    public:
        LightGrid(void)
        {
        }

        void build(const Math::UInt2 resolution, const ConcurrentLightDataList &concurrentLightDataList, const Math::Float4x4 &projectionMatrix, float nearClip, const std::vector<Math::Float2> &depthMaximumList)
        {
            depthRangeList = depthMaximumList;
            usingDepthRangeList = !depthMaximumList.empty();

            auto depthRangeData = usingDepthRangeList ? depthRangeList.data() : nullptr;
            auto tileSize = (resolution / Math::UInt2(GridWidth, GridHeight));
            buildRects(resolution, concurrentLightDataList, projectionMatrix, nearClip);

            memset(gridOffsetList, 0, sizeof(gridOffsetList));
            memset(gridCountList, 0, sizeof(gridCountList));

            int totalus = 0;
            for (size_t lightIndex = 0; lightIndex < screenBoundsList.size(); ++lightIndex)
            {
                auto &screenBounds = screenBoundsList[lightIndex];
                auto &lightData = lightDataList[lightIndex];
                Math::UInt2 l = clamp(screenBounds.min / tileSize, Math::UInt2::Zero, gridDimensions + 1U);
                Math::UInt2 u = clamp((screenBounds.max + tileSize - 1U) / tileSize, Math::UInt2::Zero, gridDimensions + 1U);
                for (uint32_t y = l.y; y < u.y; ++y)
                {
                    for (uint32_t x = l.x; x < u.x; ++x)
                    {
                        if (!usingDepthRangeList || testDepthBounds(depthRangeData[y * gridDimensions.x + x], lightData))
                        {
                            gridCountList[x + y * GridWidth] += 1;
                            ++totalus;
                        }
                    }
                }
            }

            tileIndexList.resize(totalus);
            uint32_t offset = 0;
            for (uint32_t y = 0; y < gridDimensions.y; ++y)
            {
                for (uint32_t x = 0; x < gridDimensions.x; ++x)
                {
                    auto count = gridCountList[x + y * GridWidth];
                    gridOffsetList[x + y * GridWidth] = offset + count;
                    offset += count;
                }
            }

            if (screenBoundsList.size() && !tileIndexList.empty())
            {
                int *data = tileIndexList.data();
                for (size_t lightIndex = 0; lightIndex < screenBoundsList.size(); ++lightIndex)
                {
                    uint32_t lightId = uint32_t(lightIndex);
                    auto &screenBounds = screenBoundsList[lightIndex];
                    auto &lightData = lightDataList[lightIndex];
                    Math::UInt2 l = clamp(screenBounds.min / tileSize, Math::UInt2::Zero, gridDimensions + 1U);
                    Math::UInt2 u = clamp((screenBounds.max + tileSize - 1U) / tileSize, Math::UInt2::Zero, gridDimensions + 1U);
                    for (uint32_t y = l.y; y < u.y; ++y)
                    {
                        for (uint32_t x = l.x; x < u.x; ++x)
                        {
                            if (!usingDepthRangeList || testDepthBounds(depthRangeData[y * gridDimensions.x + x], lightData))
                            {
                                auto &offset = --gridOffsetList[x + y * GridWidth];
                                data[offset] = lightId;
                            }
                        }
                    }
                }
            }
        }

        void prune(const std::vector<Math::Float2> &depthMaximumList)
        {
            depthRangeList = depthMaximumList;
            usingDepthRangeList = !depthMaximumList.empty();
            if (!usingDepthRangeList || tileIndexList.empty())
            {
                return;
            }

            auto depthRangeData = usingDepthRangeList ? depthRangeList.data() : nullptr;
            auto tileIndexData = tileIndexList.data();

            int totalus = 0;
            for (uint32_t y = 0; y < gridDimensions.y; ++y)
            {
                for (uint32_t x = 0; x < gridDimensions.x; ++x)
                {
                    auto &count = gridCountList[x + y * GridWidth];
                    auto offset = gridOffsetList[x + y * GridWidth];
                    for (uint32_t lightIndex = 0; lightIndex < count; ++lightIndex)
                    {
                        auto &lightData = lightDataList[tileIndexData[offset + lightIndex]];
                        if (!testDepthBounds(depthRangeData[y * gridDimensions.x + x], lightData))
                        {
                            std::swap(tileIndexData[offset + lightIndex], tileIndexData[offset + count - 1]);
                            --count;
                        }
                    }

                    totalus += count;
                }
            }
        }

        void pruneFarOnly(float nearClip, const std::vector<Math::Float2> &depthMaximumList)
        {
            depthRangeList = depthMaximumList;
            usingDepthRangeList = !depthMaximumList.empty();
            if (!usingDepthRangeList || tileIndexList.empty())
            {
                return;
            }

            for (auto &depthRange : depthRangeList)
            {
                depthRange -= nearClip;
            }

            auto depthRangeData = usingDepthRangeList ? depthRangeList.data() : nullptr;
            auto tileIndexData = tileIndexList.data();

            int totalus = 0;
            for (uint32_t y = 0; y < gridDimensions.y; ++y)
            {
                for (uint32_t x = 0; x < gridDimensions.x; ++x)
                {
                    auto &count = gridCountList[x + y * GridWidth];
                    auto offset = gridOffsetList[x + y * GridWidth];
                    for (uint32_t lightIndex = 0; lightIndex < count; ++lightIndex)
                    {
                        auto &lightData = lightDataList[tileIndexData[offset + lightIndex]];
                        if (!testDepthBounds(depthRangeData[y * gridDimensions.x + x], lightData))
                        {
                            std::swap(tileIndexData[offset + lightIndex], tileIndexData[offset + count - 1]);
                            --count;
                        }
                    }

                    totalus += count;
                }
            }
        }

        ScreenBounds findScreenSpaceBounds(const Math::Float4x4 &projectionMatrix, const Math::Float3 &position, float radius, int width, int height, float nearClip)
        {
            Math::Float4 region = -computeClipRegion(position, radius, nearClip, projectionMatrix);
            std::swap(region.x, region.z);
            std::swap(region.y, region.w);
            region = ((region * 0.5f) + 0.5f);
            region = Math::clamp(region, Math::Float4::Zero, Math::Float4::One);

            ScreenBounds screenBounds;
            screenBounds.min.x = uint32_t(region.x * float(width));
            screenBounds.min.y = uint32_t(region.y * float(height));
            screenBounds.max.x = uint32_t(region.z * float(width));
            screenBounds.max.y = uint32_t(region.w * float(height));
            return screenBounds;
        }
    };
}; // namespace Gek