#pragma once

#include "GEK\Math\Vector2.hpp"
#include "GEK\Math\Vector3.hpp"
#include "GEK\Math\Vector4SIMD.hpp"
#include "GEK\Math\Matrix4x4SIMD.hpp"
#include "GEK\Math\Convert.hpp"

namespace Gek
{
    template <size_t GridWidth, size_t GridHeight, typename LightData>
    class LightGrid
    {
    public:
        struct ScreenBounds
        {
            Math::UInt2 minimum;
            Math::UInt2 maximum;
        };

        using ScreenBoundsList = std::vector<ScreenBounds>;
        using LightDataList = std::vector<LightData>;
        using ConcurrentLightDataList = concurrency::concurrent_vector<LightData>;

    protected:
        Math::UInt2 gridDimensions = Math::UInt2(GridWidth, GridHeight);

        uint32_t gridOffsetList[GridWidth * GridHeight];
        uint32_t gridCountList[GridWidth * GridHeight];
        std::vector<int> tileIndexList;

        LightDataList lightDataList;
        ScreenBoundsList screenBoundsList;

    protected:
		void buildRects(const Math::Float4x4 &projectionMatrix, const Math::UInt2 resolution, const ConcurrentLightDataList &concurrentLightDataList)
        {
            lightDataList.clear();
            screenBoundsList.clear();
            for (uint32_t lightIndex = 0; lightIndex < concurrentLightDataList.size(); ++lightIndex)
            {
                auto &lightData = concurrentLightDataList[lightIndex];
                ScreenBounds screenBounds = getProjectedBounds(projectionMatrix, resolution, lightData.position, lightData.range);
                if (screenBounds.minimum.x < screenBounds.maximum.x && screenBounds.minimum.y < screenBounds.maximum.y)
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

        void build(const Math::Float4x4 &projectionMatrix, const Math::UInt2 resolution, const ConcurrentLightDataList &concurrentLightDataList)
        {
            auto tileSize = (resolution / Math::UInt2(GridWidth, GridHeight));
            buildRects(projectionMatrix, resolution, concurrentLightDataList);

            memset(gridOffsetList, 0, sizeof(gridOffsetList));
            memset(gridCountList, 0, sizeof(gridCountList));

            int totalus = 0;
            for (size_t lightIndex = 0; lightIndex < screenBoundsList.size(); ++lightIndex)
            {
                auto &screenBounds = screenBoundsList[lightIndex];
                auto &lightData = lightDataList[lightIndex];
                Math::UInt2 l = clamp(screenBounds.minimum / tileSize, Math::UInt2::Zero, gridDimensions + 1U);
                Math::UInt2 u = clamp((screenBounds.maximum + tileSize - 1U) / tileSize, Math::UInt2::Zero, gridDimensions + 1U);
                for (uint32_t y = l.y; y < u.y; ++y)
                {
                    for (uint32_t x = l.x; x < u.x; ++x)
                    {
                        gridCountList[x + y * GridWidth] += 1;
                        ++totalus;
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
                    Math::UInt2 l = clamp(screenBounds.minimum / tileSize, Math::UInt2::Zero, gridDimensions + 1U);
                    Math::UInt2 u = clamp((screenBounds.maximum + tileSize - 1U) / tileSize, Math::UInt2::Zero, gridDimensions + 1U);
                    for (uint32_t y = l.y; y < u.y; ++y)
                    {
                        for (uint32_t x = l.x; x < u.x; ++x)
                        {
                            auto &offset = --gridOffsetList[x + y * GridWidth];
                            data[offset] = lightId;
                        }
                    }
                }
            }
        }

		ScreenBounds getProjectedBounds(const Math::Float4x4 &projectionMatrix, const Math::UInt2 &resolution, const Math::Float3 &center, float radius)
		{
			float d2 = center.dot(center);
			float a = std::sqrt(d2 - radius * radius);
			Math::Float3 right = (radius / a) * Math::Float3(-center.z, 0.0f, center.x);
			Math::Float3 up = Math::Float3(0.0f, radius, 0.0f);

			Math::Float4 projectedRight = projectionMatrix.transform(Math::make(right, 0.0f));
			Math::Float4 projectedUp = projectionMatrix.transform(Math::make(up, 0.0f));
			Math::Float4 projectedCenter = projectionMatrix.transform(Math::make(center, 1.0f));

			Math::Float4 north = projectedCenter + projectedUp;
			Math::Float4 east = projectedCenter + projectedRight;
			Math::Float4 south = projectedCenter - projectedUp;
			Math::Float4 west = projectedCenter - projectedRight;

			north /= north.w;
			east /= east.w;
			west /= west.w;
			south /= south.w;
			auto minimum = std::min(std::min(std::min(east, west), north), south).xyz;
			auto maximum = std::max(std::max(std::max(east, west), north), south).xyz;

			ScreenBounds screenBounds;
			screenBounds.minimum = (minimum.xy * resolution);
			screenBounds.maximum = (maximum.xy * resolution);
			return screenBounds;
		}
	};
}; // namespace Gek