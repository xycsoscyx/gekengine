#include "GEK/Math/Matrix4x4.hpp"
#include <gtest/gtest.h>

using namespace Gek::Math;

TEST(Matrix4x4, CheckIdentity)
{
    EXPECT_EQ(Float4x4::Identity.r.x, Float4(1.0f, 0.0f, 0.0f, 0.0f));
    EXPECT_EQ(Float4x4::Identity.r.y, Float4(0.0f, 1.0f, 0.0f, 0.0f));
    EXPECT_EQ(Float4x4::Identity.r.z, Float4(0.0f, 0.0f, 1.0f, 0.0f));
    EXPECT_EQ(Float4x4::Identity.r.w, Float4(0.0f, 0.0f, 0.0f, 1.0f));
}

TEST(Matrix4x4, Operations)
{
}