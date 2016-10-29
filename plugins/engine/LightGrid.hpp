#pragma once

#include "GEK\Math\Vector2.hpp"
#include "GEK\Math\Vector3.hpp"
#include "GEK\Math\Vector4SIMD.hpp"
#include "GEK\Math\Matrix4x4SIMD.hpp"
#include "GEK\Math\Convert.hpp"

namespace Gek
{
	template <uint32_t GridWidth, uint32_t GridHeight, typename LightData>
	class LightGrid
	{
	public:
		struct ScreenBounds
		{
			Math::Float2 minimum;
			Math::Float2 maximum;

			ScreenBounds(const Math::Float2 &minimum, const Math::Float2 &maximum)
				: minimum(minimum)
				, maximum(maximum)
			{
			}
		};

		using ScreenBoundsList = std::vector<ScreenBounds>;
		using LightDataList = std::vector<LightData>;
		using ConcurrentLightDataList = concurrency::concurrent_vector<LightData>;

	protected:
		const Math::UInt2 gridDimensions = Math::UInt2(GridWidth, GridHeight);

        uint32_t gridOffsetList[GridWidth * GridHeight];
        uint32_t gridCountList[GridWidth * GridHeight];
		std::vector<uint32_t> tileIndexList;

		LightDataList lightDataList;
		ScreenBoundsList screenBoundsList;

	protected:
		void buildRects(const Math::SIMD::Float4x4 &projectionMatrix, float nearClip, const Math::UInt2 resolution, const ConcurrentLightDataList &concurrentLightDataList)
		{
			lightDataList.clear();
			screenBoundsList.clear();
			for (size_t lightIndex = 0; lightIndex < concurrentLightDataList.size(); ++lightIndex)
			{
				auto &lightData = concurrentLightDataList[lightIndex];
				ScreenBounds screenBounds = getBoundingBox(lightData.position, lightData.range, -nearClip, projectionMatrix);
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

		void build(const Math::SIMD::Float4x4 &projectionMatrix, float nearClip, const Math::UInt2 resolution, const ConcurrentLightDataList &concurrentLightDataList)
		{
			auto tileSize = (resolution / gridDimensions);
			buildRects(projectionMatrix, nearClip, resolution, concurrentLightDataList);

			memset(gridOffsetList, 0, sizeof(gridOffsetList));
			memset(gridCountList, 0, sizeof(gridCountList));

			uint32_t totalus = 0;
			for (size_t lightIndex = 0; lightIndex < screenBoundsList.size(); ++lightIndex)
			{
                auto &screenBounds = screenBoundsList[lightIndex];
                auto &lightData = lightDataList[lightIndex];

                uint32_t minTileX = std::max(0, int32_t(screenBounds.minimum.x / tileSize.x));
                uint32_t maxTileX = std::min(int32_t(gridDimensions.x - 1), int32_t(screenBounds.minimum.y / tileSize.x));

                uint32_t minTileY = std::max(0, int32_t(screenBounds.maximum.x / tileSize.y));
                uint32_t maxTileY = std::min(int32_t(gridDimensions.y - 1), int32_t(screenBounds.maximum.y / tileSize.y));

                for (uint32_t x = minTileX; x <= maxTileX; ++x)
                {
                    for (uint32_t y = minTileY; y <= maxTileY; ++y)
                    {
                        gridCountList[x + y * gridDimensions.x] += 1;
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
					auto count = gridCountList[x + y * gridDimensions.x];
					gridOffsetList[x + y * gridDimensions.x] = offset + count;
					offset += count;
				}
			}

			if (screenBoundsList.size() && !tileIndexList.empty())
			{
				auto data = tileIndexList.data();
				for (size_t lightIndex = 0; lightIndex < screenBoundsList.size(); ++lightIndex)
				{
					auto &screenBounds = screenBoundsList[lightIndex];
					auto &lightData = lightDataList[lightIndex];

                    uint32_t minTileX = std::max(0, int32_t(screenBounds.minimum.x / tileSize.x));
                    uint32_t maxTileX = std::min(int32_t(gridDimensions.x - 1), int32_t(screenBounds.minimum.y / tileSize.x));

                    uint32_t minTileY = std::max(0, int32_t(screenBounds.maximum.x / tileSize.y));
                    uint32_t maxTileY = std::min(int32_t(gridDimensions.y - 1), int32_t(screenBounds.maximum.y / tileSize.y));

                    for (uint32_t x = minTileX; x <= maxTileX; ++x)
                    {
                        for (uint32_t y = minTileY; y <= maxTileY; ++y)
                        {
                            auto &offset = --gridOffsetList[x + y * gridDimensions.x];
							data[offset] = lightIndex;
						}
					}
				}
			}
		}

		void getBoundsForAxis(bool useXAxis, const Math::Float3& center, float radius, float nearClip, const Math::SIMD::Float4x4& projMatrix, Math::Float3& U, Math::Float3& L)
		{
			static const Math::Float3 XAxis(1.0f, 0.0f, 0.0f);
			static const Math::Float3 yAxis(0.0f, 1.0f, 0.0f);
			bool trivialAccept = (center.z + radius) < nearClip; // Entirely in back of nearPlane (Trivial Accept)
			const Math::Float3& a = useXAxis ? XAxis : yAxis;

			// given in coordinates (a,z), where a is in the direction of the vector a, and z is in the standard z direction
			const Math::Float2& projectedCenter = Math::Float2(a.dot(center), center.z);
			Math::Float2 bounds_az[2];
			float tSquared = projectedCenter.dot(projectedCenter) - Math::square(radius);
			float t, cLength, costheta, sintheta;

			if (tSquared > 0)
			{
				// Camera is outside sphere
								 // Distance to the tangent points of the sphere (points where a vector from the camera are tangent to the sphere) (calculated a-z space)
				t = std::sqrt(tSquared);
				cLength = projectedCenter.getLength();

				// Theta is the angle between the vector from the camera to the center of the sphere and the vectors from the camera to the tangent points
				costheta = t / cLength;
				sintheta = radius / cLength;
			}

			float sqrtPart;
			if (!trivialAccept)
			{
				sqrtPart = std::sqrt(Math::square(radius) - Math::square(nearClip - projectedCenter.y));
			}

			for (int i = 0; i < 2; ++i)
			{
				if (tSquared > 0)
				{
					bounds_az[i] = costheta * (costheta * projectedCenter.x + sintheta * projectedCenter.y);
					bounds_az[i] = costheta * (-sintheta * projectedCenter.x + costheta * projectedCenter.y);
				}

				if (!trivialAccept && (tSquared <= 0 || bounds_az[i].y > nearClip))
				{
					bounds_az[i].x = projectedCenter.x + sqrtPart;
					bounds_az[i].y = nearClip;
				}

				sintheta *= -1; // negate theta for B
				sqrtPart *= -1; // negate sqrtPart for B
			}

			U = bounds_az[0].x * a;
			U.z = bounds_az[0].y;
			L = bounds_az[1].x * a;
			L.z = bounds_az[1].y;
		}

		/** Center is in camera space */
		ScreenBounds getBoundingBox(const Math::Float3& center, float radius, float nearClip, const Math::SIMD::Float4x4& projMatrix)
		{
			Math::Float3 maxXHomogenous, minXHomogenous, maxYHomogenous, minYHomogenous;
			getBoundsForAxis(true, center, radius, nearClip, projMatrix, maxXHomogenous, minXHomogenous);
			getBoundsForAxis(false, center, radius, nearClip, projMatrix, maxYHomogenous, minYHomogenous);
			// We only need one coordinate for each point, so we save computation by only calculating x(or y) and w
			float maxX = maxXHomogenous.dot(projMatrix.normals[0].n) / maxXHomogenous.dot(projMatrix.normals[3].n);
			float minX = minXHomogenous.dot(projMatrix.normals[0].n) / minXHomogenous.dot(projMatrix.normals[3].n);
			float maxY = maxYHomogenous.dot(projMatrix.normals[1].n) / maxYHomogenous.dot(projMatrix.normals[3].n);
			float minY = minYHomogenous.dot(projMatrix.normals[1].n) / minYHomogenous.dot(projMatrix.normals[3].n);
			return ScreenBounds(Math::Float2(minX, minY), Math::Float2(maxX, maxY));
		}
	};
}; // namespace Gek