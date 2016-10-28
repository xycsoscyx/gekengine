#include "GEK\Math\Vector4.hpp"

namespace Gek
{
    namespace Math
    {
        const Float4 Float4::Zero(0.0f, 0.0f, 0.0f, 0.0f);
        const Float4 Float4::One(1.0f, 1.0f, 1.0f, 1.0f);

		const Int4 Int4::Zero(0, 0, 0, 0);
		const Int4 Int4::One(1, 1, 1, 1);

		const UInt4 UInt4::Zero(0U, 0U, 0U, 0U);
		const UInt4 UInt4::One(1U, 1U, 1U, 1U);

		const Color Color::Zero(0.0f, 0.0f, 0.0f, 0.0f);
		const Color Color::One(1.0f, 1.0f, 1.0f, 1.0f);

		const Pixel Pixel::Zero(0U, 0U, 0U, 0U);
		const Pixel Pixel::One(255U, 255U, 255U, 255U);

	}; // namespace Math
}; // namespace Gek
