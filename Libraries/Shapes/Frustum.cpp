#include "GEK/Shapes/Frustum.hpp"

namespace Gek
{
    namespace Shapes
    {
        Frustum::Frustum(void)
        {
        }

        Frustum::Frustum(const Frustum &frustum)
        {
            for(uint32_t index = 0; index < 6; index++)
            {
                planeList[index] = frustum.planeList[index];
            }
        }

        Frustum::Frustum(Math::Float4x4 const &perspectiveTransform)
        {
            create(perspectiveTransform);
        }

        void Frustum::create(Math::Float4x4 const &perspectiveTransform)
        {
            // Left clipping plane
            planeList[0].a = perspectiveTransform._14 + perspectiveTransform._11;
            planeList[0].b = perspectiveTransform._24 + perspectiveTransform._21;
            planeList[0].c = perspectiveTransform._34 + perspectiveTransform._31;
            planeList[0].d = perspectiveTransform._44 + perspectiveTransform._41;
            
            // Right clipping plane
            planeList[1].a = perspectiveTransform._14 - perspectiveTransform._11;
            planeList[1].b = perspectiveTransform._24 - perspectiveTransform._21;
            planeList[1].c = perspectiveTransform._34 - perspectiveTransform._31;
            planeList[1].d = perspectiveTransform._44 - perspectiveTransform._41;
            
            // Bottom clipping plane
            planeList[3].a = perspectiveTransform._14 + perspectiveTransform._12;
            planeList[3].b = perspectiveTransform._24 + perspectiveTransform._22;
            planeList[3].c = perspectiveTransform._34 + perspectiveTransform._32;
            planeList[3].d = perspectiveTransform._44 + perspectiveTransform._42;

            // Top clipping plane
            planeList[2].a = perspectiveTransform._14 - perspectiveTransform._12;
            planeList[2].b = perspectiveTransform._24 - perspectiveTransform._22;
            planeList[2].c = perspectiveTransform._34 - perspectiveTransform._32;
            planeList[2].d = perspectiveTransform._44 - perspectiveTransform._42;
            
            // Near clipping plane
            planeList[4].a = perspectiveTransform._13;
            planeList[4].b = perspectiveTransform._23; 
            planeList[4].c = perspectiveTransform._33;
            planeList[4].d = perspectiveTransform._43;
            
            // Far clipping plane
            planeList[5].a = perspectiveTransform._14 - perspectiveTransform._13;
            planeList[5].b = perspectiveTransform._24 - perspectiveTransform._23;
            planeList[5].c = perspectiveTransform._34 - perspectiveTransform._33;
            planeList[5].d = perspectiveTransform._44 - perspectiveTransform._43;

            for (auto &plane : planeList)
            {
                plane.normalize();
            }
        }

        void Frustum::cull(const std::vector<Sphere, AlignedAllocator<Sphere, 16>> &shapeList, std::vector<uint32_t, AlignedAllocator<uint32_t, 16>> &visibilityList)
        {
            //to optimize calculations we gather xyzw elements in separate vectors
            static const __m128 Zero = _mm_setzero_ps();

            auto objectData = reinterpret_cast<const float *>(shapeList.data());
            auto visibilityData = visibilityList.data();

            __m128 frustumPlanesA[6];
            __m128 frustumPlanesB[6];
            __m128 frustumPlanesC[6];
            __m128 frustumPlanesD[6];
            for (uint32_t plane = 0; plane < 6; plane++)
            {
                frustumPlanesA[plane] = _mm_set1_ps(planeList[plane].normal.x);
                frustumPlanesB[plane] = _mm_set1_ps(planeList[plane].normal.y);
                frustumPlanesC[plane] = _mm_set1_ps(planeList[plane].normal.z);
                frustumPlanesD[plane] = _mm_set1_ps(planeList[plane].distance);
            }

            //we process 4 objects per step
            auto shapeCount = shapeList.size();
            for (uint32_t shape = 0; shape < shapeCount; shape += 4)
            {
                //load bounding sphere data
                __m128 spherePositionX = _mm_load_ps(objectData);
                __m128 spherePositionY = _mm_load_ps(objectData + 4);
                __m128 spherePositionZ = _mm_load_ps(objectData + 8);
                __m128 sphereRadius = _mm_load_ps(objectData + 12);
                objectData += 16;

                //but for our calculations we need transpose data, to collect x, y, z and w coordinates in separate vectors
                _MM_TRANSPOSE4_PS(spherePositionX, spherePositionY, spherePositionZ, sphereRadius);

                // negate all elements
                //http://fastcpp.blogspot.ru/2011/03/changing-sign-of-float-values-using-sse.html
                __m128 spheresNegatedRadius = _mm_sub_ps(Zero, sphereRadius);

                __m128 intersectionResult = _mm_setzero_ps();
                for (uint32_t plane = 0; plane < 6; plane++)
                {
                    //1. calc distance to plane dot(sphere_pos.xyz, plane.xyz) + plane.w
                    //2. if distance < sphere radius, then sphere outside frustum
                    __m128 dotProductX = _mm_mul_ps(spherePositionX, frustumPlanesA[plane]);
                    __m128 dotProductY = _mm_mul_ps(spherePositionY, frustumPlanesB[plane]);
                    __m128 dotProductZ = _mm_mul_ps(spherePositionZ, frustumPlanesC[plane]);

                    __m128 sumationXY = _mm_add_ps(dotProductX, dotProductY);
                    __m128 sumationZW = _mm_add_ps(dotProductZ, frustumPlanesD[plane]);
                    __m128 distanceToPlane = _mm_add_ps(sumationXY, sumationZW);

                    __m128 planeResult = _mm_cmple_ps(distanceToPlane, spheresNegatedRadius); //dist < -sphere_r ?
                    intersectionResult = _mm_or_ps(intersectionResult, planeResult); //if yes - sphere behind the plane & outside frustum
                }

                //store result
                __m128i frustumResult = _mm_cvtps_epi32(intersectionResult);
                _mm_store_si128((__m128i *)&visibilityData[shape], frustumResult);
            }
        }

        void Frustum::cull(const std::vector<AlignedBox, AlignedAllocator<AlignedBox, 16>> &shapeList, std::vector<uint32_t, AlignedAllocator<uint32_t, 16>> &visibilityList)
        {
            //to optimize calculations we gather xyzw elements in separate vectors
            static const __m128 Zero = _mm_setzero_ps();

            auto objectData = reinterpret_cast<const float *>(shapeList.data());
            auto visibilityData = visibilityList.data();

            __m128 frustumPlanesA[6];
            __m128 frustumPlanesB[6];
            __m128 frustumPlanesC[6];
            __m128 frustumPlanesD[6];
            for (uint32_t plane = 0; plane < 6; plane++)
            {
                frustumPlanesA[plane] = _mm_set1_ps(planeList[plane].normal.x);
                frustumPlanesB[plane] = _mm_set1_ps(planeList[plane].normal.y);
                frustumPlanesC[plane] = _mm_set1_ps(planeList[plane].normal.z);
                frustumPlanesD[plane] = _mm_set1_ps(planeList[plane].distance);
            }

            //we process 4 objects per step
            auto shapeCount = shapeList.size();
            for (uint32_t shape = 0; shape < shapeCount; shape += 4)
            {
                //load objects data
                //load aabb min
                __m128 boxMinimumX = _mm_load_ps(objectData);
                __m128 boxMinimumY = _mm_load_ps(objectData + 8);
                __m128 boxMinimumZ = _mm_load_ps(objectData + 16);
                __m128 boxMinimumW = _mm_load_ps(objectData + 24);

                //load aabb max
                __m128 boxMaximumX = _mm_load_ps(objectData + 4);
                __m128 boxMaximumY = _mm_load_ps(objectData + 12);
                __m128 boxMaximumZ = _mm_load_ps(objectData + 20);
                __m128 boxMaximumW = _mm_load_ps(objectData + 28);

                objectData += 32;
                //for now we have points in vectors boxMinimumX..w, but for calculations we need to xxxx yyyy zzzz vectors representation - just transpose data
                _MM_TRANSPOSE4_PS(boxMinimumX, boxMinimumY, boxMinimumZ, boxMinimumW);
                _MM_TRANSPOSE4_PS(boxMaximumX, boxMaximumY, boxMaximumZ, boxMaximumW);

                __m128 intersectionResult = _mm_setzero_ps();
                for (uint32_t plane = 0; plane < 6; plane++)
                {
                    //this code is similar to what we make in simple culling
                    //pick closest point to plane and check if it begind the plane. if yes - object outside frustum

                    //dot product, separate for each coordinate, for min & max aabb points
                    __m128 minimumPlaneX = _mm_mul_ps(boxMinimumX, frustumPlanesA[plane]);
                    __m128 minimumPlaneY = _mm_mul_ps(boxMinimumY, frustumPlanesB[plane]);
                    __m128 minimumPlaneZ = _mm_mul_ps(boxMinimumZ, frustumPlanesC[plane]);

                    __m128 maximumPlaneX = _mm_mul_ps(boxMaximumX, frustumPlanesA[plane]);
                    __m128 maximumPlaneY = _mm_mul_ps(boxMaximumY, frustumPlanesB[plane]);
                    __m128 maximumPlaneZ = _mm_mul_ps(boxMaximumZ, frustumPlanesC[plane]);

                    //we have 8 box points, but we need pick closest point to plane. Just take max
                    __m128 closestPointX = _mm_max_ps(minimumPlaneX, maximumPlaneX);
                    __m128 closestPointY = _mm_max_ps(minimumPlaneY, maximumPlaneY);
                    __m128 closestPointZ = _mm_max_ps(minimumPlaneZ, maximumPlaneZ);

                    //dist to plane = dot(aabb_point.xyz, plane.xyz) + plane.w
                    __m128 sumationXY = _mm_add_ps(closestPointX, closestPointY);
                    __m128 sumationZW = _mm_add_ps(closestPointZ, frustumPlanesD[plane]);
                    __m128 distanceToPlane = _mm_add_ps(sumationXY, sumationZW);

                    __m128 planeResult = _mm_cmple_ps(distanceToPlane, Zero); //dist from closest point to plane < 0 ?
                    intersectionResult = _mm_or_ps(intersectionResult, planeResult); //if yes - aabb behind the plane & outside frustum
                }

                //store result
                __m128i frustumResult = _mm_cvtps_epi32(intersectionResult);
                _mm_store_si128((__m128i *)&visibilityData[shape], frustumResult);
            }
        }

        void Frustum::cull(const std::vector<OrientedBox, AlignedAllocator<OrientedBox, 16>> &shapeList, std::vector<uint32_t, AlignedAllocator<uint32_t, 16>> &visibilityList)
        {
/*
            mat4_sse sse_camera_mat(cam_modelview_proj_mat);
            mat4_sse sse_clip_space_mat;

            //box points in local space
            __m128 obb_points_sse[8];
            obb_points_sse[0] = _mm_set_ps(1.f, box_min[2], box_max[1], box_min[0]);
            obb_points_sse[1] = _mm_set_ps(1.f, box_max[2], box_max[1], box_min[0]);
            obb_points_sse[2] = _mm_set_ps(1.f, box_max[2], box_max[1], box_max[0]);
            obb_points_sse[3] = _mm_set_ps(1.f, box_min[2], box_max[1], box_max[0]);
            obb_points_sse[4] = _mm_set_ps(1.f, box_min[2], box_min[1], box_max[0]);
            obb_points_sse[5] = _mm_set_ps(1.f, box_max[2], box_min[1], box_max[0]);
            obb_points_sse[6] = _mm_set_ps(1.f, box_max[2], box_min[1], box_min[0]);
            obb_points_sse[7] = _mm_set_ps(1.f, box_min[2], box_min[1], box_min[0]);

            ALIGN_SSE int obj_culling_res[4];

            __m128 Zero = _mm_setzero_ps();
            int shape, j;

            //process one object per step
            for (shape = firs_processing_object; shape < firs_processing_object + num_objects; shape++)
            {
                //clip space matrix = camera_view_proj * obj_mat
                sse_mat4_mul(sse_clip_space_mat, sse_camera_mat, sse_obj_mat[shape]);
                __m128 outside_positive_plane = _mm_set1_ps(0xffffffff);
                __m128 outside_negative_plane = _mm_set1_ps(0xffffffff);

                //for all 8 box points
                for (j = 0; j < 8; j++)
                {
                    //transform point to clip space
                    __m128 obb_transformed_point = sse_mat4_mul_vec4(sse_clip_space_mat, obb_points_sse[j]);

                    //gather w & -w
                    __m128 wwww = _mm_shuffle_ps(obb_transformed_point, obb_transformed_point, _MM_SHUFFLE(3, 3, 3, 3)); //get w
                    __m128 wwww_neg = _mm_sub_ps(Zero, wwww);  // negate all elements

                                                                 //box_point.xyz > box_point.w || box_point.xyz < -box_point.w ?
                                                                 //similar to point normalization: point.xyz /= point.w; And compare: point.xyz > 1 && point.xyz < -1
                    __m128 outside_pos_plane = _mm_cmpge_ps(obb_transformed_point, wwww);
                    __m128 outside_neg_plane = _mm_cmple_ps(obb_transformed_point, wwww_neg);

                    //if at least 1 of 8 points in front of the plane - we get 0 in outside_* flag
                    outside_positive_plane = _mm_and_ps(outside_positive_plane, outside_pos_plane);
                    outside_negative_plane = _mm_and_ps(outside_negative_plane, outside_neg_plane);
                }

                //all 8 points xyz < -1 or > 1 ?
                __m128 outside = _mm_or_ps(outside_positive_plane, outside_negative_plane);

                //store result
                __m128i outside_res_i = _mm_cvtps_epi32(outside);
                _mm_store_si128((__m128i *)&obj_culling_res[0], outside_res_i);

                //for now we have separate result separately for each axis
                //combine results. If outside any plane, then objects is outside frustum
                culling_res[shape] = (obj_culling_res[0] != 0 || obj_culling_res[1] != 0 || obj_culling_res[2] != 0) ? 1 : 0;
            }
*/
        }
    }; // namespace Shapes
}; // namespace Gek
