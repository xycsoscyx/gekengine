#include "GEK\Math\SIMD4x4.hpp"

namespace Gek
{
	namespace Math
	{
		const SIMD4x4 SIMD4x4::Identity(
			1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.0f, 0.0f, 0.0f, 1.0f);
	}; // namespace Math
}; // namespace Gek
