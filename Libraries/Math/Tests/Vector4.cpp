#include "GEK/Math/Vector4.hpp"
#include <gtest/gtest.h>

using namespace Gek::Math;

TEST(Vector4, CheckZero)
{
    EXPECT_EQ(Float4::Zero.x, 0.0f);
    EXPECT_EQ(Float4::Zero.y, 0.0f);
    EXPECT_EQ(Float4::Zero.z, 0.0f);
    EXPECT_EQ(Float4::Zero.w, 0.0f);
}

TEST(Vector4, CheckOne)
{
    EXPECT_EQ(Float4::One.x, 1.0f);
    EXPECT_EQ(Float4::One.y, 1.0f);
    EXPECT_EQ(Float4::One.z, 1.0f);
    EXPECT_EQ(Float4::One.w, 1.0f);
}

TEST(Vector4, CheckConstructor)
{
    Float4 value(-1.0f, -2.0f, -3.0f, -4.0f);
    EXPECT_EQ(value.x, -1.0f);
    EXPECT_EQ(value.y, -2.0f);
    EXPECT_EQ(value.z, -3.0f);
    EXPECT_EQ(value.w, -4.0f);
}

TEST(Vector4, CheckAssignment)
{
    Float4 value = Float4::Zero;
    EXPECT_EQ(value.x, 0.0f);
    EXPECT_EQ(value.y, 0.0f);
    EXPECT_EQ(value.z, 0.0f);
    EXPECT_EQ(value.w, 0.0f);
}

TEST(Vector4, ScalarAddition)
{
    Float4 value(2.0f, 3.0f, 4.0f, 5.0f);
    value = value + 10.0f;
    EXPECT_EQ(value, Float4(12.0f, 13.0f, 14.0f, 15.0f));
}

TEST(Vector4, ScalarAdditionAssignment)
{
    Float4 value(2.0f, 3.0f, 4.0f, 5.0f);
    value += 10.0f;
    EXPECT_EQ(value, Float4(12.0f, 13.0f, 14.0f, 15.0f));
}

TEST(Vector4, ScalarSubtraction)
{
    Float4 value(12.0f, 13.0f, 14.0f, 15.0f);
    value = value - 10.0f;
    EXPECT_EQ(value, Float4(2.0f, 3.0f, 4.0f, 5.0f));
}

TEST(Vector4, ScalarSubtractionAssignment)
{
    Float4 value(12.0f, 13.0f, 14.0f, 15.0f);
    value -= 10.0f;
    EXPECT_EQ(value, Float4(2.0f, 3.0f, 4.0f, 5.0f));
}

TEST(Vector4, ScalarMultiplication)
{
    Float4 value(2.0f, 3.0f, 4.0f, 5.0f);
    value = value * 10.0f;
    EXPECT_EQ(value, Float4(20.0f, 30.0f, 40.0f, 50.0f));
}

TEST(Vector4, ScalarMultiplicationAssignment)
{
    Float4 value(2.0f, 3.0f, 4.0f, 5.0f);
    value *= 10.0f;
    EXPECT_EQ(value, Float4(20.0f, 30.0f, 40.0f, 50.0f));
}

TEST(Vector4, ScalarDivision)
{
    Float4 value(20.0f, 30.0f, 40.0f, 50.0f);
    value = value / 10.0f;
    EXPECT_EQ(value, Float4(2.0f, 3.0f, 4.0f, 5.0f));
}

TEST(Vector4, ScalarDivisionAssignment)
{
    Float4 value(20.0f, 30.0f, 40.0f, 50.0f);
    value /= 10.0f;
    EXPECT_EQ(value, Float4(2.0f, 3.0f, 4.0f, 5.0f));
}

TEST(Vector4, VectorAddition)
{
    Float4 value1(2.0f, 3.0f, 4.0f, 5.0f);
    Float4 value2(2.0f, 3.0f, 4.0f, 5.0f);
    Float4 result = value1 + value2;
    EXPECT_EQ(result, Float4(4.0f, 6.0f, 8.0f, 10.0f));
}

TEST(Vector4, VectorAdditionAssignment)
{
    Float4 value1(2.0f, 3.0f, 4.0f, 5.0f);
    Float4 value2(2.0f, 3.0f, 4.0f, 5.0f);
    Float4 result = value1;
    result += value2;
    EXPECT_EQ(result, Float4(4.0f, 6.0f, 8.0f, 10.0f));
}

TEST(Vector4, VectorSubtraction)
{
    Float4 value1(4.0f, 6.0f, 8.0f, 10.0f);
    Float4 value2(2.0f, 3.0f, 4.0f, 5.0f);
    Float4 result = value1 - value2;
    EXPECT_EQ(result, Float4(2.0f, 3.0f, 4.0f, 5.0f));
}

TEST(Vector4, VectorSubtractionAssignment)
{
    Float4 value1(4.0f, 6.0f, 8.0f, 10.0f);
    Float4 value2(2.0f, 3.0f, 4.0f, 5.0f);
    Float4 result = value1;
    result -= value2;
    EXPECT_EQ(result, Float4(2.0f, 3.0f, 4.0f, 5.0f));
}

TEST(Vector4, VectorMultiplication)
{
    Float4 value1(2.0f, 3.0f, 4.0f, 5.0f);
    Float4 value2(2.0f, 3.0f, 4.0f, 5.0f);
    Float4 result = value1 * value2;
    EXPECT_EQ(result, Float4(4.0f, 9.0f, 16.0f, 25.0f));
}

TEST(Vector4, VectorMultiplicationAssignment)
{
    Float4 value1(2.0f, 3.0f, 4.0f, 5.0f);
    Float4 value2(2.0f, 3.0f, 4.0f, 5.0f);
    Float4 result = value1;
    result *= value2;
    EXPECT_EQ(result, Float4(4.0f, 9.0f, 16.0f, 25.0f));
}

TEST(Vector4, VectorDivision)
{
    Float4 value1(4.0f, 9.0f, 16.0f, 25.0f);
    Float4 value2(2.0f, 3.0f, 4.0f, 5.0f);
    Float4 result = value1 / value2;
    EXPECT_EQ(result, Float4(2.0f, 3.0f, 4.0f, 5.0f));
}

TEST(Vector4, VectorDivisionAssignment)
{
    Float4 value1(4.0f, 9.0f, 16.0f, 25.0f);
    Float4 value2(2.0f, 3.0f, 4.0f, 5.0f);
    Float4 result = value1;
    result /= value2;
    EXPECT_EQ(result, Float4(2.0f, 3.0f, 4.0f, 5.0f));
}

TEST(Vector4, OperationsDot)
{
    Float4 value(2.0f, 3.0f, 4.0f, 5.0f);
    EXPECT_EQ(value.dot(value), 54.0f);
}

TEST(Vector4, OperationsMagnitude)
{
    Float4 value(2.0f, 3.0f, 4.0f, 5.0f);
    EXPECT_EQ(value.getMagnitude(), 54.0f);
}

TEST(Vector4, OperationsLength)
{
    Float4 value(2.0f, 3.0f, 4.0f, 5.0f);
    EXPECT_EQ(value.getLength(), 7.34846926f);
}

TEST(Vector4, OperationsNormal)
{
    Float4 value(2.0f, 3.0f, 4.0f, 5.0f);
    EXPECT_EQ(value.getNormal(), Float4(0.272165537f, 0.408248305f, 0.544331074f, 0.680413842f));
}

TEST(Vector4, OperationsDistance)
{
    EXPECT_EQ(Float4(1.0f, 1.0f, 1.0f, 1.0f).getDistance(Float4(2.0f, 1.0f, 1.0f, 1.0f)), 1.0f);
}

TEST(Vector4, OperationsNormalize)
{
    Float4 value(2.0f, 3.0f, 4.0f, 5.0f);
    value.normalize();
    EXPECT_EQ(value, Float4(0.272165537f, 0.408248305f, 0.544331074f, 0.680413842f));
}

TEST(Vector4, OperationsAbsolute)
{
    Float4 value(5.0f, -10.0f, 15.0f, -20.0f);
    EXPECT_EQ(value.getAbsolute(), Float4(5.0f, 10.0f, 15.0f, 20.0f));
}

TEST(Vector4, OperationsClamped)
{
    Float4 value(5.0f, -10.0f, 15.0f, -20.0f);
    EXPECT_EQ(value.getClamped(Float4::Zero, Float4::One), Float4(1.0f, 0.0f, 1.0f, 0.0f));
}

TEST(Vector4, OperationsSaturated)
{
    Float4 value(5.0f, -10.0f, 15.0f, -20.0f);
    EXPECT_EQ(value.getSaturated(), Float4(1.0f, 0.0f, 1.0f, 0.0f));
}

TEST(Vector4, OperationsMinimum)
{
    Float4 value(5.0f, -10.0f, 15.0f, -20.0f);
    EXPECT_EQ(value.getMinimum(Float4::Zero), Float4(0.0f, -10.0f, 0.0f, -20.0f));
}

TEST(Vector4, OperationsMaximum)
{
    Float4 value(5.0f, -10.0f, 15.0f, -20.0f);
    EXPECT_EQ(value.getMaximum(Float4::Zero), Float4(5.0f, 0.0f, 15.0f, 0.0f));
}

static const Float4 lowValue(1.0f, 2.0f, 3.0f, 4.0f);
static const Float4 highValue(2.0f, 3.0f, 4.0f, 5.0f);
TEST(Vector4, ComparisonsLess)
{
    EXPECT_TRUE(lowValue < highValue);
}

TEST(Vector4, ComparisonsLessEqual)
{
    EXPECT_TRUE(lowValue <= highValue);
}

TEST(Vector4, ComparisonsGreater)
{
    EXPECT_TRUE(highValue > lowValue);
}

TEST(Vector4, ComparisonsGreaterEqual)
{
    EXPECT_TRUE(highValue >= lowValue);
}

TEST(Vector4, ComparisonsEqual)
{
    EXPECT_TRUE(lowValue == lowValue);
}

TEST(Vector4, ComparisonsNotEqual)
{
    EXPECT_TRUE(lowValue != highValue);
}