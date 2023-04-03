#include "GEK/Math/Vector3.hpp"
#include <gtest/gtest.h>

using namespace Gek::Math;

TEST(Vector3, Initialization)
{
    EXPECT_EQ(Float3::Zero.x, 0.0f);
    EXPECT_EQ(Float3::Zero.y, 0.0f);
    EXPECT_EQ(Float3::Zero.z, 0.0f);

    EXPECT_EQ(Float3::One.x, 1.0f);
    EXPECT_EQ(Float3::One.y, 1.0f);
    EXPECT_EQ(Float3::One.z, 1.0f);

    Float3 value(-1.0f, -2.0f, -3.0f);
    EXPECT_EQ(value.x, -1.0f);
    EXPECT_EQ(value.y, -2.0f);
    EXPECT_EQ(value.z, -3.0f);

    value = Float3::Zero;
    EXPECT_EQ(value.x, 0.0f);
    EXPECT_EQ(value.y, 0.0f);
    EXPECT_EQ(value.z, 0.0f);
}

TEST(Vector3, ScalarOperations)
{
    static const Float3 testValue(2.0f, 3.0f, 4.0f);
    Float3 value(testValue);

    value = value + 10.0f;
    EXPECT_EQ(value, Float3(12.0f, 13.0f, 14.0f));

    value += 10.0f;
    EXPECT_EQ(value, Float3(22.0f, 23.0f, 24.0f));

    value = value - 10.0f;
    EXPECT_EQ(value, Float3(12.0f, 13.0f, 14.0f));

    value -= 10.0f;
    EXPECT_EQ(value, Float3(2.0f, 3.0f, 4.0f));

    value = value * 10.0f;
    EXPECT_EQ(value, Float3(20.0f, 30.0f, 40.0f));

    value *= 10.0f;
    EXPECT_EQ(value, Float3(200.0f, 300.0f, 400.0f));

    value = value / 10.0f;
    EXPECT_EQ(value, Float3(20.0f, 30.0f, 40.0f));

    value /= 10.0f;
    EXPECT_EQ(value, Float3(2.0f, 3.0f, 4.0f));
}

TEST(Vector3, VectorOperations)
{
    static const Float3 testValue(2.0f, 3.0f, 4.0f);
    Float3 value(testValue);

    value = value + testValue;
    EXPECT_EQ(value, Float3(4.0f, 6.0f, 8.0f));

    value += testValue;
    EXPECT_EQ(value, Float3(6.0f, 9.0f, 12.0f));

    value = value - testValue;
    EXPECT_EQ(value, Float3(4.0f, 6.0f, 8.0f));

    value -= testValue;
    EXPECT_EQ(value, Float3(2.0f, 3.0f, 4.0f));

    value = value * testValue;
    EXPECT_EQ(value, Float3(4.0f, 9.0f, 16.0f));

    value *= testValue;
    EXPECT_EQ(value, Float3(8.0f, 27.0f, 64.0f));

    value = value / testValue;
    EXPECT_EQ(value, Float3(4.0f, 9.0f, 16.0f));

    value /= testValue;
    EXPECT_EQ(value, Float3(2.0f, 3.0f, 4.0f));
}

TEST(Vector3, Operations)
{
    static const Float3 testValue(2.0f, 3.0f, 4.0f);

    EXPECT_EQ(testValue.getLength(), 5.38516474f);

    Float3 value(testValue);
    EXPECT_EQ(value.getNormal(), Float3(0.371390671f, 0.557086f, 0.742781341f));

    value.normalize();
    EXPECT_EQ(value, Float3(0.371390671f, 0.557086f, 0.742781341f));
}