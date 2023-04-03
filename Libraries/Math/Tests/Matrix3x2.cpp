#include "GEK/Math/Matrix3x2.hpp"
#include <gtest/gtest.h>

using namespace Gek::Math;

TEST(Matrix3x2, Initialization)
{
    EXPECT_EQ(Float3x2::Identity.rx.x, 1.0f);
    EXPECT_EQ(Float3x2::Identity.rx.y, 0.0f);
    EXPECT_EQ(Float3x2::Identity.ry.x, 0.0f);
    EXPECT_EQ(Float3x2::Identity.ry.y, 1.0f);
    EXPECT_EQ(Float3x2::Identity.rz.x, 0.0f);
    EXPECT_EQ(Float3x2::Identity.rz.y, 0.0f);
}

TEST(Matrix3x2, Operations)
{
}