#include "GEK/Math/Matrix3x2.hpp"
#include <gtest/gtest.h>

using namespace Gek::Math;

TEST(Matrix3x2, Initialization)
{
    EXPECT_EQ(Float3x2::Identity.r.x, Float2(1.0f, 0.0f));
    EXPECT_EQ(Float3x2::Identity.r.y, Float2(0.0f, 1.0f));
    EXPECT_EQ(Float3x2::Identity.r.z, Float2(0.0f, 0.0f));
}

TEST(Matrix3x2, Operations)
{
}