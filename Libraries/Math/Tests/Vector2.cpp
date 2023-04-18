#include "GEK/Math/Vector2.hpp"
#include <gtest/gtest.h>

using namespace Gek::Math;

TEST(Vector2, CheckZero)
{
    EXPECT_EQ(Float2::Zero.x, 0.0f);
    EXPECT_EQ(Float2::Zero.y, 0.0f);
}

TEST(Vector2, CheckOne)
{
    EXPECT_EQ(Float2::One.x, 1.0f);
    EXPECT_EQ(Float2::One.y, 1.0f);
}

TEST(Vector2, CheckConstructor)
{
    Float2 value(-1.0f, -2.0f);
    EXPECT_EQ(value.x, -1.0f);
    EXPECT_EQ(value.y, -2.0f);
}

TEST(Vector2, CheckAssignment)
{
    Float2 value = Float2::Zero;
    EXPECT_EQ(value.x, 0.0f);
    EXPECT_EQ(value.y, 0.0f);
}

TEST(Vector2, ScalarAddition)
{
    Float2 value(2.0f, 3.0f);
    value = value + 10.0f;
    EXPECT_EQ(value, Float2(12.0f, 13.0f));
}

TEST(Vector2, ScalarAdditionAssignment)
{
    Float2 value(2.0f, 3.0f);
    value += 10.0f;
    EXPECT_EQ(value, Float2(12.0f, 13.0f));
}

TEST(Vector2, ScalarSubtraction)
{
    Float2 value(12.0f, 13.0f);
    value = value - 10.0f;
    EXPECT_EQ(value, Float2(2.0f, 3.0f));
}

TEST(Vector2, ScalarSubtractionAssignment)
{
    Float2 value(12.0f, 13.0f);
    value -= 10.0f;
    EXPECT_EQ(value, Float2(2.0f, 3.0f));
}

TEST(Vector2, ScalarMultiplication)
{
    Float2 value(2.0f, 3.0f);
    value = value * 10.0f;
    EXPECT_EQ(value, Float2(20.0f, 30.0f));
}

TEST(Vector2, ScalarMultiplicationAssignment)
{
    Float2 value(2.0f, 3.0f);
    value *= 10.0f;
    EXPECT_EQ(value, Float2(20.0f, 30.0f));
}

TEST(Vector2, ScalarDivision)
{
    Float2 value(20.0f, 30.0f);
    value = value / 10.0f;
    EXPECT_EQ(value, Float2(2.0f, 3.0f));
}

TEST(Vector2, ScalarDivisionAssignment)
{
    Float2 value(20.0f, 30.0f);
    value /= 10.0f;
    EXPECT_EQ(value, Float2(2.0f, 3.0f));
}

TEST(Vector2, VectorAddition)
{
    Float2 value1(2.0f, 3.0f);
    Float2 value2(2.0f, 3.0f);
    Float2 result = value1 + value2;
    EXPECT_EQ(result, Float2(4.0f, 6.0f));
}

TEST(Vector2, VectorAdditionAssignment)
{
    Float2 value1(2.0f, 3.0f);
    Float2 value2(2.0f, 3.0f);
    Float2 result = value1;
    result += value2;
    EXPECT_EQ(result, Float2(4.0f, 6.0f));
}

TEST(Vector2, VectorSubtraction)
{
    Float2 value1(4.0f, 6.0f);
    Float2 value2(2.0f, 3.0f);
    Float2 result = value1 - value2;
    EXPECT_EQ(result, Float2(2.0f, 3.0f));
}

TEST(Vector2, VectorSubtractionAssignment)
{
    Float2 value1(4.0f, 6.0f);
    Float2 value2(2.0f, 3.0f);
    Float2 result = value1;
    result -= value2;
    EXPECT_EQ(result, Float2(2.0f, 3.0f));
}

TEST(Vector2, VectorMultiplication)
{
    Float2 value1(2.0f, 3.0f);
    Float2 value2(2.0f, 3.0f);
    Float2 result = value1 * value2;
    EXPECT_EQ(result, Float2(4.0f, 9.0f));
}

TEST(Vector2, VectorMultiplicationAssignment)
{
    Float2 value1(2.0f, 3.0f);
    Float2 value2(2.0f, 3.0f);
    Float2 result = value1;
    result *= value2;
    EXPECT_EQ(result, Float2(4.0f, 9.0f));
}

TEST(Vector2, VectorDivision)
{
    Float2 value1(4.0f, 9.0f);
    Float2 value2(2.0f, 3.0f);
    Float2 result = value1 / value2;
    EXPECT_EQ(result, Float2(2.0f, 3.0f));
}

TEST(Vector2, VectorDivisionAssignment)
{
    Float2 value1(4.0f, 9.0f);
    Float2 value2(2.0f, 3.0f);
    Float2 result = value1;
    result /= value2;
    EXPECT_EQ(result, Float2(2.0f, 3.0f));
}

TEST(Vector2, OperationsDot)
{
    Float2 value(2.0f, 3.0f);
    EXPECT_EQ(value.dot(value), 13.0f);
}

TEST(Vector2, OperationsMagnitude)
{
    Float2 value(2.0f, 3.0f);
    EXPECT_EQ(value.getMagnitude(), 13.0f);
}

TEST(Vector2, OperationsLength)
{
    Float2 value(2.0f, 3.0f);
    EXPECT_EQ(value.getLength(), 3.60555124f);
}

TEST(Vector2, OperationsNormal)
{
    Float2 value(2.0f, 3.0f);
    EXPECT_EQ(value.getNormal(), Float2(0.554700196f, 0.832050323f));
}

TEST(Vector2, OperationsDistance)
{
    EXPECT_EQ(Float2(1.0f, 1.0f).getDistance(Float2(2.0f, 1.0f)), 1.0f);
}

TEST(Vector2, OperationsNormalize)
{
    Float2 value(2.0f, 3.0f);
    value.normalize();
    EXPECT_EQ(value, Float2(0.554700196f, 0.832050323f));
}

TEST(Vector2, OperationsAbsolute)
{
    Float2 value(5.0f, -10.0f);
    EXPECT_EQ(value.getAbsolute(), Float2(5.0f, 10.0f));
}

TEST(Vector2, OperationsClamped)
{
    Float2 value(5.0f, -10.0f);
    EXPECT_EQ(value.getClamped(Float2::Zero, Float2::One), Float2(1.0f, 0.0f));
}

TEST(Vector2, OperationsSaturated)
{
    Float2 value(5.0f, -10.0f);
    EXPECT_EQ(value.getSaturated(), Float2(1.0f, 0.0f));
}

TEST(Vector2, OperationsMinimum)
{
    Float2 value(5.0f, -10.0f);
    EXPECT_EQ(value.getMinimum(Float2::Zero), Float2(0.0f, -10.0f));
}

TEST(Vector2, OperationsMaximum)
{
    Float2 value(5.0f, -10.0f);
    EXPECT_EQ(value.getMaximum(Float2::Zero), Float2(5.0f, 0.0f));
}

static const Float2 lowValue(1.0f, 2.0f);
static const Float2 highValue(2.0f, 3.0f);
TEST(Vector2, ComparisonsLess)
{
    EXPECT_TRUE(lowValue < highValue);
}

TEST(Vector2, ComparisonsLessEqual)
{
    EXPECT_TRUE(lowValue <= highValue);
}

TEST(Vector2, ComparisonsGreater)
{
    EXPECT_TRUE(highValue > lowValue);
}

TEST(Vector2, ComparisonsGreaterEqual)
{
    EXPECT_TRUE(highValue >= lowValue);
}

TEST(Vector2, ComparisonsEqual)
{
    EXPECT_TRUE(lowValue == lowValue);
}

TEST(Vector2, ComparisonsNotEqual)
{
    EXPECT_TRUE(lowValue != highValue);
}