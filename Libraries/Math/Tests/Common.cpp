#include "GEK/Math/Common.hpp"
#include <gtest/gtest.h>

using namespace Gek::Math;

TEST(Common, DegreesToRadians)
{
	EXPECT_EQ(DegreesToRadians(90.0f), Pi / 2.0f);
}

TEST(Common, RadiansToDegrees)
{
	EXPECT_EQ(RadiansToDegrees(Pi + Pi / 2.0f), 270.0f);
}

TEST(Common, Interpolate)
{
	EXPECT_EQ(Interpolate(10.0f, 30.0f, 0.5f), 20.0f);
}

TEST(Common, BlendUnified)
{
	EXPECT_EQ(Blend(12.0f, 60.0f, 0.25f), 24.0f);
}

TEST(Common, BlendSeparate)
{
	EXPECT_EQ(Blend(3.0f, 1.0f / 3.0f, 9.0f, 0.5f), 5.5f);
}

TEST(Common, Clamp)
{
	EXPECT_EQ(Clamp(0.0f, 1.0f, 2.0f), 1.0f);
	EXPECT_EQ(Clamp(3.0f, 1.0f, 2.0f), 2.0f);
}