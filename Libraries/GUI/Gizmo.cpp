#include "GEK/GUI/Gizmo.hpp"

namespace Gek
{
    namespace UI
    {
        namespace Gizmo
        {
            const float screenRotateSize = 0.06f;

            static const float angleLimit = 0.96f;
            static const float planeLimit = 0.2f;

            static const Math::Float3 directionUnary[3] =
            {
                Math::Float3(1.0f, 0.0f, 0.0f),
                Math::Float3(0.0f, 1.0f, 0.0f),
                Math::Float3(0.0f, 0.0f, 1.0f)
            };

            static const ImU32 directionColor[3] =
            {
                0xFF0000AA,
                0xFF00AA00,
                0xFFAA0000
            };

            static const ImU32 selectionColor = 0xFF1080FF;
            static const ImU32 inactiveColor = 0x99999999;
            static const ImU32 translationLineColor = 0xAAAAAAAA;
            static const char *translationInfoMask[] =
            {
                "X : %5.3f",
                "Y : %5.3f",
                "Z : %5.3f",
                "X : %5.3f Y : %5.3f",
                "Y : %5.3f Z : %5.3f",
                "X : %5.3f Z : %5.3f",
                "X : %5.3f Y : %5.3f Z : %5.3f"
            };

            static const char *scaleInfoMask[] =
            {
                "X : %5.2f", 
                "Y : %5.2f", 
                "Z : %5.2f", "XYZ : %5.2f"
            };

            static const char *rotationInfoMask[] =
            {
                "X : %5.2f deg %5.2f rad",
                "Y : %5.2f deg %5.2f rad",
                "Z : %5.2f deg %5.2f rad",
                "Screen : %5.2f deg %5.2f rad"
            };

            static const int translationInfoIndex[] = 
            {
                0,0,0,
                1,0,0,
                2,0,0,
                0,1,0,
                1,2,0,
                0,2,1,
                0,1,2
            };

            static const float quadMin = 0.5f;
            static const float quadMax = 0.8f;
            static const float quadUV[8] =
            {
                quadMin, quadMin,
                quadMin, quadMax,
                quadMax, quadMax,
                quadMax, quadMin
            };

            static const int halfCircleSegmentCount = 64;
            static const float snapTension = 0.5f;

            struct Context
            {
                enum Movement
                {
                    NONE,
                    MOVE_X,
                    MOVE_Y,
                    MOVE_Z,
                    MOVE_XY,
                    MOVE_XZ,
                    MOVE_YZ,
                    MOVE_SCREEN,
                    ROTATE_X,
                    ROTATE_Y,
                    ROTATE_Z,
                    ROTATE_SCREEN,
                    SCALE_X,
                    SCALE_Y,
                    SCALE_Z,
                    SCALE_XYZ,
                    BOUNDS,
                };

                ImDrawList* mDrawList = nullptr;

                int mMode = Alignment::World;
                Math::Float4x4 mViewMat;
                Math::Float4x4 mProjectionMat;
                Math::Float4x4 mModel;
                Math::Float4x4 mModelInverse;
                Math::Float4x4 mModelSource;
                Math::Float4x4 mModelSourceInverse;
                Math::Float4x4 mMVP;
                Math::Float4x4 mViewProjection;

                Math::Float3 mModelScaleOrigin;
                Math::Float3 mCameraEye;
                Math::Float3 mCameraRight;
                Math::Float3 mCameraDir;
                Math::Float3 mCameraUp;
                Math::Float3 mRayOrigin;
                Math::Float3 mRayVector;

                ImVec2 mScreenSquareCenter;
                ImVec2 mScreenSquareMin;
                ImVec2 mScreenSquareMax;

                float mScreenFactor;
                Math::Float3 mRelativeOrigin;

                bool mbUsing = false;
                bool mbEnable = true;

                // translation
                Math::Float4 mTranslationPlane;
                Math::Float3 mTranslationPlaneOrigin;
                Math::Float3 mMatrixOrigin;

                // rotation
                Math::Float3 mRotationVectorSource;
                float mRotationAngle;
                float mRotationAngleOrigin;

                // scale
                Math::Float3 mScale;
                Math::Float3 mScaleValueOrigin;
                float mSaveMousePosx;

                // save axis factor when using gizmo
                bool mBelowAxisLimit[3];
                bool mBelowPlaneLimit[3];
                float mAxisFactor[3];

                // bounds stretching
                Math::Float4 mBoundsPivot;
                Math::Float4 mBoundsAnchor;
                Math::Float4 mBoundsPlan;
                Math::Float4 mBoundsLocalPivot;
                int mBoundsBestAxis;
                int mBoundsAxis[2];
                bool mbUsingBounds = false;
                Math::Float4x4 mBoundsMatrix;

                //
                int mCurrentOperation = Operation::None;

                float mX = 0.0f;
                float mY = 0.0f;
                float mWidth = 0.0f;
                float mHeight = 0.0f;

                ImVec2 worldToPos(Math::Float3 const &worldPos, const Math::Float4x4& mat)
                {
                    ImGuiIO& io = ImGui::GetIO();

                    Math::Float4 trans = mat.transform(Math::Float4(worldPos, 1.0f));
                    trans.xy *= 0.5f / trans.w;
                    trans.xy += Math::Float2(0.5f, 0.5f);
                    trans.y = 1.0f - trans.y;
                    trans.x *= mWidth;
                    trans.y *= mHeight;
                    trans.x += mX;
                    trans.y += mY;
                    return ImVec2(trans.x, trans.y);
                }

                void ComputeCameraRay(Math::Float3 &rayOrigin, Math::Float3 &rayDir)
                {
                    ImGuiIO& io = ImGui::GetIO();

                    Math::Float4x4 mViewProjInverse;
                    mViewProjInverse = (mViewMat * mProjectionMat).getInverse();

                    float mox = ((io.MousePos.x - mX) / mWidth) * 2.0f - 1.0f;
                    float moy = (1.0f - ((io.MousePos.y - mY) / mHeight)) * 2.0f - 1.0f;

                    auto rayProjected = mViewProjInverse.transform(Math::Float4(mox, moy, 0.0f, 1.0f));
                    rayOrigin = rayProjected.xyz;
                    rayOrigin *= 1.0f / rayProjected.w;
                    Math::Float4 rayEnd;
                    rayEnd = mViewProjInverse.transform(Math::Float4(mox, moy, 1.0f, 1.0f));
                    rayEnd.xyz *= 1.0f / rayEnd.w;
                    rayDir = (rayEnd.xyz - rayOrigin).getNormal();
                }

                float IntersectRayPlane(Math::Float3 const &rOrigin, Math::Float3 const &rVector, Math::Float4 const &plane)
                {
                    float numer = plane.xyz.dot(rOrigin) - plane.w;
                    float denom = plane.xyz.dot(rVector);

                    if (std::abs(denom) < FLT_EPSILON)  // normal is orthogonal to vector, cant intersect
                    {
                        return -1.0f;
                    }

                    return -(numer / denom);
                }

                void SetRect(float x, float y, float width, float height)
                {
                    mX = x;
                    mY = y;
                    mWidth = width;
                    mHeight = height;
                }

                void BeginFrame()
                {
                    mDrawList = ImGui::GetWindowDrawList();
                    if (!mDrawList)
                    {
                        ImGuiIO& io = ImGui::GetIO();
                        ImGui::Begin("gizmo", NULL, io.DisplaySize, 0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus);
                        mDrawList = ImGui::GetWindowDrawList();
                        ImGui::End();
                    }
                }

                bool IsUsing()
                {
                    return mbUsing || mbUsingBounds;
                }

                bool IsOver()
                {
                    return (GetMoveType(NULL) != NONE) || GetRotateType() != NONE || GetScaleType() != NONE || IsUsing();
                }

                void Enable(bool enable)
                {
                    mbEnable = enable;
                    if (!enable)
                    {
                        mbUsing = false;
                        mbUsingBounds = false;
                    }
                }

                float GetUniform(const Math::Float3 &position, const Math::Float4x4& mat)
                {
                    return mat.transform(Math::Float4(position, 1.0f)).w;
                }

                void ComputeContext(const float *view, const float *projection, float *matrix, int mode)
                {
                    mMode = mode;
                    mViewMat = *(Math::Float4x4*)view;
                    mProjectionMat = *(Math::Float4x4*)projection;

                    if (mode == Alignment::Local)
                    {
                        mModel = *(Math::Float4x4*)matrix;
                        mModel.OrthoNormalize();
                    }
                    else
                    {
                        mModel.translation = ((Math::Float4x4*)matrix)->translation;
                    }

                    mModelSource = *(Math::Float4x4*)matrix;
                    mModelScaleOrigin.xyz = mModelSource.getScaling();

                    mModelInverse = mModel.getInverse();
                    mModelSourceInverse = mModelSource.getInverse();
                    mViewProjection = mViewMat * mProjectionMat;
                    mMVP = mModel * mViewProjection;

                    Math::Float4x4 viewInverse;
                    viewInverse = mViewMat.getInverse();
                    mCameraDir = viewInverse.rz.xyz;
                    mCameraEye = viewInverse.translation.xyz;
                    mCameraRight = viewInverse.rx.xyz;
                    mCameraUp = viewInverse.ry.xyz;
                    mScreenFactor = 0.1f * GetUniform(mModel.translation.xyz, mViewProjection);

                    ImVec2 centerSSpace = worldToPos(Math::Float3::Zero, mMVP);
                    mScreenSquareCenter = centerSSpace;
                    mScreenSquareMin = ImVec2(centerSSpace.x - 10.0f, centerSSpace.y - 10.0f);
                    mScreenSquareMax = ImVec2(centerSSpace.x + 10.0f, centerSSpace.y + 10.0f);

                    ComputeCameraRay(mRayOrigin, mRayVector);
                }

                void ComputeColors(ImU32 *colors, int type, int operation)
                {
                    if (mbEnable)
                    {
                        switch (operation)
                        {
                        case Operation::Translate:
                            colors[0] = (type == MOVE_SCREEN) ? selectionColor : 0xFFFFFFFF;
                            for (int index = 0; index < 3; index++)
                            {
                                int colorPlaneIndex = (index + 2) % 3;
                                colors[index + 1] = (type == (int)(MOVE_X + index)) ? selectionColor : directionColor[index];
                                colors[index + 4] = (type == (int)(MOVE_XY + index)) ? selectionColor : directionColor[colorPlaneIndex];
                            }

                            break;

                        case Operation::Rotate:
                            colors[0] = (type == ROTATE_SCREEN) ? selectionColor : 0xFFFFFFFF;
                            for (int index = 0; index < 3; index++)
                            {
                                colors[index + 1] = (type == (int)(ROTATE_X + index)) ? selectionColor : directionColor[index];
                            }

                            break;

                        case Operation::Scale:
                            colors[0] = (type == SCALE_XYZ) ? selectionColor : 0xFFFFFFFF;
                            for (int index = 0; index < 3; index++)
                            {
                                colors[index + 1] = (type == (int)(SCALE_X + index)) ? selectionColor : directionColor[index];
                            }

                            break;

                        case Operation::Bounds:
                            break;
                        }
                    }
                    else
                    {
                        for (int index = 0; index < 7; index++)
                        {
                            colors[index] = inactiveColor;
                        }
                    }
                }

                void ComputeTripodAxisAndVisibility(int axisIndex, Math::Float3 &dirPlaneX, Math::Float3 &dirPlaneY, bool& belowAxisLimit, bool& belowPlaneLimit)
                {
                    const int planNormal = (axisIndex + 2) % 3;
                    dirPlaneX = directionUnary[axisIndex];
                    dirPlaneY = directionUnary[(axisIndex + 1) % 3];

                    if (mbUsing)
                    {
                        // when using, use stored factors so the gizmo doesn't flip when we translate
                        belowAxisLimit = mBelowAxisLimit[axisIndex];
                        belowPlaneLimit = mBelowPlaneLimit[axisIndex];

                        dirPlaneX *= mAxisFactor[axisIndex];
                        dirPlaneY *= mAxisFactor[(axisIndex + 1) % 3];
                    }
                    else
                    {
                        Math::Float3 dirPlaneNormalWorld = mModel.transform(directionUnary[planNormal]);
                        dirPlaneNormalWorld.normalize();

                        Math::Float3 dirPlaneXWorld = mModel.transform(dirPlaneX);
                        dirPlaneXWorld.normalize();

                        Math::Float3 dirPlaneYWorld = mModel.transform(dirPlaneY);
                        dirPlaneYWorld.normalize();

                        Math::Float3 cameraEyeToGizmo = (mModel.translation.xyz - mCameraEye).getNormal();
                        float dotCameraDirX = cameraEyeToGizmo.dot(dirPlaneXWorld);
                        float dotCameraDirY = cameraEyeToGizmo.dot(dirPlaneYWorld);

                        // compute factor values
                        float mulAxisX = (dotCameraDirX > 0.0f) ? -1.0f : 1.0f;
                        float mulAxisY = (dotCameraDirY > 0.0f) ? -1.0f : 1.0f;
                        dirPlaneX *= mulAxisX;
                        dirPlaneY *= mulAxisY;

                        belowAxisLimit = std::abs(dotCameraDirX) < angleLimit;
                        belowPlaneLimit = (std::abs(cameraEyeToGizmo.dot(dirPlaneNormalWorld)) > planeLimit);

                        // and store values
                        mAxisFactor[axisIndex] = mulAxisX;
                        mAxisFactor[(axisIndex + 1) % 3] = mulAxisY;
                        mBelowAxisLimit[axisIndex] = belowAxisLimit;
                        mBelowPlaneLimit[axisIndex] = belowPlaneLimit;
                    }
                }

                void ComputeSnap(float *value, float snap)
                {
                    if (snap <= FLT_EPSILON)
                    {
                        return;
                    }

                    float modulo = fmodf(*value, snap);
                    float moduloRatio = std::abs(modulo) / snap;
                    if (moduloRatio < snapTension)
                    {
                        *value -= modulo;
                    }
                    else if (moduloRatio > (1.0f - snapTension))
                    {
                        *value = *value - modulo + snap * ((*value < 0.0f) ? -1.0f : 1.0f);
                    }
                }
                void ComputeSnap(Math::Float3 &value, float *snap)
                {
                    for (int index = 0; index < 3; index++)
                    {
                        ComputeSnap(&value[index], snap[index]);
                    }
                }

                float ComputeAngleOnPlan()
                {
                    const float len = IntersectRayPlane(mRayOrigin, mRayVector, mTranslationPlane);
                    Math::Float3 localPos = (mRayOrigin + mRayVector * len - mModel.translation.xyz).getNormal();

                    Math::Float3 perpendicularVector = mRotationVectorSource.cross(mTranslationPlane.xyz);
                    perpendicularVector.normalize();
                    float acosAngle = std::clamp(localPos.dot(mRotationVectorSource), -0.9999f, 0.9999f);
                    float angle = acosf(acosAngle);
                    angle *= (localPos.dot(perpendicularVector) < 0.0f) ? 1.0f : -1.0f;
                    return angle;
                }

                void DrawRotationGizmo(int type)
                {
                    ImDrawList* drawList = mDrawList;
                    ImGuiIO& io = ImGui::GetIO();

                    // colors
                    ImU32 colors[7];
                    ComputeColors(colors, type, Operation::Rotate);

                    Math::Float3 cameraToModelNormalized = (mModel.translation.xyz - mCameraEye).getNormal();
                    cameraToModelNormalized = mModelInverse.transform(cameraToModelNormalized);

                    for (int axis = 0; axis < 3; axis++)
                    {
                        ImVec2 circlePos[halfCircleSegmentCount];

                        float angleStart = std::atan2(cameraToModelNormalized[(4 - axis) % 3], cameraToModelNormalized[(3 - axis) % 3]) + Math::Pi * 0.5f;

                        for (unsigned int index = 0; index < halfCircleSegmentCount; index++)
                        {
                            float ng = angleStart + Math::Pi * ((float)index / (float)halfCircleSegmentCount);
                            Math::Float3 axisPos = Math::Float3(cosf(ng), sinf(ng), 0.0f);
                            Math::Float3 pos = Math::Float3(axisPos[axis], axisPos[(axis + 1) % 3], axisPos[(axis + 2) % 3]) * mScreenFactor;
                            circlePos[index] = worldToPos(pos, mMVP);
                        }
                        drawList->AddPolyline(circlePos, halfCircleSegmentCount, colors[3 - axis], false, 2, true);
                    }
                    drawList->AddCircle(worldToPos(mModel.translation, mViewProjection), screenRotateSize * mHeight, colors[0], 64);

                    if (mbUsing)
                    {
                        ImVec2 circlePos[halfCircleSegmentCount + 1];

                        circlePos[0] = worldToPos(mModel.translation, mViewProjection);
                        for (unsigned int index = 1; index < halfCircleSegmentCount; index++)
                        {
                            float ng = mRotationAngle * ((float)(index - 1) / (float)(halfCircleSegmentCount - 1));
                            Math::Float4x4 rotateVectorMatrix;
                            rotateVectorMatrix.RotationAxis(mTranslationPlane, ng);
                            Math::Float4 pos;
                            pos.TransformPoint(mRotationVectorSource, rotateVectorMatrix);
                            pos *= mScreenFactor;
                            circlePos[index] = worldToPos(pos + mModel.translation, mViewProjection);
                        }
                        drawList->AddConvexPolyFilled(circlePos, halfCircleSegmentCount, 0x801080FF, true);
                        drawList->AddPolyline(circlePos, halfCircleSegmentCount, 0xFF1080FF, true, 2, true);

                        ImVec2 destinationPosOnScreen = circlePos[1];
                        char tmps[512];
                        ImFormatString(tmps, sizeof(tmps), rotationInfoMask[type - ROTATE_X], (mRotationAngle / Math::Pi)*180.0f, mRotationAngle);
                        drawList->AddText(ImVec2(destinationPosOnScreen.x + 15, destinationPosOnScreen.y + 15), 0xFF000000, tmps);
                        drawList->AddText(ImVec2(destinationPosOnScreen.x + 14, destinationPosOnScreen.y + 14), 0xFFFFFFFF, tmps);
                    }
                }

                void DrawHatchedAxis(Math::Float3 const &axis)
                {
                    for (int j = 1; j < 10; j++)
                    {
                        ImVec2 baseSSpace2 = worldToPos(axis * 0.05f * (float)(j * 2) * mScreenFactor, mMVP);
                        ImVec2 worldDirSSpace2 = worldToPos(axis * 0.05f * (float)(j * 2 + 1) * mScreenFactor, mMVP);
                        mDrawList->AddLine(baseSSpace2, worldDirSSpace2, 0x80000000, 6.0f);
                    }
                }

                void DrawScaleGizmo(int type)
                {
                    ImDrawList* drawList = mDrawList;

                    // colors
                    ImU32 colors[7];
                    ComputeColors(colors, type, Operation::Scale);

                    // draw screen cirle
                    drawList->AddCircleFilled(mScreenSquareCenter, 12.0f, colors[0], 32);

                    // draw
                    Math::Float4 scaleDisplay = { 1.0f, 1.0f, 1.0f, 1.0f };

                    if (mbUsing)
                        scaleDisplay = mScale;

                    for (unsigned int index = 0; index < 3; index++)
                    {
                        Math::Float4 dirPlaneX, dirPlaneY;
                        bool belowAxisLimit, belowPlaneLimit;
                        ComputeTripodAxisAndVisibility(index, dirPlaneX, dirPlaneY, belowAxisLimit, belowPlaneLimit);

                        // draw axis
                        if (belowAxisLimit)
                        {
                            ImVec2 baseSSpace = worldToPos(dirPlaneX * 0.1f * mScreenFactor, mMVP);
                            ImVec2 worldDirSSpaceNoScale = worldToPos(dirPlaneX * mScreenFactor, mMVP);
                            ImVec2 worldDirSSpace = worldToPos((dirPlaneX * scaleDisplay[index]) * mScreenFactor, mMVP);

                            if (mbUsing)
                            {
                                drawList->AddLine(baseSSpace, worldDirSSpaceNoScale, 0xFF404040, 6.0f);
                                drawList->AddCircleFilled(worldDirSSpaceNoScale, 10.0f, 0xFF404040);
                            }

                            drawList->AddLine(baseSSpace, worldDirSSpace, colors[index + 1], 6.0f);
                            drawList->AddCircleFilled(worldDirSSpace, 10.0f, colors[index + 1]);

                            if (mAxisFactor[index] < 0.0f)
                                DrawHatchedAxis(dirPlaneX * scaleDisplay[index]);
                        }
                    }

                    if (mbUsing)
                    {
                        //ImVec2 sourcePosOnScreen = worldToPos(mMatrixOrigin, mViewProjection);
                        ImVec2 destinationPosOnScreen = worldToPos(mModel.translation, mViewProjection);
                        /*Math::Float4 dif(destinationPosOnScreen.x - sourcePosOnScreen.x, destinationPosOnScreen.y - sourcePosOnScreen.y);
                        dif.normalize();
                        dif *= 5.0f;
                        drawList->AddCircle(sourcePosOnScreen, 6.0f, translationLineColor);
                        drawList->AddCircle(destinationPosOnScreen, 6.0f, translationLineColor);
                        drawList->AddLine(ImVec2(sourcePosOnScreen.x + dif.x, sourcePosOnScreen.y + dif.y), ImVec2(destinationPosOnScreen.x - dif.x, destinationPosOnScreen.y - dif.y), translationLineColor, 2.0f);
                        */
                        char tmps[512];
                        //Math::Float4 deltaInfo = mModel.translation - mMatrixOrigin;
                        int componentInfoIndex = (type - SCALE_X) * 3;
                        ImFormatString(tmps, sizeof(tmps), scaleInfoMask[type - SCALE_X], scaleDisplay[translationInfoIndex[componentInfoIndex]]);
                        drawList->AddText(ImVec2(destinationPosOnScreen.x + 15, destinationPosOnScreen.y + 15), 0xFF000000, tmps);
                        drawList->AddText(ImVec2(destinationPosOnScreen.x + 14, destinationPosOnScreen.y + 14), 0xFFFFFFFF, tmps);
                    }
                }


                void DrawTranslationGizmo(int type)
                {
                    ImDrawList* drawList = mDrawList;
                    if (!drawList)
                        return;

                    // colors
                    ImU32 colors[7];
                    ComputeColors(colors, type, Operation::Translate);

                    // draw screen quad
                    drawList->AddRectFilled(mScreenSquareMin, mScreenSquareMax, colors[0], 2.0f);

                    // draw
                    for (unsigned int index = 0; index < 3; index++)
                    {
                        Math::Float4 dirPlaneX, dirPlaneY;
                        bool belowAxisLimit, belowPlaneLimit;
                        ComputeTripodAxisAndVisibility(index, dirPlaneX, dirPlaneY, belowAxisLimit, belowPlaneLimit);

                        // draw axis
                        if (belowAxisLimit)
                        {
                            ImVec2 baseSSpace = worldToPos(dirPlaneX * 0.1f * mScreenFactor, mMVP);
                            ImVec2 worldDirSSpace = worldToPos(dirPlaneX * mScreenFactor, mMVP);

                            drawList->AddLine(baseSSpace, worldDirSSpace, colors[index + 1], 6.0f);

                            if (mAxisFactor[index] < 0.0f)
                                DrawHatchedAxis(dirPlaneX);
                        }

                        // draw plane
                        if (belowPlaneLimit)
                        {
                            ImVec2 screenQuadPts[4];
                            for (int j = 0; j < 4; j++)
                            {
                                Math::Float4 cornerWorldPos = (dirPlaneX * quadUV[j * 2] + dirPlaneY  * quadUV[j * 2 + 1]) * mScreenFactor;
                                screenQuadPts[j] = worldToPos(cornerWorldPos, mMVP);
                            }
                            drawList->AddConvexPolyFilled(screenQuadPts, 4, colors[index + 4], true);
                        }
                    }

                    if (mbUsing)
                    {
                        ImVec2 sourcePosOnScreen = worldToPos(mMatrixOrigin, mViewProjection);
                        ImVec2 destinationPosOnScreen = worldToPos(mModel.translation, mViewProjection);
                        Math::Float4 dif = { destinationPosOnScreen.x - sourcePosOnScreen.x, destinationPosOnScreen.y - sourcePosOnScreen.y, 0.0f, 0.0f };
                        dif.normalize();
                        dif *= 5.0f;
                        drawList->AddCircle(sourcePosOnScreen, 6.0f, translationLineColor);
                        drawList->AddCircle(destinationPosOnScreen, 6.0f, translationLineColor);
                        drawList->AddLine(ImVec2(sourcePosOnScreen.x + dif.x, sourcePosOnScreen.y + dif.y), ImVec2(destinationPosOnScreen.x - dif.x, destinationPosOnScreen.y - dif.y), translationLineColor, 2.0f);

                        char tmps[512];
                        Math::Float4 deltaInfo = mModel.translation - mMatrixOrigin;
                        int componentInfoIndex = (type - MOVE_X) * 3;
                        ImFormatString(tmps, sizeof(tmps), translationInfoMask[type - MOVE_X], deltaInfo[translationInfoIndex[componentInfoIndex]], deltaInfo[translationInfoIndex[componentInfoIndex + 1]], deltaInfo[translationInfoIndex[componentInfoIndex + 2]]);
                        drawList->AddText(ImVec2(destinationPosOnScreen.x + 15, destinationPosOnScreen.y + 15), 0xFF000000, tmps);
                        drawList->AddText(ImVec2(destinationPosOnScreen.x + 14, destinationPosOnScreen.y + 14), 0xFFFFFFFF, tmps);
                    }
                }

                void DrawBounds(float *bounds, Math::Float4x4 *matrix, float *snapValues)
                {
                    ImGuiIO& io = ImGui::GetIO();
                    ImDrawList* drawList = mDrawList;

                    // compute best projection axis
                    Math::Float4 bestAxisWorldDirection;
                    int bestAxis = mBoundsBestAxis;
                    if (!mbUsingBounds)
                    {

                        float bestDot = 0.0f;
                        for (unsigned int index = 0; index < 3; index++)
                        {
                            Math::Float4 dirPlaneNormalWorld;
                            dirPlaneNormalWorld.TransformVector(directionUnary[index], mModelSource);
                            dirPlaneNormalWorld.normalize();

                            float dt = Dot(Normalized(mCameraEye - mModelSource.translation), dirPlaneNormalWorld);
                            if (std::abs(dt) >= bestDot)
                            {
                                bestDot = std::abs(dt);
                                bestAxis = index;
                                bestAxisWorldDirection = dirPlaneNormalWorld;
                            }
                        }
                    }

                    // corners
                    Math::Float4 aabb[4];

                    int secondAxis = (bestAxis + 1) % 3;
                    int thirdAxis = (bestAxis + 2) % 3;

                    for (int index = 0; index < 4; index++)
                    {
                        aabb[index][3] = aabb[index][bestAxis] = 0.0f;
                        aabb[index][secondAxis] = bounds[secondAxis + 3 * (index >> 1)];
                        aabb[index][thirdAxis] = bounds[thirdAxis + 3 * ((index >> 1) ^ (index & 1))];
                    }

                    // draw bounds
                    unsigned int anchorAlpha = mbEnable ? 0xFF000000 : 0x80000000;

                    Math::Float4x4 boundsMVP = mModelSource * mViewProjection;
                    for (int index = 0; index < 4; index++)
                    {
                        ImVec2 worldBound1 = worldToPos(aabb[index], boundsMVP);
                        ImVec2 worldBound2 = worldToPos(aabb[(index + 1) % 4], boundsMVP);
                        float boundDistance = sqrtf(ImLengthSqr(worldBound1 - worldBound2));
                        int stepCount = (int)(boundDistance / 10.0f);
                        float stepLength = 1.0f / (float)stepCount;
                        for (int j = 0; j < stepCount; j++)
                        {
                            float t1 = (float)j * stepLength;
                            float t2 = (float)j * stepLength + stepLength * 0.5f;
                            ImVec2 worldBoundSS1 = ImLerp(worldBound1, worldBound2, ImVec2(t1, t1));
                            ImVec2 worldBoundSS2 = ImLerp(worldBound1, worldBound2, ImVec2(t2, t2));
                            drawList->AddLine(worldBoundSS1, worldBoundSS2, 0xAAAAAA + anchorAlpha, 3.0f);
                        }
                        Math::Float4 midPoint = (aabb[index] + aabb[(index + 1) % 4]) * 0.5f;
                        ImVec2 midBound = worldToPos(midPoint, boundsMVP);
                        static const float AnchorBigRadius = 8.0f;
                        static const float AnchorSmallRadius = 6.0f;
                        bool overBigAnchor = ImLengthSqr(worldBound1 - io.MousePos) <= (AnchorBigRadius*AnchorBigRadius);
                        bool overSmallAnchor = ImLengthSqr(midBound - io.MousePos) <= (AnchorBigRadius*AnchorBigRadius);


                        unsigned int bigAnchorColor = overBigAnchor ? selectionColor : (0xAAAAAA + anchorAlpha);
                        unsigned int smallAnchorColor = overSmallAnchor ? selectionColor : (0xAAAAAA + anchorAlpha);

                        drawList->AddCircleFilled(worldBound1, AnchorBigRadius, bigAnchorColor);
                        drawList->AddCircleFilled(midBound, AnchorSmallRadius, smallAnchorColor);
                        int oppositeIndex = (index + 2) % 4;
                        // big anchor on corners
                        if (!mbUsingBounds && mbEnable && overBigAnchor && io.MouseDown[0])
                        {
                            mBoundsPivot.TransformPoint(aabb[(index + 2) % 4], mModelSource);
                            mBoundsAnchor.TransformPoint(aabb[index], mModelSource);
                            mBoundsPlan = BuildPlane(mBoundsAnchor, bestAxisWorldDirection);
                            mBoundsBestAxis = bestAxis;
                            mBoundsAxis[0] = secondAxis;
                            mBoundsAxis[1] = thirdAxis;

                            mBoundsLocalPivot.Set(0.0f);
                            mBoundsLocalPivot[secondAxis] = aabb[oppositeIndex][secondAxis];
                            mBoundsLocalPivot[thirdAxis] = aabb[oppositeIndex][thirdAxis];

                            mbUsingBounds = true;
                            mBoundsMatrix = mModelSource;
                        }
                        // small anchor on middle of segment
                        if (!mbUsingBounds && mbEnable && overSmallAnchor && io.MouseDown[0])
                        {
                            Math::Float4 midPointOpposite = (aabb[(index + 2) % 4] + aabb[(index + 3) % 4]) * 0.5f;
                            mBoundsPivot.TransformPoint(midPointOpposite, mModelSource);
                            mBoundsAnchor.TransformPoint(midPoint, mModelSource);
                            mBoundsPlan = BuildPlane(mBoundsAnchor, bestAxisWorldDirection);
                            mBoundsBestAxis = bestAxis;
                            int indices[] = { secondAxis , thirdAxis };
                            mBoundsAxis[0] = indices[index % 2];
                            mBoundsAxis[1] = -1;

                            mBoundsLocalPivot.Set(0.0f);
                            mBoundsLocalPivot[mBoundsAxis[0]] = aabb[oppositeIndex][indices[index % 2]];// bounds[mBoundsAxis[0]] * (((index + 1) & 2) ? 1.0f : -1.0f);

                            mbUsingBounds = true;
                            mBoundsMatrix = mModelSource;
                        }
                    }

                    if (mbUsingBounds)
                    {
                        Math::Float4x4 scale;
                        scale.SetToIdentity();

                        // compute projected mouse position on plane
                        const float len = IntersectRayPlane(mRayOrigin, mRayVector, mBoundsPlan);
                        Math::Float4 newPos = mRayOrigin + mRayVector * len;

                        // compute a reference and delta vectors base on mouse move
                        Math::Float4 deltaVector = (newPos - mBoundsPivot).Abs();
                        Math::Float4 referenceVector = (mBoundsAnchor - mBoundsPivot).Abs();

                        // for 1 or 2 axes, compute a ratio that's used for scale and snap it based on resulting length
                        for (int index = 0; index < 2; index++)
                        {
                            int axisIndex = mBoundsAxis[index];
                            if (axisIndex == -1)
                                continue;

                            float ratioAxis = 1.0f;
                            Math::Float4 axisDir = mBoundsMatrix.component[axisIndex].Abs();

                            float dtAxis = axisDir.Dot(referenceVector);
                            float boundSize = bounds[axisIndex + 3] - bounds[axisIndex];
                            if (dtAxis > FLT_EPSILON)
                                ratioAxis = axisDir.Dot(deltaVector) / dtAxis;

                            if (snapValues)
                            {
                                float length = boundSize * ratioAxis;
                                ComputeSnap(&length, snapValues[axisIndex]);
                                if (boundSize > FLT_EPSILON)
                                    ratioAxis = length / boundSize;
                            }
                            scale.component[axisIndex] *= ratioAxis;
                        }

                        // transform matrix
                        Math::Float4x4 preScale, postScale;
                        preScale.Translation(-mBoundsLocalPivot);
                        postScale.Translation(mBoundsLocalPivot);
                        Math::Float4x4 res = preScale * scale * postScale * mBoundsMatrix;
                        *matrix = res;

                        // info text
                        char tmps[512];
                        ImVec2 destinationPosOnScreen = worldToPos(mModel.translation, mViewProjection);
                        ImFormatString(tmps, sizeof(tmps), "X: %.2f Y: %.2f Z:%.2f"
                            , (bounds[3] - bounds[0]) * mBoundsMatrix.component[0].Length() * scale.component[0].Length()
                            , (bounds[4] - bounds[1]) * mBoundsMatrix.component[1].Length() * scale.component[1].Length()
                            , (bounds[5] - bounds[2]) * mBoundsMatrix.component[2].Length() * scale.component[2].Length()
                        );
                        drawList->AddText(ImVec2(destinationPosOnScreen.x + 15, destinationPosOnScreen.y + 15), 0xFF000000, tmps);
                        drawList->AddText(ImVec2(destinationPosOnScreen.x + 14, destinationPosOnScreen.y + 14), 0xFFFFFFFF, tmps);
                    }

                    if (!io.MouseDown[0])
                        mbUsingBounds = false;
                }

                ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
                // 

                int GetScaleType()
                {
                    ImGuiIO& io = ImGui::GetIO();
                    int type = NONE;

                    // screen
                    if (io.MousePos.x >= mScreenSquareMin.x && io.MousePos.x <= mScreenSquareMax.x &&
                        io.MousePos.y >= mScreenSquareMin.y && io.MousePos.y <= mScreenSquareMax.y)
                        type = SCALE_XYZ;

                    const Math::Float4 direction[3] = { mModel.v.right, mModel.v.up, mModel.v.dir };
                    // compute
                    for (unsigned int index = 0; index < 3 && type == NONE; index++)
                    {
                        Math::Float4 dirPlaneX, dirPlaneY;
                        bool belowAxisLimit, belowPlaneLimit;
                        ComputeTripodAxisAndVisibility(index, dirPlaneX, dirPlaneY, belowAxisLimit, belowPlaneLimit);
                        dirPlaneX.TransformVector(mModel);
                        dirPlaneY.TransformVector(mModel);

                        const int planNormal = (index + 2) % 3;

                        const float len = IntersectRayPlane(mRayOrigin, mRayVector, BuildPlane(mModel.translation, direction[planNormal]));
                        Math::Float4 posOnPlan = mRayOrigin + mRayVector * len;

                        const float dx = dirPlaneX.xyz.dot((posOnPlan - mModel.translation) * (1.0f / mScreenFactor));
                        const float dy = dirPlaneY.xyz.dot((posOnPlan - mModel.translation) * (1.0f / mScreenFactor));
                        if (belowAxisLimit && dy > -0.1f && dy < 0.1f && dx > 0.1f  && dx < 1.0f)
                            type = SCALE_X + index;
                    }
                    return type;
                }

                int GetRotateType()
                {
                    ImGuiIO& io = ImGui::GetIO();
                    int type = NONE;

                    Math::Float4 deltaScreen = { io.MousePos.x - mScreenSquareCenter.x, io.MousePos.y - mScreenSquareCenter.y, 0.0f, 0.0f };
                    float dist = deltaScreen.Length();
                    if (dist >= (screenRotateSize - 0.002f) * mHeight && dist < (screenRotateSize + 0.002f) * mHeight)
                        type = ROTATE_SCREEN;

                    const Math::Float4 planNormals[] = { mModel.v.right, mModel.v.up, mModel.v.dir };

                    for (unsigned int index = 0; index < 3 && type == NONE; index++)
                    {
                        // pickup plane
                        Math::Float4 pickupPlan = BuildPlane(mModel.translation, planNormals[index]);

                        const float len = IntersectRayPlane(mRayOrigin, mRayVector, pickupPlan);
                        Math::Float4 localPos = mRayOrigin + mRayVector * len - mModel.translation;

                        if (Dot(Normalized(localPos), mRayVector) > FLT_EPSILON)
                            continue;

                        float distance = localPos.Length() / mScreenFactor;
                        if (distance > 0.9f && distance < 1.1f)
                            type = ROTATE_X + index;
                    }

                    return type;
                }

                int GetMoveType(Math::Float4 *gizmoHitProportion)
                {
                    ImGuiIO& io = ImGui::GetIO();
                    int type = NONE;

                    // screen
                    if (io.MousePos.x >= mScreenSquareMin.x && io.MousePos.x <= mScreenSquareMax.x &&
                        io.MousePos.y >= mScreenSquareMin.y && io.MousePos.y <= mScreenSquareMax.y)
                        type = MOVE_SCREEN;

                    const Math::Float4 direction[3] = { mModel.v.right, mModel.v.up, mModel.v.dir };

                    // compute
                    for (unsigned int index = 0; index < 3 && type == NONE; index++)
                    {
                        Math::Float4 dirPlaneX, dirPlaneY;
                        bool belowAxisLimit, belowPlaneLimit;
                        ComputeTripodAxisAndVisibility(index, dirPlaneX, dirPlaneY, belowAxisLimit, belowPlaneLimit);
                        dirPlaneX.TransformVector(mModel);
                        dirPlaneY.TransformVector(mModel);

                        const int planNormal = (index + 2) % 3;

                        const float len = IntersectRayPlane(mRayOrigin, mRayVector, BuildPlane(mModel.translation, direction[planNormal]));
                        Math::Float4 posOnPlan = mRayOrigin + mRayVector * len;

                        const float dx = dirPlaneX.xyz.dot((posOnPlan - mModel.translation) * (1.0f / mScreenFactor));
                        const float dy = dirPlaneY.xyz.dot((posOnPlan - mModel.translation) * (1.0f / mScreenFactor));
                        if (belowAxisLimit && dy > -0.1f && dy < 0.1f && dx > 0.1f  && dx < 1.0f)
                            type = MOVE_X + index;

                        if (belowPlaneLimit && dx >= quadUV[0] && dx <= quadUV[4] && dy >= quadUV[1] && dy <= quadUV[3])
                            type = MOVE_XY + index;

                        if (gizmoHitProportion)
                            *gizmoHitProportion = makeVect(dx, dy, 0.0f);
                    }
                    return type;
                }

                void HandleTranslation(float *matrix, float *deltaMatrix, int& type, float *snap)
                {
                    ImGuiIO& io = ImGui::GetIO();
                    bool applyRotationLocaly = mMode == Alignment::Local || type == MOVE_SCREEN;

                    // move
                    if (mbUsing)
                    {
                        const float len = IntersectRayPlane(mRayOrigin, mRayVector, mTranslationPlane);
                        Math::Float4 newPos = mRayOrigin + mRayVector * len;

                        // compute delta
                        Math::Float4 newOrigin = newPos - mRelativeOrigin * mScreenFactor;
                        Math::Float4 delta = newOrigin - mModel.translation;

                        // 1 axis constraint
                        if (mCurrentOperation >= MOVE_X && mCurrentOperation <= MOVE_Z)
                        {
                            int axisIndex = mCurrentOperation - MOVE_X;
                            Math::Float3 const &axisValue = *(Math::Float4*)&mModel.m[axisIndex];
                            float lengthOnAxis = Dot(axisValue, delta);
                            delta = axisValue * lengthOnAxis;
                        }

                        // snap
                        if (snap)
                        {
                            Math::Float4 cumulativeDelta = mModel.translation + delta - mMatrixOrigin;
                            if (applyRotationLocaly)
                            {
                                Math::Float4x4 modelSourceNormalized = mModelSource;
                                modelSourceNormalized.OrthoNormalize();
                                Math::Float4x4 modelSourceNormalizedInverse;
                                modelSourceNormalizedInverse.Inverse(modelSourceNormalized);
                                cumulativeDelta.TransformVector(modelSourceNormalizedInverse);
                                ComputeSnap(cumulativeDelta, snap);
                                cumulativeDelta.TransformVector(modelSourceNormalized);
                            }
                            else
                            {
                                ComputeSnap(cumulativeDelta, snap);
                            }
                            delta = mMatrixOrigin + cumulativeDelta - mModel.translation;

                        }

                        // compute matrix & delta
                        Math::Float4x4 deltaMatrixTranslation;
                        deltaMatrixTranslation.Translation(delta);
                        if (deltaMatrix)
                            memcpy(deltaMatrix, deltaMatrixTranslation.m16, sizeof(float) * 16);


                        Math::Float4x4 res = mModelSource * deltaMatrixTranslation;
                        *(Math::Float4x4*)matrix = res;

                        if (!io.MouseDown[0])
                            mbUsing = false;

                        type = mCurrentOperation;
                    }
                    else
                    {
                        // find new possible way to move
                        Math::Float4 gizmoHitProportion;
                        type = GetMoveType(&gizmoHitProportion);
                        if (io.MouseDown[0] && type != NONE)
                        {
                            mbUsing = true;
                            mCurrentOperation = type;
                            const Math::Float4 movePlanNormal[] = { mModel.v.up, mModel.v.dir, mModel.v.right, mModel.v.dir, mModel.v.right, mModel.v.up, -mCameraDir };
                            // pickup plane
                            mTranslationPlane = BuildPlane(mModel.translation, movePlanNormal[type - MOVE_X]);
                            const float len = IntersectRayPlane(mRayOrigin, mRayVector, mTranslationPlane);
                            mTranslationPlaneOrigin = mRayOrigin + mRayVector * len;
                            mMatrixOrigin = mModel.translation;

                            mRelativeOrigin = (mTranslationPlaneOrigin - mModel.translation) * (1.0f / mScreenFactor);
                        }
                    }
                }

                void HandleScale(float *matrix, float *deltaMatrix, int& type, float *snap)
                {
                    ImGuiIO& io = ImGui::GetIO();

                    if (!mbUsing)
                    {
                        // find new possible way to scale
                        type = GetScaleType();
                        if (io.MouseDown[0] && type != NONE)
                        {
                            mbUsing = true;
                            mCurrentOperation = type;
                            const Math::Float4 movePlanNormal[] = { mModel.v.up, mModel.v.dir, mModel.v.right, mModel.v.dir, mModel.v.up, mModel.v.right, -mCameraDir };
                            // pickup plane

                            mTranslationPlane = BuildPlane(mModel.translation, movePlanNormal[type - SCALE_X]);
                            const float len = IntersectRayPlane(mRayOrigin, mRayVector, mTranslationPlane);
                            mTranslationPlaneOrigin = mRayOrigin + mRayVector * len;
                            mMatrixOrigin = mModel.translation;
                            mScale.Set(1.0f, 1.0f, 1.0f);
                            mRelativeOrigin = (mTranslationPlaneOrigin - mModel.translation) * (1.0f / mScreenFactor);
                            mScaleValueOrigin = makeVect(mModelSource.v.right.Length(), mModelSource.v.up.Length(), mModelSource.v.dir.Length());
                            mSaveMousePosx = io.MousePos.x;
                        }
                    }
                    // scale
                    if (mbUsing)
                    {
                        const float len = IntersectRayPlane(mRayOrigin, mRayVector, mTranslationPlane);
                        Math::Float4 newPos = mRayOrigin + mRayVector * len;
                        Math::Float4 newOrigin = newPos - mRelativeOrigin * mScreenFactor;
                        Math::Float4 delta = newOrigin - mModel.translation;

                        // 1 axis constraint
                        if (mCurrentOperation >= SCALE_X && mCurrentOperation <= SCALE_Z)
                        {
                            int axisIndex = mCurrentOperation - SCALE_X;
                            Math::Float3 const &axisValue = *(Math::Float4*)&mModel.m[axisIndex];
                            float lengthOnAxis = Dot(axisValue, delta);
                            delta = axisValue * lengthOnAxis;

                            Math::Float4 baseVector = mTranslationPlaneOrigin - mModel.translation;
                            float ratio = Dot(axisValue, baseVector + delta) / Dot(axisValue, baseVector);

                            mScale[axisIndex] = max(ratio, 0.001f);
                        }
                        else
                        {
                            float scaleDelta = (io.MousePos.x - mSaveMousePosx)  * 0.01f;
                            mScale.Set(max(1.0f + scaleDelta, 0.001f));
                        }

                        // snap
                        if (snap)
                        {
                            float scaleSnap[] = { snap[0], snap[0], snap[0] };
                            ComputeSnap(mScale, scaleSnap);
                        }

                        // no 0 allowed
                        for (int index = 0; index < 3; index++)
                            mScale[index] = max(mScale[index], 0.001f);

                        // compute matrix & delta
                        Math::Float4x4 deltaMatrixScale;
                        deltaMatrixScale.Scale(mScale * mScaleValueOrigin);

                        Math::Float4x4 res = deltaMatrixScale * mModel;
                        *(Math::Float4x4*)matrix = res;

                        if (deltaMatrix)
                        {
                            deltaMatrixScale.Scale(mScale);
                            memcpy(deltaMatrix, deltaMatrixScale.m16, sizeof(float) * 16);
                        }

                        if (!io.MouseDown[0])
                            mbUsing = false;

                        type = mCurrentOperation;
                    }
                }

                void HandleRotation(float *matrix, float *deltaMatrix, int& type, float *snap)
                {
                    ImGuiIO& io = ImGui::GetIO();
                    bool applyRotationLocaly = mMode == Alignment::Local;

                    if (!mbUsing)
                    {
                        type = GetRotateType();

                        if (type == ROTATE_SCREEN)
                        {
                            applyRotationLocaly = true;
                        }

                        if (io.MouseDown[0] && type != NONE)
                        {
                            mbUsing = true;
                            mCurrentOperation = type;
                            const Math::Float4 rotatePlanNormal[] = { mModel.v.right, mModel.v.up, mModel.v.dir, -mCameraDir };
                            // pickup plane
                            if (applyRotationLocaly)
                            {
                                mTranslationPlane = BuildPlane(mModel.translation, rotatePlanNormal[type - ROTATE_X]);
                            }
                            else
                            {
                                mTranslationPlane = BuildPlane(mModelSource.translation, directionUnary[type - ROTATE_X]);
                            }

                            const float len = IntersectRayPlane(mRayOrigin, mRayVector, mTranslationPlane);
                            Math::Float4 localPos = mRayOrigin + mRayVector * len - mModel.translation;
                            mRotationVectorSource = Normalized(localPos);
                            mRotationAngleOrigin = ComputeAngleOnPlan();
                        }
                    }

                    // rotation
                    if (mbUsing)
                    {
                        mRotationAngle = ComputeAngleOnPlan();
                        if (snap)
                        {
                            float snapInRadian = Math::DegreesToRadians(snap[0]);
                            ComputeSnap(&mRotationAngle, snapInRadian);
                        }
                        Math::Float4 rotationAxisLocalSpace;

                        rotationAxisLocalSpace.TransformVector(makeVect(mTranslationPlane.x, mTranslationPlane.y, mTranslationPlane.z, 0.0f), mModelInverse);
                        rotationAxisLocalSpace.normalize();

                        Math::Float4x4 deltaRotation;
                        deltaRotation.RotationAxis(rotationAxisLocalSpace, mRotationAngle - mRotationAngleOrigin);
                        mRotationAngleOrigin = mRotationAngle;

                        Math::Float4x4 scaleOrigin;
                        scaleOrigin.Scale(mModelScaleOrigin);

                        if (applyRotationLocaly)
                        {
                            *(Math::Float4x4*)matrix = scaleOrigin * deltaRotation * mModel;
                        }
                        else
                        {
                            Math::Float4x4 res = mModelSource;
                            res.translation.Set(0.0f);

                            *(Math::Float4x4*)matrix = res * deltaRotation;
                            ((Math::Float4x4*)matrix)->translation = mModelSource.translation;
                        }

                        if (deltaMatrix)
                        {
                            *(Math::Float4x4*)deltaMatrix = mModelInverse * deltaRotation * mModel;
                        }

                        if (!io.MouseDown[0])
                            mbUsing = false;

                        type = mCurrentOperation;
                    }
                }

                void HandleAndDrawTranslation(float *matrix, float *deltaMatrix, float *snap)
                {
                    int type = NONE;
                    HandleTranslation(matrix, deltaMatrix, type, snap);
                    DrawTranslationGizmo(type);
                }

                void HandleAndDrawRotation(float *matrix, float *deltaMatrix, float *snap)
                {
                    int type = NONE;
                    HandleRotation(matrix, deltaMatrix, type, snap);
                    DrawRotationGizmo(type);
                }

                void HandleAndDrawScale(float *matrix, float *deltaMatrix, float *snap)
                {
                    int type = NONE;
                    HandleScale(matrix, deltaMatrix, type, snap);
                    DrawScaleGizmo(type);
                }

                void HandleAndDrawBounds(float *bounds, Math::Float4x4 *matrix, float *snapValues)
                {
                    ImGuiIO& io = ImGui::GetIO();
                    ImDrawList* drawList = mDrawList;

                    // compute best projection axis
                    Math::Float4 bestAxisWorldDirection;
                    int bestAxis = mBoundsBestAxis;
                    if (!mbUsingBounds)
                    {

                        float bestDot = 0.0f;
                        for (unsigned int index = 0; index < 3; index++)
                        {
                            Math::Float4 dirPlaneNormalWorld;
                            dirPlaneNormalWorld.TransformVector(directionUnary[index], mModelSource);
                            dirPlaneNormalWorld.normalize();

                            float dt = Dot(Normalized(mCameraEye - mModelSource.translation), dirPlaneNormalWorld);
                            if (std::abs(dt) >= bestDot)
                            {
                                bestDot = std::abs(dt);
                                bestAxis = index;
                                bestAxisWorldDirection = dirPlaneNormalWorld;
                            }
                        }
                    }

                    // corners
                    Math::Float4 aabb[4];

                    int secondAxis = (bestAxis + 1) % 3;
                    int thirdAxis = (bestAxis + 2) % 3;

                    for (int index = 0; index < 4; index++)
                    {
                        aabb[index][3] = aabb[index][bestAxis] = 0.0f;
                        aabb[index][secondAxis] = bounds[secondAxis + 3 * (index >> 1)];
                        aabb[index][thirdAxis] = bounds[thirdAxis + 3 * ((index >> 1) ^ (index & 1))];
                    }

                    // draw bounds
                    unsigned int anchorAlpha = mbEnable ? 0xFF000000 : 0x80000000;

                    Math::Float4x4 boundsMVP = mModelSource * mViewProjection;
                    for (int index = 0; index < 4; index++)
                    {
                        ImVec2 worldBound1 = worldToPos(aabb[index], boundsMVP);
                        ImVec2 worldBound2 = worldToPos(aabb[(index + 1) % 4], boundsMVP);
                        float boundDistance = sqrtf(ImLengthSqr(worldBound1 - worldBound2));
                        int stepCount = (int)(boundDistance / 10.0f);
                        float stepLength = 1.0f / (float)stepCount;
                        for (int j = 0; j < stepCount; j++)
                        {
                            float t1 = (float)j * stepLength;
                            float t2 = (float)j * stepLength + stepLength * 0.5f;
                            ImVec2 worldBoundSS1 = ImLerp(worldBound1, worldBound2, ImVec2(t1, t1));
                            ImVec2 worldBoundSS2 = ImLerp(worldBound1, worldBound2, ImVec2(t2, t2));
                            drawList->AddLine(worldBoundSS1, worldBoundSS2, 0xAAAAAA + anchorAlpha, 3.0f);
                        }
                        Math::Float4 midPoint = (aabb[index] + aabb[(index + 1) % 4]) * 0.5f;
                        ImVec2 midBound = worldToPos(midPoint, boundsMVP);
                        static const float AnchorBigRadius = 8.0f;
                        static const float AnchorSmallRadius = 6.0f;
                        bool overBigAnchor = ImLengthSqr(worldBound1 - io.MousePos) <= (AnchorBigRadius*AnchorBigRadius);
                        bool overSmallAnchor = ImLengthSqr(midBound - io.MousePos) <= (AnchorBigRadius*AnchorBigRadius);


                        unsigned int bigAnchorColor = overBigAnchor ? selectionColor : (0xAAAAAA + anchorAlpha);
                        unsigned int smallAnchorColor = overSmallAnchor ? selectionColor : (0xAAAAAA + anchorAlpha);

                        drawList->AddCircleFilled(worldBound1, AnchorBigRadius, bigAnchorColor);
                        drawList->AddCircleFilled(midBound, AnchorSmallRadius, smallAnchorColor);
                        int oppositeIndex = (index + 2) % 4;
                        // big anchor on corners
                        if (!mbUsingBounds && mbEnable && overBigAnchor && io.MouseDown[0])
                        {
                            mBoundsPivot.TransformPoint(aabb[(index + 2) % 4], mModelSource);
                            mBoundsAnchor.TransformPoint(aabb[index], mModelSource);
                            mBoundsPlan = BuildPlane(mBoundsAnchor, bestAxisWorldDirection);
                            mBoundsBestAxis = bestAxis;
                            mBoundsAxis[0] = secondAxis;
                            mBoundsAxis[1] = thirdAxis;

                            mBoundsLocalPivot.Set(0.0f);
                            mBoundsLocalPivot[secondAxis] = aabb[oppositeIndex][secondAxis];
                            mBoundsLocalPivot[thirdAxis] = aabb[oppositeIndex][thirdAxis];

                            mbUsingBounds = true;
                            mBoundsMatrix = mModelSource;
                        }
                        // small anchor on middle of segment
                        if (!mbUsingBounds && mbEnable && overSmallAnchor && io.MouseDown[0])
                        {
                            Math::Float4 midPointOpposite = (aabb[(index + 2) % 4] + aabb[(index + 3) % 4]) * 0.5f;
                            mBoundsPivot.TransformPoint(midPointOpposite, mModelSource);
                            mBoundsAnchor.TransformPoint(midPoint, mModelSource);
                            mBoundsPlan = BuildPlane(mBoundsAnchor, bestAxisWorldDirection);
                            mBoundsBestAxis = bestAxis;
                            int indices[] = { secondAxis , thirdAxis };
                            mBoundsAxis[0] = indices[index % 2];
                            mBoundsAxis[1] = -1;

                            mBoundsLocalPivot.Set(0.0f);
                            mBoundsLocalPivot[mBoundsAxis[0]] = aabb[oppositeIndex][indices[index % 2]];// bounds[mBoundsAxis[0]] * (((index + 1) & 2) ? 1.0f : -1.0f);

                            mbUsingBounds = true;
                            mBoundsMatrix = mModelSource;
                        }
                    }

                    if (mbUsingBounds)
                    {
                        Math::Float4x4 scale;
                        scale.SetToIdentity();

                        // compute projected mouse position on plane
                        const float len = IntersectRayPlane(mRayOrigin, mRayVector, mBoundsPlan);
                        Math::Float4 newPos = mRayOrigin + mRayVector * len;

                        // compute a reference and delta vectors base on mouse move
                        Math::Float4 deltaVector = (newPos - mBoundsPivot).Abs();
                        Math::Float4 referenceVector = (mBoundsAnchor - mBoundsPivot).Abs();

                        // for 1 or 2 axes, compute a ratio that's used for scale and snap it based on resulting length
                        for (int index = 0; index < 2; index++)
                        {
                            int axisIndex = mBoundsAxis[index];
                            if (axisIndex == -1)
                            {
                                continue;
                            }

                            float ratioAxis = 1.0f;
                            Math::Float4 axisDir = mBoundsMatrix.component[axisIndex].Abs();

                            float dtAxis = axisDir.Dot(referenceVector);
                            float boundSize = bounds[axisIndex + 3] - bounds[axisIndex];
                            if (dtAxis > FLT_EPSILON)
                            {
                                ratioAxis = axisDir.Dot(deltaVector) / dtAxis;
                            }

                            if (snapValues)
                            {
                                float length = boundSize * ratioAxis;
                                ComputeSnap(&length, snapValues[axisIndex]);
                                if (boundSize > FLT_EPSILON)
                                {
                                    ratioAxis = length / boundSize;
                                }
                            }

                            scale.component[axisIndex] *= ratioAxis;
                        }

                        // transform matrix
                        Math::Float4x4 preScale, postScale;
                        preScale.Translation(-mBoundsLocalPivot);
                        postScale.Translation(mBoundsLocalPivot);
                        Math::Float4x4 res = preScale * scale * postScale * mBoundsMatrix;
                        *matrix = res;

                        // info text
                        char tmps[512];
                        ImVec2 destinationPosOnScreen = worldToPos(mModel.translation, mViewProjection);
                        ImFormatString(tmps, sizeof(tmps), "X: %.2f Y: %.2f Z:%.2f"
                            , (bounds[3] - bounds[0]) * mBoundsMatrix.component[0].Length() * scale.component[0].Length()
                            , (bounds[4] - bounds[1]) * mBoundsMatrix.component[1].Length() * scale.component[1].Length()
                            , (bounds[5] - bounds[2]) * mBoundsMatrix.component[2].Length() * scale.component[2].Length()
                        );

                        drawList->AddText(ImVec2(destinationPosOnScreen.x + 15, destinationPosOnScreen.y + 15), 0xFF000000, tmps);
                        drawList->AddText(ImVec2(destinationPosOnScreen.x + 14, destinationPosOnScreen.y + 14), 0xFFFFFFFF, tmps);
                    }

                    if (!io.MouseDown[0])
                    {
                        mbUsingBounds = false;
                    }
                }

                void DecomposeMatrixToComponents(const float *matrix, float *translation, float *rotation, float *scale)
                {
                    Math::Float4x4 mat = *(Math::Float4x4*)matrix;

                    scale[0] = mat.v.right.Length();
                    scale[1] = mat.v.up.Length();
                    scale[2] = mat.v.dir.Length();

                    mat.OrthoNormalize();

                    rotation[0] = Math::RadiansToDegrees(std::atan2(mat.m[1][2], mat.m[2][2]));
                    rotation[1] = Math::RadiansToDegrees(std::atan2(-mat.m[0][2], sqrtf(mat.m[1][2] * mat.m[1][2] + mat.m[2][2] * mat.m[2][2])));
                    rotation[2] = Math::RadiansToDegrees(std::atan2(mat.m[0][1], mat.m[0][0]));

                    translation[0] = mat.translation.x;
                    translation[1] = mat.translation.y;
                    translation[2] = mat.translation.z;
                }

                void RecomposeMatrixFromComponents(const float *translation, const float *rotation, const float *scale, float *matrix)
                {
                    Math::Float4x4& mat = *(Math::Float4x4*)matrix;

                    Math::Float4x4 rot[3];
                    for (int index = 0; index < 3; index++)
                    {
                        rot[index].RotationAxis(directionUnary[index], Math::DegreesToRadians(rotation[index]));
                    }

                    mat = rot[0] * rot[1] * rot[2];

                    float validScale[3];
                    for (int index = 0; index < 3; index++)
                    {
                        if (std::abs(scale[index]) < FLT_EPSILON)
                        {
                            validScale[index] = 0.001f;
                        }
                        else
                        {
                            validScale[index] = scale[index];
                        }
                    }

                    mat.v.right *= validScale[0];
                    mat.v.up *= validScale[1];
                    mat.v.dir *= validScale[2];
                    mat.translation.Set(translation[0], translation[1], translation[2], 1.0f);
                }

                void Manipulate(const float *view, const float *projection, int operation, int mode, float *matrix, float *deltaMatrix, float *snap, float *localBounds, float *boundsSnap)
                {
                    ComputeContext(view, projection, matrix, mode);

                    // set delta to identity 
                    if (deltaMatrix)
                    {
                        ((Math::Float4x4*)deltaMatrix)->SetToIdentity();
                    }

                    // behind camera
                    Math::Float4 camSpacePosition;
                    camSpacePosition.TransformPoint(makeVect(0.0f, 0.0f, 0.0f), mMVP);
                    if (camSpacePosition.z < 0.001f)
                    {
                        return;
                    }

                    // -- 
                    switch (operation)
                    {
                    case Operation::Rotate:
                        HandleAndDrawRotation(matrix, deltaMatrix, snap);
                        break;
                            
                    case Operation::Translate:
                        HandleAndDrawTranslation(matrix, deltaMatrix, snap);
                        break;
                            
                    case Operation::Scale:
                        HandleAndDrawScale(matrix, deltaMatrix, snap);
                        break;

                    case Operation::Bounds:
                        HandleAndDrawBounds(localBounds, (Math::Float4x4*)matrix, boundsSnap);
                        break;
                    };
                }

                void DrawCube(const float *view, const float *projection, float *matrix)
                {
                    Math::Float4x4 viewInverse;
                    viewInverse.Inverse(*(Math::Float4x4*)view);
                    const Math::Float4x4& model = *(Math::Float4x4*)matrix;
                    Math::Float4x4 res = *(Math::Float4x4*)matrix * *(Math::Float4x4*)view * *(Math::Float4x4*)projection;

                    for (int iFace = 0; iFace < 6; iFace++)
                    {
                        const int normalIndex = (iFace % 3);
                        const int perpXIndex = (normalIndex + 1) % 3;
                        const int perpYIndex = (normalIndex + 2) % 3;
                        const float invert = (iFace > 2) ? -1.0f : 1.0f;

                        const Math::Float4 faceCoords[4] = { directionUnary[normalIndex] + directionUnary[perpXIndex] + directionUnary[perpYIndex],
                            directionUnary[normalIndex] + directionUnary[perpXIndex] - directionUnary[perpYIndex],
                            directionUnary[normalIndex] - directionUnary[perpXIndex] - directionUnary[perpYIndex],
                            directionUnary[normalIndex] - directionUnary[perpXIndex] + directionUnary[perpYIndex],
                        };

                        // clipping
                        bool skipFace = false;
                        for (unsigned int iCoord = 0; iCoord < 4; iCoord++)
                        {
                            Math::Float4 camSpacePosition;
                            camSpacePosition.TransformPoint(faceCoords[iCoord] * 0.5f * invert, res);
                            if (camSpacePosition.z < 0.001f)
                            {
                                skipFace = true;
                                break;
                            }
                        }
                        if (skipFace)
                            continue;

                        // 3D->2D
                        ImVec2 faceCoordsScreen[4];
                        for (unsigned int iCoord = 0; iCoord < 4; iCoord++)
                            faceCoordsScreen[iCoord] = worldToPos(faceCoords[iCoord] * 0.5f * invert, res);

                        // back face culling 
                        Math::Float4 cullPos, cullNormal;
                        cullPos.TransformPoint(faceCoords[0] * 0.5f * invert, model);
                        cullNormal.TransformVector(directionUnary[normalIndex] * invert, model);
                        float dt = Dot(Normalized(cullPos - viewInverse.translation), Normalized(cullNormal));
                        if (dt > 0.0f)
                            continue;

                        // draw face with lighter color
                        mDrawList->AddConvexPolyFilled(faceCoordsScreen, 4, directionColor[normalIndex] | 0x808080, true);
                    }
                }
            };

            WorkSpace::WorkSpace(void)
                : context((Context *)ImGui::MemAlloc(sizeof(Context)))
            {
                IM_PLACEMENT_NEW(context) Context();
            }

            WorkSpace::~WorkSpace(void)
            {
                ImGui::MemFree(context);
            }

            void WorkSpace::beginFrame(void)
            {
                context->BeginFrame();
            }

            void WorkSpace::setViewPort(float x, float y, float width, float height)
            {
                context->SetRect(x, y, width, height);
            }

            void WorkSpace::manipulate(float const *view, float const *projection, int operation, int alignment, float *matrix, float *deltaMatrix, float *snap, float *localBounds, float *boundsSnap)
            {
                context->Manipulate(view, projection, operation, alignment, matrix, deltaMatrix, snap, localBounds, boundsSnap);
            }
        }; // Gizmo
    }; // namespace UI
}; // namespace Gek