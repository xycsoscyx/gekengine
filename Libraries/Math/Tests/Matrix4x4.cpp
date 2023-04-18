#include "GEK/Math/Matrix4x4.hpp"
#include <gtest/gtest.h>

using namespace Gek::Math;

TEST(Matrix4x4, CheckIdentity)
{
    EXPECT_EQ(Float4x4::Identity.rx.x, 1.0f);
    EXPECT_EQ(Float4x4::Identity.rx.y, 0.0f);
    EXPECT_EQ(Float4x4::Identity.rx.z, 0.0f);
    EXPECT_EQ(Float4x4::Identity.rx.w, 0.0f);
    EXPECT_EQ(Float4x4::Identity.ry.x, 0.0f);
    EXPECT_EQ(Float4x4::Identity.ry.y, 1.0f);
    EXPECT_EQ(Float4x4::Identity.ry.z, 0.0f);
    EXPECT_EQ(Float4x4::Identity.ry.w, 0.0f);
    EXPECT_EQ(Float4x4::Identity.rz.x, 0.0f);
    EXPECT_EQ(Float4x4::Identity.rz.y, 0.0f);
    EXPECT_EQ(Float4x4::Identity.rz.z, 1.0f);
    EXPECT_EQ(Float4x4::Identity.rz.w, 0.0f);
    EXPECT_EQ(Float4x4::Identity.rw.x, 0.0f);
    EXPECT_EQ(Float4x4::Identity.rw.y, 0.0f);
    EXPECT_EQ(Float4x4::Identity.rw.z, 0.0f);
    EXPECT_EQ(Float4x4::Identity.rw.w, 1.0f);
}

TEST(Matrix4x4, Operations)
{
}