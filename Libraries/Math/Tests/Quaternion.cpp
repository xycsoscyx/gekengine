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

TEST(Quaternion, RotationOperations)
{
    Quaternion qx(Quaternion::MakeAngularRotation(Float3(1.0, 0.0, 0.0), Pi / 3.0f));
    EXPECT_NEAR(qx.w, std::sqrt(3.0) / 2, Epsilon);
    EXPECT_NEAR(qx.x, 0.5, Epsilon);
    EXPECT_NEAR(qx.y, 0.0, Epsilon);
    EXPECT_NEAR(qx.z, 0.0, Epsilon);

    Quaternion qy(Quaternion::MakeAngularRotation(Float3(0.0, 1.0, 0.0), Pi / 2.0f));
    EXPECT_NEAR(qy.w, 1 / std::sqrt(2.0), Epsilon);
    EXPECT_NEAR(qy.x, 0.0, Epsilon);
    EXPECT_NEAR(qy.y, 1 / std::sqrt(2.0), Epsilon);
    EXPECT_NEAR(qy.z, 0.0, Epsilon);

    Quaternion qz(Quaternion::MakeAngularRotation(Float3(0.0, 0.0, 1.0), Pi));
    EXPECT_NEAR(qz.w, 0.0, Epsilon);
    EXPECT_NEAR(qz.x, 0.0, Epsilon);
    EXPECT_NEAR(qz.y, 0.0, Epsilon);
    EXPECT_NEAR(qz.z, 1.0, Epsilon);

    Quaternion qnull(Quaternion::MakeAngularRotation(Float3(1.0, 0.0, 0.0), 0.0f));
    EXPECT_NEAR(qnull.w, 1.0, Epsilon);
    EXPECT_NEAR(qnull.x, 0.0, Epsilon);
    EXPECT_NEAR(qnull.y, 0.0, Epsilon);
    EXPECT_NEAR(qnull.z, 0.0, Epsilon);
}

TEST(Quaternion, MultiplyOperations)
{
    Quaternion qa(1.0f / std::sqrt(2.0f), 0.0f, 0.0f, 1.0f / std::sqrt(2.0f));
    Quaternion qb(0.0f, 1.0f / std::sqrt(2.0f), 0.0f, 1.0f / std::sqrt(2.0f));

    Quaternion qc = qa * qb;
    EXPECT_NEAR(qc.w, 0.5, Epsilon);
    EXPECT_NEAR(qc.x, 0.5, Epsilon);
    EXPECT_NEAR(qc.y, 0.5, Epsilon);
    EXPECT_NEAR(qc.z, 0.5, Epsilon);

    qc = qa * qa.getConjugate();
    EXPECT_NEAR(qc.w, 1.0, Epsilon);
    EXPECT_NEAR(qc.x, 0.0, Epsilon);
    EXPECT_NEAR(qc.y, 0.0, Epsilon);
    EXPECT_NEAR(qc.x, 0.0, Epsilon);

    qc = qa * qa.getInverse();
    EXPECT_NEAR(qc.w, 1.0, Epsilon);
    EXPECT_NEAR(qc.x, 0.0, Epsilon);
    EXPECT_NEAR(qc.y, 0.0, Epsilon);
    EXPECT_NEAR(qc.x, 0.0, Epsilon);
}