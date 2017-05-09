/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision$
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date$
#pragma once

#include "GEK/Math/Matrix4x4.hpp"
#include <xmmintrin.h>

namespace Gek
{
	namespace Math
	{
        namespace SIMD
        {
            struct Vector
            {
                __m128 x, y, z, w;
            };

            struct Matrix
            {
                Vector x, y, z, w;
            };

            struct Frustum
            {
                Vector planeList[6];
            };

            Vector multiply(Vector const &vector, Matrix const &matrix)
            {
                __m128 x = _mm_mul_ps(vector.x, matrix.x.x);
                x = _mm_add_ps(_mm_mul_ps(vector.y, matrix.y.x), x);
                x = _mm_add_ps(_mm_mul_ps(vector.z, matrix.z.x), x);
                x = _mm_add_ps(_mm_mul_ps(vector.w, matrix.w.x), x);

                __m128 y = _mm_mul_ps(vector.x, matrix.x.y);
                y = x = _mm_add_ps(_mm_mul_ps(vector.y, matrix.y.y), y);
                y = x = _mm_add_ps(_mm_mul_ps(vector.z, matrix.z.y), y);
                y = x = _mm_add_ps(_mm_mul_ps(vector.w, matrix.w.y), y);

                __m128 z = _mm_mul_ps(vector.x, matrix.x.z);
                z = x = _mm_add_ps(_mm_mul_ps(vector.y, matrix.y.z), z);
                z = x = _mm_add_ps(_mm_mul_ps(vector.z, matrix.z.z), z);
                z = x = _mm_add_ps(_mm_mul_ps(vector.w, matrix.w.z), z);

                __m128 w = _mm_mul_ps(vector.x, matrix.x.w);
                w = x = _mm_add_ps(_mm_mul_ps(vector.y, matrix.y.w), w);
                w = x = _mm_add_ps(_mm_mul_ps(vector.z, matrix.z.w), w);
                w = x = _mm_add_ps(_mm_mul_ps(vector.w, matrix.w.w), w);

                return
                {
                    x, y, z, w
                };
            }

            Matrix multiply(Matrix const &leftMatrix, Matrix const &rightMatrix)
            {
                Vector x = multiply(leftMatrix.x, rightMatrix);
                Vector y = multiply(leftMatrix.y, rightMatrix);
                Vector z = multiply(leftMatrix.z, rightMatrix);
                Vector w = multiply(leftMatrix.w, rightMatrix);
                return
                {
                    x, y, z, w
                };
            }

            Frustum loadFrustum(Float4 const planeList[])
            {
                return
                {
                    _mm_set_ps1(planeList[0].x),
                    _mm_set_ps1(planeList[0].y),
                    _mm_set_ps1(planeList[0].z),
                    _mm_set_ps1(planeList[0].w),

                    _mm_set_ps1(planeList[1].x),
                    _mm_set_ps1(planeList[1].y),
                    _mm_set_ps1(planeList[1].z),
                    _mm_set_ps1(planeList[1].w),

                    _mm_set_ps1(planeList[2].x),
                    _mm_set_ps1(planeList[2].y),
                    _mm_set_ps1(planeList[2].z),
                    _mm_set_ps1(planeList[2].w),

                    _mm_set_ps1(planeList[3].x),
                    _mm_set_ps1(planeList[3].y),
                    _mm_set_ps1(planeList[3].z),
                    _mm_set_ps1(planeList[3].w),

                    _mm_set_ps1(planeList[4].x),
                    _mm_set_ps1(planeList[4].y),
                    _mm_set_ps1(planeList[4].z),
                    _mm_set_ps1(planeList[4].w),

                    _mm_set_ps1(planeList[5].x),
                    _mm_set_ps1(planeList[5].y),
                    _mm_set_ps1(planeList[5].z),
                    _mm_set_ps1(planeList[5].w),
                };
            }

            template <typename FLOATS, typename BOOLEANS>
            void cullSpheres(Frustum const &frustumData,
                size_t objectCount,
                FLOATS const &shapeXPositionList,
                FLOATS const &shapeYPositionList,
                FLOATS const &shapeZPositionList,
                FLOATS const &shapeRadiusList,
                BOOLEANS &visibilityList)
            {
                static const auto AllZero = _mm_setzero_ps();
                for (size_t objectBase = 0; objectBase < objectCount; objectBase += 4)
                {
                    const auto shapeXPosition = _mm_load_ps(&shapeXPositionList[objectBase]);
                    const auto shapeYPosition = _mm_load_ps(&shapeYPositionList[objectBase]);
                    const auto shapeZPosition = _mm_load_ps(&shapeZPositionList[objectBase]);
                    const auto shapeRadius = _mm_load_ps(&shapeRadiusList[objectBase]);
                    const auto negativeShapeRadius = _mm_sub_ps(AllZero, shapeRadius);

                    auto intersectionResult = AllZero;
                    for (uint32_t plane = 0; plane < 6; ++plane)
                    {
                        // Plane.Normal.Dot(Sphere.Position)
                        const auto dotX = _mm_mul_ps(shapeXPosition, frustumData.planeList[plane].x);
                        const auto dotY = _mm_mul_ps(shapeYPosition, frustumData.planeList[plane].y);
                        const auto dotZ = _mm_mul_ps(shapeZPosition, frustumData.planeList[plane].z);
                        const auto dotXY = _mm_add_ps(dotX, dotY);
                        const auto dotProduct = _mm_add_ps(dotXY, dotZ);

                        // + Plane.Distance
                        const auto planeDistance = _mm_add_ps(dotProduct, frustumData.planeList[plane].w);

                        // < -Sphere.Radius
                        const auto planeTest = _mm_cmplt_ps(planeDistance, negativeShapeRadius);
                        intersectionResult = _mm_or_ps(intersectionResult, planeTest);
                    }

                    __declspec(align(16)) uint32_t resultValues[4];
                    _mm_store_ps((float *)resultValues, intersectionResult);
                    for (uint32_t sectionIndex = 0; sectionIndex < 4; ++sectionIndex)
                    {
                        visibilityList[objectBase + sectionIndex] = !resultValues[sectionIndex];
                    }
                }
            }

            void getViewSpaceRect(Matrix const &workdViewProjectionMatrix, Vector const &minimum, Vector const &maximum, Vector result[])
            {
                auto m_xx_x = _mm_mul_ps(workdViewProjectionMatrix.x.x, minimum.x);    m_xx_x = _mm_add_ps(m_xx_x, workdViewProjectionMatrix.w.x);
                auto m_xy_x = _mm_mul_ps(workdViewProjectionMatrix.x.y, minimum.x);    m_xy_x = _mm_add_ps(m_xy_x, workdViewProjectionMatrix.w.y);
                auto m_xz_x = _mm_mul_ps(workdViewProjectionMatrix.x.z, minimum.x);    m_xz_x = _mm_add_ps(m_xz_x, workdViewProjectionMatrix.w.z);
                auto m_xw_x = _mm_mul_ps(workdViewProjectionMatrix.x.w, minimum.x);    m_xw_x = _mm_add_ps(m_xw_x, workdViewProjectionMatrix.w.w);

                auto m_xx_X = _mm_mul_ps(workdViewProjectionMatrix.x.x, maximum.x);    m_xx_X = _mm_add_ps(m_xx_X, workdViewProjectionMatrix.w.x);
                auto m_xy_X = _mm_mul_ps(workdViewProjectionMatrix.x.y, maximum.x);    m_xy_X = _mm_add_ps(m_xy_X, workdViewProjectionMatrix.w.y);
                auto m_xz_X = _mm_mul_ps(workdViewProjectionMatrix.x.z, maximum.x);    m_xz_X = _mm_add_ps(m_xz_X, workdViewProjectionMatrix.w.z);
                auto m_xw_X = _mm_mul_ps(workdViewProjectionMatrix.x.w, maximum.x);    m_xw_X = _mm_add_ps(m_xw_X, workdViewProjectionMatrix.w.w);

                auto m_yx_y = _mm_mul_ps(workdViewProjectionMatrix.y.x, minimum.y);
                auto m_yy_y = _mm_mul_ps(workdViewProjectionMatrix.y.y, minimum.y);
                auto m_yz_y = _mm_mul_ps(workdViewProjectionMatrix.y.z, minimum.y);
                auto m_yw_y = _mm_mul_ps(workdViewProjectionMatrix.y.w, minimum.y);

                auto m_yx_Y = _mm_mul_ps(workdViewProjectionMatrix.y.x, maximum.y);
                auto m_yy_Y = _mm_mul_ps(workdViewProjectionMatrix.y.y, maximum.y);
                auto m_yz_Y = _mm_mul_ps(workdViewProjectionMatrix.y.z, maximum.y);
                auto m_yw_Y = _mm_mul_ps(workdViewProjectionMatrix.y.w, maximum.y);

                auto m_zx_z = _mm_mul_ps(workdViewProjectionMatrix.z.x, minimum.z);
                auto m_zy_z = _mm_mul_ps(workdViewProjectionMatrix.z.y, minimum.z);
                auto m_zz_z = _mm_mul_ps(workdViewProjectionMatrix.z.z, minimum.z);
                auto m_zw_z = _mm_mul_ps(workdViewProjectionMatrix.z.w, minimum.z);

                auto m_zx_Z = _mm_mul_ps(workdViewProjectionMatrix.z.x, maximum.z);
                auto m_zy_Z = _mm_mul_ps(workdViewProjectionMatrix.z.y, maximum.z);
                auto m_zz_Z = _mm_mul_ps(workdViewProjectionMatrix.z.z, maximum.z);
                auto m_zw_Z = _mm_mul_ps(workdViewProjectionMatrix.z.w, maximum.z);

                auto x = _mm_add_ps(m_xx_x, m_yx_y);   x = _mm_add_ps(x, m_zx_z);
                auto y = _mm_add_ps(m_xy_x, m_yy_y);   y = _mm_add_ps(y, m_zy_z);
                auto z = _mm_add_ps(m_xz_x, m_yz_y);   z = _mm_add_ps(z, m_zz_z);
                auto w = _mm_add_ps(m_xw_x, m_yw_y);   w = _mm_add_ps(w, m_zw_z);
                result[0].x = x;
                result[0].y = y;
                result[0].z = z;
                result[0].w = w;

                x = _mm_add_ps(m_xx_X, m_yx_y);   x = _mm_add_ps(x, m_zx_z);
                y = _mm_add_ps(m_xy_X, m_yy_y);   y = _mm_add_ps(y, m_zy_z);
                z = _mm_add_ps(m_xz_X, m_yz_y);   z = _mm_add_ps(z, m_zz_z);
                w = _mm_add_ps(m_xw_X, m_yw_y);   w = _mm_add_ps(w, m_zw_z);
                result[1].x = x;
                result[1].y = y;
                result[1].z = z;
                result[1].w = w;

                x = _mm_add_ps(m_xx_x, m_yx_Y);   x = _mm_add_ps(x, m_zx_z);
                y = _mm_add_ps(m_xy_x, m_yy_Y);   y = _mm_add_ps(y, m_zy_z);
                z = _mm_add_ps(m_xz_x, m_yz_Y);   z = _mm_add_ps(z, m_zz_z);
                w = _mm_add_ps(m_xw_x, m_yw_Y);   w = _mm_add_ps(w, m_zw_z);
                result[2].x = x;
                result[2].y = y;
                result[2].z = z;
                result[2].w = w;

                x = _mm_add_ps(m_xx_X, m_yx_Y);   x = _mm_add_ps(x, m_zx_z);
                y = _mm_add_ps(m_xy_X, m_yy_Y);   y = _mm_add_ps(y, m_zy_z);
                z = _mm_add_ps(m_xz_X, m_yz_Y);   z = _mm_add_ps(z, m_zz_z);
                w = _mm_add_ps(m_xw_X, m_yw_Y);   w = _mm_add_ps(w, m_zw_z);
                result[3].x = x;
                result[3].y = y;
                result[3].z = z;
                result[3].w = w;

                x = _mm_add_ps(m_xx_x, m_yx_y);   x = _mm_add_ps(x, m_zx_Z);
                y = _mm_add_ps(m_xy_x, m_yy_y);   y = _mm_add_ps(y, m_zy_Z);
                z = _mm_add_ps(m_xz_x, m_yz_y);   z = _mm_add_ps(z, m_zz_Z);
                w = _mm_add_ps(m_xw_x, m_yw_y);   w = _mm_add_ps(w, m_zw_Z);
                result[4].x = x;
                result[4].y = y;
                result[4].z = z;
                result[4].w = w;

                x = _mm_add_ps(m_xx_X, m_yx_y);   x = _mm_add_ps(x, m_zx_Z);
                y = _mm_add_ps(m_xy_X, m_yy_y);   y = _mm_add_ps(y, m_zy_Z);
                z = _mm_add_ps(m_xz_X, m_yz_y);   z = _mm_add_ps(z, m_zz_Z);
                w = _mm_add_ps(m_xw_X, m_yw_y);   w = _mm_add_ps(w, m_zw_Z);
                result[5].x = x;
                result[5].y = y;
                result[5].z = z;
                result[5].w = w;

                x = _mm_add_ps(m_xx_x, m_yx_Y);   x = _mm_add_ps(x, m_zx_Z);
                y = _mm_add_ps(m_xy_x, m_yy_Y);   y = _mm_add_ps(y, m_zy_Z);
                z = _mm_add_ps(m_xz_x, m_yz_Y);   z = _mm_add_ps(z, m_zz_Z);
                w = _mm_add_ps(m_xw_x, m_yw_Y);   w = _mm_add_ps(w, m_zw_Z);
                result[6].x = x;
                result[6].y = y;
                result[6].z = z;
                result[6].w = w;

                x = _mm_add_ps(m_xx_X, m_yx_Y);   x = _mm_add_ps(x, m_zx_Z);
                y = _mm_add_ps(m_xy_X, m_yy_Y);   y = _mm_add_ps(y, m_zy_Z);
                z = _mm_add_ps(m_xz_X, m_yz_Y);   z = _mm_add_ps(z, m_zz_Z);
                w = _mm_add_ps(m_xw_X, m_yw_Y);   w = _mm_add_ps(w, m_zw_Z);
                result[7].x = x;
                result[7].y = y;
                result[7].z = z;
                result[7].w = w;
            }

            template <typename FLOATS, typename BOOLEANS>
            void cullOrientedBoundingBoxes(
                Float4x4 const &viewMatrix,
                Float4x4 const &projectionMatrix,
                size_t objectCount,
                FLOATS const &halfSizeXList,
                FLOATS const &halfSizeYList,
                FLOATS const &halfSizeZList,
                FLOATS const * const transformList,
                BOOLEANS &visibilityList)
            {
                auto combinedMatrix(viewMatrix * projectionMatrix);
                const Matrix viewProjectionMatrix =
                {
                    _mm_set_ps1(combinedMatrix.rx.x),
                    _mm_set_ps1(combinedMatrix.rx.y),
                    _mm_set_ps1(combinedMatrix.rx.z),
                    _mm_set_ps1(combinedMatrix.rx.w),

                    _mm_set_ps1(combinedMatrix.ry.x),
                    _mm_set_ps1(combinedMatrix.ry.y),
                    _mm_set_ps1(combinedMatrix.ry.z),
                    _mm_set_ps1(combinedMatrix.ry.w),

                    _mm_set_ps1(combinedMatrix.rz.x),
                    _mm_set_ps1(combinedMatrix.rz.y),
                    _mm_set_ps1(combinedMatrix.rz.z),
                    _mm_set_ps1(combinedMatrix.rz.w),

                    _mm_set_ps1(combinedMatrix.rw.x),
                    _mm_set_ps1(combinedMatrix.rw.y),
                    _mm_set_ps1(combinedMatrix.rw.z),
                    _mm_set_ps1(combinedMatrix.rw.w),
                };

                for (uint32_t objectBase = 0; objectBase < objectCount; objectBase += 4)
                {
                    Matrix workdMatrix;
                    workdMatrix.x.x = _mm_load_ps(&transformList[0][objectBase]);
                    workdMatrix.x.y = _mm_load_ps(&transformList[1][objectBase]);
                    workdMatrix.x.z = _mm_load_ps(&transformList[2][objectBase]);
                    workdMatrix.x.w = _mm_load_ps(&transformList[3][objectBase]);

                    workdMatrix.y.x = _mm_load_ps(&transformList[4][objectBase]);
                    workdMatrix.y.y = _mm_load_ps(&transformList[5][objectBase]);
                    workdMatrix.y.z = _mm_load_ps(&transformList[6][objectBase]);
                    workdMatrix.y.w = _mm_load_ps(&transformList[7][objectBase]);

                    workdMatrix.z.x = _mm_load_ps(&transformList[8][objectBase]);
                    workdMatrix.z.y = _mm_load_ps(&transformList[9][objectBase]);
                    workdMatrix.z.z = _mm_load_ps(&transformList[10][objectBase]);
                    workdMatrix.z.w = _mm_load_ps(&transformList[11][objectBase]);

                    workdMatrix.w.x = _mm_load_ps(&transformList[12][objectBase]);
                    workdMatrix.w.y = _mm_load_ps(&transformList[13][objectBase]);
                    workdMatrix.w.z = _mm_load_ps(&transformList[14][objectBase]);
                    workdMatrix.w.w = _mm_load_ps(&transformList[15][objectBase]);

                    const auto workdViewProjectionMatrix = multiply(workdMatrix, viewProjectionMatrix);

                    static const auto AllZero = _mm_setzero_ps();

                    // Load the mininum and maximum corner positions of the bounding box in object space.
                    Vector minimum;
                    minimum.x = _mm_sub_ps(AllZero, _mm_load_ps(&halfSizeXList[objectBase]));
                    minimum.y = _mm_sub_ps(AllZero, _mm_load_ps(&halfSizeYList[objectBase]));
                    minimum.z = _mm_sub_ps(AllZero, _mm_load_ps(&halfSizeZList[objectBase]));
                    minimum.w = _mm_set_ps1(1.0f);

                    Vector maximum;
                    maximum.x = _mm_load_ps(&halfSizeXList[objectBase]);
                    maximum.y = _mm_load_ps(&halfSizeYList[objectBase]);
                    maximum.z = _mm_load_ps(&halfSizeZList[objectBase]);
                    maximum.w = _mm_set_ps1(1.0f);

                    Vector viewSpaceBox[8];
                    // Transform each bounding box corner from object to workdViewProjectionMatrix space by sharing calculations.
                    getViewSpaceRect(workdViewProjectionMatrix, minimum, maximum, viewSpaceBox);

                    static const auto AllTrue = _mm_cmpeq_ps(AllZero, AllZero);

                    // Initialize test conditions.
                    auto areAllXLess = AllTrue;
                    auto areAllXGreater = AllTrue;
                    auto areAllYLess = AllTrue;
                    auto areAllYGreater = AllTrue;
                    auto areAllZLess = AllTrue;
                    auto areAllZGreater = AllTrue;
                    auto areAnyZLess = _mm_cmpgt_ps(AllZero, AllZero);

                    // Test each corner of the oobb and if any corner intersects the frustum that object
                    // is visible.
                    for (unsigned edge = 0; edge < 8; ++edge)
                    {
                        const auto negativeEdgeW = _mm_sub_ps(AllZero, viewSpaceBox[edge].w);

                        auto isXLess = _mm_cmple_ps(viewSpaceBox[edge].x, negativeEdgeW);
                        auto isXGreater = _mm_cmpge_ps(viewSpaceBox[edge].x, viewSpaceBox[edge].w);
                        areAllXLess = _mm_and_ps(isXLess, areAllXLess);
                        areAllXGreater = _mm_and_ps(isXGreater, areAllXGreater);

                        auto isYLess = _mm_cmple_ps(viewSpaceBox[edge].y, negativeEdgeW);
                        auto isYGreater = _mm_cmpge_ps(viewSpaceBox[edge].y, viewSpaceBox[edge].w);
                        areAllYLess = _mm_and_ps(isYLess, areAllYLess);
                        areAllYGreater = _mm_and_ps(isYGreater, areAllYGreater);

                        auto isZLess = _mm_cmple_ps(viewSpaceBox[edge].z, AllZero);
                        auto isZGreater = _mm_cmpge_ps(viewSpaceBox[edge].z, viewSpaceBox[edge].w);
                        areAllZLess = _mm_and_ps(isZLess, areAllZLess);
                        areAllZGreater = _mm_and_ps(isZGreater, areAllZGreater);
                        areAnyZLess = _mm_or_ps(isZLess, areAnyZLess);
                    }

                    const auto areAnyXOutside = _mm_or_ps(areAllXLess, areAllXGreater);
                    const auto areAnyYOutside = _mm_or_ps(areAllYLess, areAllYGreater);
                    const auto areAnyZOutside = _mm_or_ps(areAllZLess, areAllZGreater);
                    auto isOutside = _mm_or_ps(areAnyXOutside, areAnyYOutside);
                    isOutside = _mm_or_ps(isOutside, areAnyZOutside);

                    const auto isInside = _mm_xor_ps(isOutside, AllTrue);

                    __declspec(align(16)) uint32_t insideValues[4];
                    _mm_store_ps((float *)insideValues, isInside);
                    for (size_t sectionIndex = 0; sectionIndex < 4; ++sectionIndex)
                    {
                        visibilityList[objectBase + sectionIndex] = !insideValues[sectionIndex];
                    }
                }
            }
        }; // namespace SIMD
    }; // namespace Math
}; // namespace Gek
