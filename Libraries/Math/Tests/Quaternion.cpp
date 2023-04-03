#include "GEK/Math/Quaternion.hpp"
#include <gtest/gtest.h>

using namespace Gek::Math;

TEST(Quaternion, Initialization)
{
    EXPECT_EQ(Quaternion::Zero.x, 0.0f);
    EXPECT_EQ(Quaternion::Zero.y, 0.0f);
    EXPECT_EQ(Quaternion::Zero.z, 0.0f);
    EXPECT_EQ(Quaternion::Zero.w, 0.0f);

    EXPECT_EQ(Quaternion::Identity.x, 0.0f);
    EXPECT_EQ(Quaternion::Identity.y, 0.0f);
    EXPECT_EQ(Quaternion::Identity.z, 0.0f);
    EXPECT_EQ(Quaternion::Identity.w, 1.0f);
}

TEST(Quaternion, ScalarOperations)
{
    static const Quaternion testValue(2.0f, 3.0f, 4.0f, 5.0f);
    Quaternion value(testValue);

    value = value + 10.0f;
    EXPECT_EQ(value, Quaternion(12.0f, 13.0f, 14.0f, 15.0f));

    value = value * 10.0f;
    EXPECT_EQ(value, Quaternion(120.0f, 130.0f, 140.0f, 150.0f));

    value *= 10.0f;
    EXPECT_EQ(value, Quaternion(1200.0f, 1300.0f, 1400.0f, 1500.0f));

    value = value / 10.0f;
    EXPECT_EQ(value, Quaternion(120.0f, 130.0f, 140.0f, 150.0f));

    value /= 10.0f;
    EXPECT_EQ(value, Quaternion(12.0f, 13.0f, 14.0f, 15.0f));
}

TEST(Quaternion, VectorOperations)
{
    static const Quaternion testValue(2.0f, 3.0f, 4.0f, 5.0f);
    Quaternion value(testValue);

    value = value + testValue;
    EXPECT_EQ(value, Quaternion(4.0f, 6.0f, 8.0f, 10.0f));
}