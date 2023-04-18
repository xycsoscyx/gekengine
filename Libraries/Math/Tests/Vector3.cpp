#include "GEK/Math/Vector3.hpp"
#include <gtest/gtest.h>

using namespace Gek::Math;

TEST(Vector3, CheckZero)
{
    EXPECT_EQ(Float3::Zero.x, 0.0f);
    EXPECT_EQ(Float3::Zero.y, 0.0f);
    EXPECT_EQ(Float3::Zero.z, 0.0f);
}

TEST(Vector3, CheckOne)
{
    EXPECT_EQ(Float3::One.x, 1.0f);
    EXPECT_EQ(Float3::One.y, 1.0f);
    EXPECT_EQ(Float3::One.z, 1.0f);
}

TEST(Vector3, CheckConstructor)
{
    Float3 value(-1.0f, -2.0f, -3.0f);
    EXPECT_EQ(value.x, -1.0f);
    EXPECT_EQ(value.y, -2.0f);
    EXPECT_EQ(value.z, -3.0f);
}

TEST(Vector3, CheckAssignment)
{
    Float3 value = Float3::Zero;
    EXPECT_EQ(value.x, 0.0f);
    EXPECT_EQ(value.y, 0.0f);
    EXPECT_EQ(value.z, 0.0f);
}

TEST(Vector3, ScalarAddition)
{
    Float3 value(2.0f, 3.0f, 4.0f);
    value = value + 10.0f;
    EXPECT_EQ(value, Float3(12.0f, 13.0f, 14.0f));
}

TEST(Vector3, ScalarAdditionAssignment)
{
    Float3 value(2.0f, 3.0f, 4.0f);
    value += 10.0f;
    EXPECT_EQ(value, Float3(12.0f, 13.0f, 14.0f));
}

TEST(Vector3, ScalarSubtraction)
{
    Float3 value(12.0f, 13.0f, 14.0f);
    value = value - 10.0f;
    EXPECT_EQ(value, Float3(2.0f, 3.0f, 4.0f));
}

TEST(Vector3, ScalarSubtractionAssignment)
{
    Float3 value(12.0f, 13.0f, 14.0f);
    value -= 10.0f;
    EXPECT_EQ(value, Float3(2.0f, 3.0f, 4.0f));
}

TEST(Vector3, ScalarMultiplication)
{
    Float3 value(2.0f, 3.0f, 4.0f);
    value = value * 10.0f;
    EXPECT_EQ(value, Float3(20.0f, 30.0f, 40.0f));
}

TEST(Vector3, ScalarMultiplicationAssignment)
{
    Float3 value(2.0f, 3.0f, 4.0f);
    value *= 10.0f;
    EXPECT_EQ(value, Float3(20.0f, 30.0f, 40.0f));
}

TEST(Vector3, ScalarDivision)
{
    Float3 value(20.0f, 30.0f, 40.0f);
    value = value / 10.0f;
    EXPECT_EQ(value, Float3(2.0f, 3.0f, 4.0f));
}

TEST(Vector3, ScalarDivisionAssignment)
{
    Float3 value(20.0f, 30.0f, 40.0f);
    value /= 10.0f;
    EXPECT_EQ(value, Float3(2.0f, 3.0f, 4.0f));
}

TEST(Vector3, VectorAddition)
{
    Float3 value1(2.0f, 3.0f, 4.0f);
    Float3 value2(2.0f, 3.0f, 4.0f);
    Float3 result = value1 + value2;
    EXPECT_EQ(result, Float3(4.0f, 6.0f, 8.0f));
}

TEST(Vector3, VectorAdditionAssignment)
{
    Float3 value1(2.0f, 3.0f, 4.0f);
    Float3 value2(2.0f, 3.0f, 4.0f);
    Float3 result = value1;
    result += value2;
    EXPECT_EQ(result, Float3(4.0f, 6.0f, 8.0f));
}

TEST(Vector3, VectorSubtraction)
{
    Float3 value1(4.0f, 6.0f, 8.0f);
    Float3 value2(2.0f, 3.0f, 4.0f);
    Float3 result = value1 - value2;
    EXPECT_EQ(result, Float3(2.0f, 3.0f, 4.0f));
}

TEST(Vector3, VectorSubtractionAssignment)
{
    Float3 value1(4.0f, 6.0f, 8.0f);
    Float3 value2(2.0f, 3.0f, 4.0f);
    Float3 result = value1;
    result -= value2;
    EXPECT_EQ(result, Float3(2.0f, 3.0f, 4.0f));
}

TEST(Vector3, VectorMultiplication)
{
    Float3 value1(2.0f, 3.0f, 4.0f);
    Float3 value2(2.0f, 3.0f, 4.0f);
    Float3 result = value1 * value2;
    EXPECT_EQ(result, Float3(4.0f, 9.0f, 16.0f));
}

TEST(Vector3, VectorMultiplicationAssignment)
{
    Float3 value1(2.0f, 3.0f, 4.0f);
    Float3 value2(2.0f, 3.0f, 4.0f);
    Float3 result = value1;
    result *= value2;
    EXPECT_EQ(result, Float3(4.0f, 9.0f, 16.0f));
}

TEST(Vector3, VectorDivision)
{
    Float3 value1(4.0f, 9.0f, 16.0f);
    Float3 value2(2.0f, 3.0f, 4.0f);
    Float3 result = value1 / value2;
    EXPECT_EQ(result, Float3(2.0f, 3.0f, 4.0f));
}

TEST(Vector3, VectorDivisionAssignment)
{
    Float3 value1(4.0f, 9.0f, 16.0f);
    Float3 value2(2.0f, 3.0f, 4.0f);
    Float3 result = value1;
    result /= value2;
    EXPECT_EQ(result, Float3(2.0f, 3.0f, 4.0f));
}

TEST(Vector3, OperationsDot)
{
    Float3 value(2.0f, 3.0f, 4.0f);
    EXPECT_EQ(value.dot(value), 29.0f);
}

TEST(Vector3, OperationsMagnitude)
{
    Float3 value(2.0f, 3.0f, 4.0f);
    EXPECT_EQ(value.getMagnitude(), 29.0f);
}

TEST(Vector3, OperationsLength)
{
    Float3 value(2.0f, 3.0f, 4.0f);
    EXPECT_EQ(value.getLength(), 5.38516474f);
}

TEST(Vector3, OperationsNormal)
{
    Float3 value(2.0f, 3.0f, 4.0f);
    EXPECT_EQ(value.getNormal(), Float3(0.371390671f, 0.557086f, 0.742781341f));
}

TEST(Vector3, OperationsDistance)
{
    EXPECT_EQ(Float3(1.0f, 1.0f, 1.0f).getDistance(Float3(2.0f, 1.0f, 1.0f)), 1.0f);
}

TEST(Vector3, OperationsNormalize)
{
    Float3 value(2.0f, 3.0f, 4.0f);
    value.normalize();
    EXPECT_EQ(value, Float3(0.371390671f, 0.557086f, 0.742781341f));
}

TEST(Vector3, OperationsAbsolute)
{
    Float3 value(5.0f, -10.0f, 15.0f);
    EXPECT_EQ(value.getAbsolute(), Float3(5.0f, 10.0f, 15.0f));
}

TEST(Vector3, OperationsClamped)
{
    Float3 value(5.0f, -10.0f, 15.0f);
    EXPECT_EQ(value.getClamped(Float3::Zero, Float3::One), Float3(1.0f, 0.0f, 1.0f));
}

TEST(Vector3, OperationsSaturated)
{
    Float3 value(5.0f, -10.0f, 15.0f);
    EXPECT_EQ(value.getSaturated(), Float3(1.0f, 0.0f, 1.0f));
}

TEST(Vector3, OperationsMinimum)
{
    Float3 value(5.0f, -10.0f, 15.0f);
    EXPECT_EQ(value.getMinimum(Float3::Zero), Float3(0.0f, -10.0f, 0.0f));
}

TEST(Vector3, OperationsMaximum)
{
    Float3 value(5.0f, -10.0f, 15.0f);
    EXPECT_EQ(value.getMaximum(Float3::Zero), Float3(5.0f, 0.0f, 15.0f));
}

TEST(Vector3, OperationsCrossX)
{
    EXPECT_EQ(Float3(1.0f, 0.0f, 0.0f).cross(Float3(0.0f, 1.0f, 0.0f)), Float3(0.0f, 0.0f, 1.0f));
}

TEST(Vector3, OperationsCrossY)
{
    EXPECT_EQ(Float3(0.0f, 1.0f, 0.0f).cross(Float3(0.0f, 0.0f, 1.0f)), Float3(1.0f, 0.0f, 0.0f));
}

TEST(Vector3, OperationsCrossZ)
{
    EXPECT_EQ(Float3(0.0f, 0.0f, 1.0f).cross(Float3(1.0f, 0.0f, 0.0f)), Float3(0.0f, 1.0f, 0.0f));
}

static const Float3 lowValue(1.0f, 2.0f, 3.0f);
static const Float3 highValue(2.0f, 3.0f, 4.0f);
TEST(Vector3, ComparisonsLess)
{
    EXPECT_TRUE(lowValue < highValue);
}

TEST(Vector3, ComparisonsLessEqual)
{
    EXPECT_TRUE(lowValue <= highValue);
}

TEST(Vector3, ComparisonsGreater)
{
    EXPECT_TRUE(highValue > lowValue);
}

TEST(Vector3, ComparisonsGreaterEqual)
{
    EXPECT_TRUE(highValue >= lowValue);
}

TEST(Vector3, ComparisonsEqual)
{
    EXPECT_TRUE(lowValue == lowValue);
}

TEST(Vector3, ComparisonsNotEqual)
{
    EXPECT_TRUE(lowValue != highValue);
}