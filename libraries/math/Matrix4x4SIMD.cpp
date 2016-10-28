#include "GEK\Math\Matrix4x4SIMD.hpp"

namespace Gek
{
	namespace Math
	{
		namespace SIMD
		{
			const Float4x4 Float4x4::Identity(
			{
				1.0f, 0.0f, 0.0f, 0.0f,
				0.0f, 1.0f, 0.0f, 0.0f,
				0.0f, 0.0f, 1.0f, 0.0f,
				0.0f, 0.0f, 0.0f, 1.0f,
			});
		}; // namespace SIMD
	}; // namespace Math
}; // namespace Gek
