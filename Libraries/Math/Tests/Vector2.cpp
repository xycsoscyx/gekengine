#include "GEK/Math/Vector2.hpp"
#include <gtest/gtest.h>

using namespace Gek::Math;

TEST(Vector2, Initialization)
{
    EXPECT_EQ(Float2::Zero.x, 0.0f);
    EXPECT_EQ(Float2::Zero.y, 0.0f);

    EXPECT_EQ(Float2::One.x, 1.0f);
    EXPECT_EQ(Float2::One.y, 1.0f);

    Float2 value(-1.0f, -2.0f);
    EXPECT_EQ(value.x, -1.0f);
    EXPECT_EQ(value.y, -2.0f);

    value = Float2::Zero;
    EXPECT_EQ(value.x, 0.0f);
    EXPECT_EQ(value.y, 0.0f);
}

TEST(Vector2, ScalarOperations)
{
    static const Float2 testValue(2.0f, 3.0f);
    Float2 value(testValue);

    value = value + 10.0f;
    EXPECT_EQ(value, Float2(12.0f, 13.0f));

    value += 10.0f;
    EXPECT_EQ(value, Float2(22.0f, 23.0f));

    value = value - 10.0f;
    EXPECT_EQ(value, Float2(12.0f, 13.0f));

    value -= 10.0f;
    EXPECT_EQ(value, Float2(2.0f, 3.0f));

    value = value * 10.0f;
    EXPECT_EQ(value, Float2(20.0f, 30.0f));

    value *= 10.0f;
    EXPECT_EQ(value, Float2(200.0f, 300.0f));

    value = value / 10.0f;
    EXPECT_EQ(value, Float2(20.0f, 30.0f));

    value /= 10.0f;
    EXPECT_EQ(value, Float2(2.0f, 3.0f));
}

TEST(Vector2, VectorOperations)
{
    static const Float2 testValue(2.0f, 3.0f);
    Float2 value(testValue);

    value = value + testValue;
    EXPECT_EQ(value, Float2(4.0f, 6.0f));

    value += testValue;
    EXPECT_EQ(value, Float2(6.0f, 9.0f));

    value = value - testValue;
    EXPECT_EQ(value, Float2(4.0f, 6.0f));

    value -= testValue;
    EXPECT_EQ(value, Float2(2.0f, 3.0f));

    value = value * testValue;
    EXPECT_EQ(value, Float2(4.0f, 9.0f));

    value *= testValue;
    EXPECT_EQ(value, Float2(8.0f, 27.0f));

    value = value / testValue;
    EXPECT_EQ(value, Float2(4.0f, 9.0f));

    value /= testValue;
    EXPECT_EQ(value, Float2(2.0f, 3.0f));
}

TEST(Vector2, Operations)
{
    static const Float2 testValue(2.0f, 3.0f);

    EXPECT_EQ(testValue.dot(testValue), 13.0f);
    EXPECT_EQ(testValue.getMagnitude(), 13.0f);
    EXPECT_EQ(testValue.getLength(), 3.60555124f);
    EXPECT_EQ(testValue.getNormal(), Float2(0.554700196f, 0.832050323f));

    EXPECT_EQ(Float2(1.0f, 1.0f).getDistance(Float2(2.0f, 1.0f)), 1.0f);

    Float2 value(testValue);
    value.normalize();
    EXPECT_EQ(value, Float2(0.554700196f, 0.832050323f));

    value.set(5.0f, -10.0f);
    EXPECT_EQ(value.getAbsolute(), Float2(5.0f, 10.0f));
    EXPECT_EQ(value.getClamped(Float2::Zero, Float2::One), Float2(1.0f, 0.0f));
    EXPECT_EQ(value.getSaturated(), Float2(1.0f, 0.0f));
    EXPECT_EQ(value.getMinimum(Float2::Zero), Float2(0.0f, -10.0f));
    EXPECT_EQ(value.getMaximum(Float2::Zero), Float2(5.0f, 0.0f));
}

TEST(Vector2, Comparisons)
{
    static const Float2 lowValue(1.0f, 2.0f);
    static const Float2 highValue(2.0f, 3.0f);

    EXPECT_TRUE(lowValue < highValue);
    EXPECT_TRUE(lowValue <= highValue);
    EXPECT_TRUE(highValue > lowValue);
    EXPECT_TRUE(highValue >= lowValue);
    EXPECT_TRUE(lowValue == lowValue);
    EXPECT_TRUE(lowValue != highValue);
}