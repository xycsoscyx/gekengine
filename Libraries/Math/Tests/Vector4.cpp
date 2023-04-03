#include "GEK/Math/Vector4.hpp"
#include <gtest/gtest.h>

using namespace Gek::Math;

TEST(Vector4, Initialization)
{
    EXPECT_EQ(Float4::Zero.x, 0.0f);
    EXPECT_EQ(Float4::Zero.y, 0.0f);
    EXPECT_EQ(Float4::Zero.z, 0.0f);
    EXPECT_EQ(Float4::Zero.w, 0.0f);

    EXPECT_EQ(Float4::One.x, 1.0f);
    EXPECT_EQ(Float4::One.y, 1.0f);
    EXPECT_EQ(Float4::One.z, 1.0f);
    EXPECT_EQ(Float4::One.w, 1.0f);

    Float4 value(-1.0f, -2.0f, -3.0f, -4.0f);
    EXPECT_EQ(value.x, -1.0f);
    EXPECT_EQ(value.y, -2.0f);
    EXPECT_EQ(value.z, -3.0f);
    EXPECT_EQ(value.w, -4.0f);

    value = Float4::Zero;
    EXPECT_EQ(value.x, 0.0f);
    EXPECT_EQ(value.y, 0.0f);
    EXPECT_EQ(value.z, 0.0f);
    EXPECT_EQ(value.w, 0.0f);
}

TEST(Vector4, ScalarOperations)
{
    static const Float4 testValue(2.0f, 3.0f, 4.0f, 5.0f);
    Float4 value(testValue);

    value = value + 10.0f;
    EXPECT_EQ(value, Float4(12.0f, 13.0f, 14.0f, 15.0f));

    value += 10.0f;
    EXPECT_EQ(value, Float4(22.0f, 23.0f, 24.0f, 25.0f));

    value = value - 10.0f;
    EXPECT_EQ(value, Float4(12.0f, 13.0f, 14.0f, 15.0f));

    value -= 10.0f;
    EXPECT_EQ(value, Float4(2.0f, 3.0f, 4.0f, 5.0f));

    value = value * 10.0f;
    EXPECT_EQ(value, Float4(20.0f, 30.0f, 40.0f, 50.0f));

    value *= 10.0f;
    EXPECT_EQ(value, Float4(200.0f, 300.0f, 400.0f, 500.0f));

    value = value / 10.0f;
    EXPECT_EQ(value, Float4(20.0f, 30.0f, 40.0f, 50.0f));

    value /= 10.0f;
    EXPECT_EQ(value, Float4(2.0f, 3.0f, 4.0f, 5.0f));
}

TEST(Vector4, VectorOperations)
{
    static const Float4 testValue(2.0f, 3.0f, 4.0f, 5.0f);
    Float4 value(testValue);

    value = value + testValue;
    EXPECT_EQ(value, Float4(4.0f, 6.0f, 8.0f, 10.0f));

    value += testValue;
    EXPECT_EQ(value, Float4(6.0f, 9.0f, 12.0f, 15.0f));

    value = value - testValue;
    EXPECT_EQ(value, Float4(4.0f, 6.0f, 8.0f, 10.0f));

    value -= testValue;
    EXPECT_EQ(value, Float4(2.0f, 3.0f, 4.0f, 5.0f));

    value = value * testValue;
    EXPECT_EQ(value, Float4(4.0f, 9.0f, 16.0f, 25.0f));

    value *= testValue;
    EXPECT_EQ(value, Float4(8.0f, 27.0f, 64.0f, 125.0f));

    value = value / testValue;
    EXPECT_EQ(value, Float4(4.0f, 9.0f, 16.0f, 25.0f));

    value /= testValue;
    EXPECT_EQ(value, Float4(2.0f, 3.0f, 4.0f, 5.0f));
}

TEST(Vector4, Operations)
{
    static const Float4 testValue(2.0f, 3.0f, 4.0f, 5.0f);

    EXPECT_EQ(testValue.getLength(), 7.34846926f);

    Float4 value(testValue);
    EXPECT_EQ(value.getNormal(), Float4(0.272165537f, 0.408248305f, 0.544331074f, 0.680413842f));

    value.normalize();
    EXPECT_EQ(value, Float4(0.272165537f, 0.408248305f, 0.544331074f, 0.680413842f));
}