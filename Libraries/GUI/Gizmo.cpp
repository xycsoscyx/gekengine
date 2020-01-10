#include "GEK/GUI/Gizmo.hpp"
#include "GEK/Shapes/Plane.hpp"
#include <imgui_internal.h>

namespace Gek
{
    namespace UI
    {
        namespace Gizmo
        {
			static constexpr float screenRotateSize = 0.1f;

			static constexpr float angleLimit = 0.96f;
			static constexpr float planeLimit = 0.2f;

            static const Math::Float3 directionUnary[3] =
            {
                Math::Float3(1.0f, 0.0f, 0.0f),
                Math::Float3(0.0f, 1.0f, 0.0f),
                Math::Float3(0.0f, 0.0f, 1.0f)
            };

			static constexpr ImU32 directionColor[3] =
            {
                0xFF0000AA,
                0xFF00AA00,
                0xFFAA0000
            };

			static constexpr ImU32 selectionColor = 0xFF1080FF;
			static constexpr ImU32 inactiveColor = 0x99999999;
			static constexpr ImU32 translationLineColor = 0xAAAAAAAA;
			static constexpr std::string_view translationInfoMask[] =
            {
                "X : %5.3f"sv,
                "Y : %5.3f"sv,
                "Z : %5.3f"sv,
                "X : %5.3f Y : %5.3f"sv,
                "Y : %5.3f Z : %5.3f"sv,
                "X : %5.3f Z : %5.3f"sv,
                "X : %5.3f Y : %5.3f Z : %5.3f"sv,
            };

			static constexpr std::string_view scaleInfoMask[] =
            {
                "X : %5.2f"sv,
                "Y : %5.2f"sv,
                "Z : %5.2f"sv,
                "XYZ : %5.2f"sv,
            };

			static constexpr std::string_view rotationInfoMask[] =
            {
                "X : %5.2f deg %5.2f rad"sv,
                "Y : %5.2f deg %5.2f rad"sv,
                "Z : %5.2f deg %5.2f rad"sv,
                "Screen : %5.2f deg %5.2f rad"sv,
            };

			static constexpr int translationInfoIndex[] =
            {
                0, 0, 0,
                1, 0, 0,
                2, 0, 0,
                0, 1, 0,
                1, 2, 0,
                0, 2, 1,
                0, 1, 2,
            };

			static constexpr float quadMinimum = 0.5f;
			static constexpr float quadMaximum = 0.8f;
			static constexpr float quadCoords[8] =
            {
                quadMinimum, quadMinimum,
                quadMinimum, quadMaximum,
                quadMaximum, quadMaximum,
                quadMaximum, quadMinimum
            };

			static constexpr int halfCircleSegmentCount = 64;
			static constexpr float snapTension = 0.5f;

            struct Control
            {
                enum
                {
                    None = 0,
                    MoveX,
                    MoveY,
                    MoveZ,
                    MoveXY,
                    MoveXZ,
                    MoveYZ,
                    MoveScreen,
                    RotateX,
                    RotateY,
                    RotateZ,
                    RotateScreen,
                    ScaleX,
                    ScaleY,
                    ScaleZ,
                    ScaleXYZ,
                };
            };

            struct Context
            {
                ImDrawList* currentDrawList = nullptr;

                Alignment currentAlignment = Alignment::Local;
                Math::Float4x4 viewMatrix;
                Math::Float4x4 projectionMatrix;
                Math::Float4x4 modelMatrix;
                Math::Float4x4 inverseModelMatrix;
                Math::Float4x4 sourceModelMatrix;
                Math::Float4x4 inverseSourceModelMatrix;
                Math::Float4x4 modelViewProjectionMatrix;
                Math::Float4x4 viewProjectionMatrix;

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

                bool isUsing = false;
                bool isEnabled = true;
                bool isOver = false;

                // translation
                Shapes::Plane mTranslationPlane;
                Math::Float3 mTranslationPlaneOrigin;
                Math::Float3 mMatrixOrigin;

                // rotation
                Math::Float3 mRotationVectorSource;
                float mRotationAngle;
                float mRotationAngleOrigin;
                //Math::Float3 mWorldToLocalAxis;

                // scale
                Math::Float3 mScale;
                Math::Float3 mScaleValueOrigin;
                float mSaveMousePosx;

                // save axis factor when using gizmo
                bool mBelowAxisLimit[3];
                bool mBelowPlaneLimit[3];
                float mAxisFactor[3];

                // bounds stretching
                Math::Float3 mBoundsPivot;
                Math::Float3 mBoundsAnchor;
                Shapes::Plane mBoundsPlane;
                Math::Float3 mBoundsLocalPivot;
                int mBoundsBestAxis;
                int mBoundsAxis[2];
                Math::Float4x4 mBoundsMatrix;

                //
                int currentControl = Control::None;

                Math::Float4 viewPort;
                Math::Float4 viewBounds;

                ImVec2 getPointFromClipPosition(Math::Float4 const &clipPosition)
                {
                    Math::Float2 ndcPosition = ((clipPosition.xy / clipPosition.w) * Math::Float2(1.0f, -1.0f));
                    Math::Float2 screenPosition = ((ndcPosition + 1.0f) * 0.5f);
                    screenPosition = ((screenPosition * viewPort.size) + viewPort.position);
                    return *(ImVec2 *)&screenPosition;
                }

                ImVec2 getPointFromPosition(Math::Float3 const &position, Math::Float4x4 const &matrix)
                {
                    Math::Float4 clipPosition = matrix.transform(Math::Float4(position, 1.0f));
                    Math::Float2 ndcPosition = ((clipPosition.xy / clipPosition.w) * Math::Float2(1.0f, -1.0f));
                    Math::Float2 screenPosition = ((ndcPosition + 1.0f) * 0.5f);
                    screenPosition = ((screenPosition * viewPort.size) + viewPort.position);
                    return *(ImVec2 *)&screenPosition;
                }

                void getMouseRay(Math::Float3 &rayOrigin, Math::Float3 &rayDir)
                {
                    ImGuiIO &imGuiIO = ImGui::GetIO();
                    Math::Float4x4 mViewProjInverse = viewProjectionMatrix.getInverse();
                    auto mouse = (((*(Math::Float2 *)&imGuiIO.MousePos) - viewPort.position) / viewPort.size);
                    mouse = ((mouse * 2.0f) - 1.0f) * Math::Float2(1.0f, -1.0f);

                    Math::Float4 rayStart;
                    rayStart = mViewProjInverse.transform(Math::Float4(mouse.x, mouse.y, 0.0f, 1.0f));
                    rayOrigin = rayStart.xyz;
                    rayOrigin *= 1.0f / rayStart.w;

                    Math::Float4 rayEnd;
                    rayEnd = mViewProjInverse.transform(Math::Float4(mouse.x, mouse.y, 1.0f, 1.0f));
                    rayEnd *= 1.0f / rayEnd.w;
                    rayDir = (rayEnd.xyz - rayOrigin).getNormal();
                }

                float IntersectRayPlane(Math::Float3 const &rOrigin, Math::Float3 const &rVector, Shapes::Plane const &plane)
                {
                    float numer = plane.getDistance(rOrigin);
                    float denom = plane.normal.dot(rVector);
                    if (std::abs(denom) < FLT_EPSILON)  // normal is orthogonal to vector, cant intersect
                    {
                        return -1.0f;
                    }

                    return -(numer / denom);
                }

                void beginFrame(Math::Float4x4 const &view, Math::Float4x4 const &projection, float x, float y, float width, float height)
                {
                    currentDrawList = ImGui::GetWindowDrawList();
                    if (!currentDrawList)
                    {
                        ImGuiIO &imGuiIO = ImGui::GetIO();
                        ImGui::SetNextWindowSize(imGuiIO.DisplaySize);
                        ImGui::Begin("gizmo", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoBringToFrontOnFocus);
                        currentDrawList = ImGui::GetWindowDrawList();
                        ImGui::End();
                    }

                    viewPort = Math::Float4(x, y, width, height);
                    viewBounds.minimum = viewPort.position;
                    viewBounds.maximum = viewPort.position + viewPort.size;
                    currentDrawList->PushClipRect(*(ImVec2 *)&viewBounds.minimum, *(ImVec2 *)&viewBounds.maximum, false);

                    viewMatrix = view;
                    projectionMatrix = projection;
                    viewProjectionMatrix = viewMatrix * projectionMatrix;

                    Math::Float4x4 viewInverse = viewMatrix.getInverse();
                    mCameraDir = viewInverse.rz.xyz;
                    mCameraEye = viewInverse.rw.xyz;
                    mCameraRight = viewInverse.rx.xyz;
                    mCameraUp = viewInverse.ry.xyz;

                    getMouseRay(mRayOrigin, mRayVector);
                }

                void Enable(bool enable)
                {
                    isEnabled = enable;
                    if (!enable)
                    {
                        isUsing = false;
                    }
                }

                float GetUniform(Math::Float3 const &position, Math::Float4x4 const &matrix)
                {
                    return matrix.transform(Math::Float4(position, 1.0f)).w;
                }

                void computeContext(Math::Float4x4 const &matrix, Alignment alignment)
                {
                    currentAlignment = alignment;
                    if (currentAlignment == Alignment::Local)
                    {
                        modelMatrix = matrix;
                        modelMatrix.orthonormalize();
                    }
                    else
                    {
                        modelMatrix = Math::Float4x4::MakeTranslation(matrix.translation.xyz);
                    }

                    sourceModelMatrix = matrix;
                    mModelScaleOrigin.set(sourceModelMatrix.rx.getLength(), sourceModelMatrix.ry.getLength(), sourceModelMatrix.rz.getLength());

                    inverseModelMatrix = modelMatrix.getInverse();
                    inverseSourceModelMatrix = sourceModelMatrix.getInverse();
                    modelViewProjectionMatrix = modelMatrix * viewProjectionMatrix;

                    mScreenFactor = 0.15f * GetUniform(modelMatrix.rw.xyz, viewProjectionMatrix);

                    ImVec2 centerSSpace = getPointFromPosition(Math::Float3(0.0f, 0.0f, 0.0f), modelViewProjectionMatrix);
                    mScreenSquareCenter = centerSSpace;
                    mScreenSquareMin = ImVec2(centerSSpace.x - 10.0f, centerSSpace.y - 10.0f);
                    mScreenSquareMax = ImVec2(centerSSpace.x + 10.0f, centerSSpace.y + 10.0f);
                }

                void ComputeColors(ImU32 *colors, int control, Operation operation)
                {
                    if (isEnabled)
                    {
                        switch (operation)
                        {
                        case Operation::Translate:
                            colors[0] = (control == Control::MoveScreen) ? selectionColor : 0xFFFFFFFF;
                            for (int index = 0; index < 3; index++)
                            {
                                int colorPlaneIndex = (index + 2) % 3;
                                colors[index + 1] = (control == (int)(Control::MoveX + index)) ? selectionColor : directionColor[index];
                                colors[index + 4] = (control == (int)(Control::MoveXY + index)) ? selectionColor : directionColor[colorPlaneIndex];
                            }

                            break;

                        case Operation::Rotate:
                            colors[0] = (control == Control::RotateScreen) ? selectionColor : 0xFFFFFFFF;
                            for (int index = 0; index < 3; index++)
                            {
                                colors[index + 1] = (control == (int)(Control::RotateX + index)) ? selectionColor : directionColor[index];
                            }

                            break;

                        case Operation::Scale:
                            colors[0] = (control == Control::ScaleXYZ) ? selectionColor : 0xFFFFFFFF;
                            for (int index = 0; index < 3; index++)
                            {
                                colors[index + 1] = (control == (int)(Control::ScaleX + index)) ? selectionColor : directionColor[index];
                            }

                            break;

                        case Operation::Bounds:
                            break;
                        };
                    }
                    else
                    {
                        for (int index = 0; index < 7; index++)
                        {
                            colors[index] = inactiveColor;
                        }
                    }
                }

                void ComputeTripodAxisAndVisibility(int axisIndex, Math::Float3& dirPlaneX, Math::Float3& dirPlaneY, bool& belowAxisLimit, bool& belowPlaneLimit)
                {
                    const int planeNormal = (axisIndex + 2) % 3;
                    dirPlaneX = directionUnary[axisIndex];
                    dirPlaneY = directionUnary[(axisIndex + 1) % 3];
                    if (isUsing)
                    {
                        // when using, use stored factors so the gizmo doesn't flip when we translate
                        belowAxisLimit = mBelowAxisLimit[axisIndex];
                        belowPlaneLimit = mBelowPlaneLimit[axisIndex];

                        dirPlaneX *= mAxisFactor[axisIndex];
                        dirPlaneY *= mAxisFactor[(axisIndex + 1) % 3];
                    }
                    else
                    {
                        Math::Float3 dirPlaneNormalWorld;
                        dirPlaneNormalWorld = modelMatrix.rotate(directionUnary[planeNormal]);
                        dirPlaneNormalWorld.normalize();

                        Math::Float3 dirPlaneXWorld = modelMatrix.rotate(dirPlaneX);
                        dirPlaneXWorld.normalize();

                        Math::Float3 dirPlaneYWorld = modelMatrix.rotate(dirPlaneY);
                        dirPlaneYWorld.normalize();

                        Math::Float3 cameraEyeToGizmo = (modelMatrix.rw.xyz - mCameraEye).getNormal();
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

                void ComputeSnap(float*value, float snap)
                {
                    if (snap <= FLT_EPSILON)
                    {
                        return;
                    }

                    float modulo = std::fmod(*value, snap);
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

                void ComputeSnap(Math::Float3& value, float *snap)
                {
                    for (int index = 0; index < 3; index++)
                    {
                        ComputeSnap(&value[index], snap[index]);
                    }
                }

                float ComputeAngleOnPlane()
                {
                    const float len = IntersectRayPlane(mRayOrigin, mRayVector, mTranslationPlane);
                    Math::Float3 localPos = (mRayOrigin + mRayVector * len - modelMatrix.rw.xyz).getNormal();

                    Math::Float3 perpendicularVector;
                    perpendicularVector = mRotationVectorSource.cross(mTranslationPlane.normal);
                    perpendicularVector.normalize();
                    float acosAngle = std::clamp(localPos.dot(mRotationVectorSource), -0.9999f, 0.9999f);
                    float angle = std::acos(acosAngle);
                    angle *= (localPos.dot(perpendicularVector) < 0.0f) ? 1.0f : -1.0f;
                    return angle;
                }

                void DrawRotationGizmo(int control)
                {
                    ImDrawList* drawList = currentDrawList;
                    ImGuiIO &imGuiIO = ImGui::GetIO();

                    // colors
                    ImU32 colors[7];
                    ComputeColors(colors, control, Operation::Rotate);
                    Math::Float3 cameraToModelNormalized = inverseModelMatrix.rotate((modelMatrix.rw.xyz - mCameraEye).getNormal());
                    for (int axis = 0; axis < 3; axis++)
                    {
                        ImVec2 circlePos[halfCircleSegmentCount];
                        float angleStart = std::atan2(cameraToModelNormalized[(4 - axis) % 3], cameraToModelNormalized[(3 - axis) % 3]) + Math::Pi * 0.5f;
                        for (uint32_t index = 0; index < halfCircleSegmentCount; index++)
                        {
                            float ng = angleStart + Math::Pi * ((float)index / (float)halfCircleSegmentCount);
                            Math::Float3 axisPos = Math::Float3(std::cos(ng), std::sin(ng), 0.0f);
                            Math::Float3 pos = Math::Float3(axisPos[axis], axisPos[(axis + 1) % 3], axisPos[(axis + 2) % 3]) * mScreenFactor;
                            circlePos[index] = getPointFromPosition(pos, modelViewProjectionMatrix);
                        }

                        drawList->AddPolyline(circlePos, halfCircleSegmentCount, colors[3 - axis], false, 5);
                    }

                    drawList->AddCircle(getPointFromPosition(modelMatrix.rw.xyz, viewProjectionMatrix), screenRotateSize * viewPort.size.height, colors[0], 64, 5);
                    if (isUsing)
                    {
                        ImVec2 circlePos[halfCircleSegmentCount + 1];

                        circlePos[0] = getPointFromPosition(modelMatrix.rw.xyz, viewProjectionMatrix);
                        for (uint32_t index = 1; index < halfCircleSegmentCount; index++)
                        {
                            float ng = mRotationAngle * ((float)(index - 1) / (float)(halfCircleSegmentCount - 1));

                            Math::Float4x4 rotateVectorMatrix;
                            rotateVectorMatrix = Math::Float4x4::MakeAngularRotation(mTranslationPlane.normal, ng);

                            Math::Float3 pos = rotateVectorMatrix.transform(mRotationVectorSource) * mScreenFactor;
                            circlePos[index] = getPointFromPosition(pos + modelMatrix.rw.xyz, viewProjectionMatrix);
                        }

                        drawList->AddConvexPolyFilled(circlePos, halfCircleSegmentCount, 0x801080FF);
                        drawList->AddPolyline(circlePos, halfCircleSegmentCount, 0xFF1080FF, true, 2);

                        ImVec2 destinationPosOnScreen = circlePos[1];
                        char tmps[512];
                        ImFormatString(tmps, sizeof(tmps), rotationInfoMask[control - Control::RotateX].data(), (mRotationAngle / Math::Pi)*180.0f, mRotationAngle);
                        drawList->AddText(ImVec2(destinationPosOnScreen.x + 15, destinationPosOnScreen.y + 15), 0xFF000000, tmps);
                        drawList->AddText(ImVec2(destinationPosOnScreen.x + 14, destinationPosOnScreen.y + 14), 0xFFFFFFFF, tmps);
                    }
                }

                void DrawHatchedAxis(Math::Float3 const &axis)
                {
                    for (int step = 1; step < 10; step++)
                    {
                        ImVec2 baseSSpace2 = getPointFromPosition(axis * 0.05f * (float)(step * 2) * mScreenFactor, modelViewProjectionMatrix);
                        ImVec2 worldDirSSpace2 = getPointFromPosition(axis * 0.05f * (float)(step * 2 + 1) * mScreenFactor, modelViewProjectionMatrix);
                        currentDrawList->AddLine(baseSSpace2, worldDirSSpace2, 0x80000000, 6.0f);
                    }
                }

                void DrawScaleGizmo(int control)
                {
                    ImDrawList* drawList = currentDrawList;

                    // colors
                    ImU32 colors[7];
                    ComputeColors(colors, control, Operation::Scale);

                    // draw screen cirle
                    drawList->AddCircleFilled(mScreenSquareCenter, 12.0f, colors[0], 32);

                    // draw
                    Math::Float3 scaleDisplay(1.0f, 1.0f, 1.0f);
                    if (isUsing)
                    {
                        scaleDisplay = mScale;
                    }

                    for (uint32_t index = 0; index < 3; index++)
                    {
                        Math::Float3 dirPlaneX, dirPlaneY;
                        bool belowAxisLimit, belowPlaneLimit;
                        ComputeTripodAxisAndVisibility(index, dirPlaneX, dirPlaneY, belowAxisLimit, belowPlaneLimit);

                        // draw axis
                        if (belowAxisLimit)
                        {
                            ImVec2 baseSSpace = getPointFromPosition(dirPlaneX * 0.1f * mScreenFactor, modelViewProjectionMatrix);
                            ImVec2 worldDirSSpaceNoScale = getPointFromPosition(dirPlaneX * mScreenFactor, modelViewProjectionMatrix);
                            ImVec2 worldDirSSpace = getPointFromPosition((dirPlaneX * scaleDisplay[index]) * mScreenFactor, modelViewProjectionMatrix);

                            if (isUsing)
                            {
                                drawList->AddLine(baseSSpace, worldDirSSpaceNoScale, 0xFF404040, 6.0f);
                                drawList->AddCircleFilled(worldDirSSpaceNoScale, 10.0f, 0xFF404040);
                            }

                            drawList->AddLine(baseSSpace, worldDirSSpace, colors[index + 1], 6.0f);
                            drawList->AddCircleFilled(worldDirSSpace, 10.0f, colors[index + 1]);

                            if (mAxisFactor[index] < 0.0f)
                            {
                                DrawHatchedAxis(dirPlaneX * scaleDisplay[index]);
                            }
                        }
                    }

                    if (isUsing)
                    {
                        ImVec2 destinationPosOnScreen = getPointFromPosition(modelMatrix.rw.xyz, viewProjectionMatrix);

                        char tmps[512];
                        int componentInfoIndex = (control - Control::ScaleX) * 3;
                        ImFormatString(tmps, sizeof(tmps), scaleInfoMask[control - Control::ScaleX].data(), scaleDisplay[translationInfoIndex[componentInfoIndex]]);
                        drawList->AddText(ImVec2(destinationPosOnScreen.x + 15, destinationPosOnScreen.y + 15), 0xFF000000, tmps);
                        drawList->AddText(ImVec2(destinationPosOnScreen.x + 14, destinationPosOnScreen.y + 14), 0xFFFFFFFF, tmps);
                    }
                }

                void DrawTranslationGizmo(int control)
                {
                    ImDrawList* drawList = currentDrawList;
                    if (!drawList)
                    {
                        return;
                    }

                    // colors
                    ImU32 colors[7];
                    ComputeColors(colors, control, Operation::Translate);

                    // draw screen quad
                    drawList->AddRectFilled(mScreenSquareMin, mScreenSquareMax, colors[0], 2.0f);

                    // draw
                    for (uint32_t index = 0; index < 3; index++)
                    {
                        Math::Float3 dirPlaneX, dirPlaneY;
                        bool belowAxisLimit, belowPlaneLimit;
                        ComputeTripodAxisAndVisibility(index, dirPlaneX, dirPlaneY, belowAxisLimit, belowPlaneLimit);

                        // draw axis
                        if (belowAxisLimit)
                        {
                            ImVec2 baseSSpace = getPointFromPosition(dirPlaneX * 0.1f * mScreenFactor, modelViewProjectionMatrix);
                            ImVec2 worldDirSSpace = getPointFromPosition(dirPlaneX * mScreenFactor, modelViewProjectionMatrix);
                            drawList->AddLine(baseSSpace, worldDirSSpace, colors[index + 1], 6.0f);
                            if (mAxisFactor[index] < 0.0f)
                            {
                                DrawHatchedAxis(dirPlaneX);
                            }
                        }

                        // draw plane
                        if (belowPlaneLimit)
                        {
                            ImVec2 screenQuadPts[4];
                            for (int step = 0; step < 4; step++)
                            {
                                Math::Float3 cornerWorldPos = (dirPlaneX * quadCoords[step * 2] + dirPlaneY  * quadCoords[step * 2 + 1]) * mScreenFactor;
                                screenQuadPts[step] = getPointFromPosition(cornerWorldPos, modelViewProjectionMatrix);
                            }

                            drawList->AddConvexPolyFilled(screenQuadPts, 4, colors[index + 4]);
                        }
                    }

                    if (isUsing)
                    {
                        ImVec2 sourcePosOnScreen = getPointFromPosition(mMatrixOrigin, viewProjectionMatrix);
                        ImVec2 destinationPosOnScreen = getPointFromPosition(modelMatrix.rw.xyz, viewProjectionMatrix);
                        Math::Float3 dif(destinationPosOnScreen.x - sourcePosOnScreen.x, destinationPosOnScreen.y - sourcePosOnScreen.y, 0.0f);
                        dif.normalize();
                        dif *= 5.0f;

                        drawList->AddCircle(sourcePosOnScreen, 6.0f, translationLineColor);
                        drawList->AddCircle(destinationPosOnScreen, 6.0f, translationLineColor);
                        drawList->AddLine(ImVec2(sourcePosOnScreen.x + dif.x, sourcePosOnScreen.y + dif.y), ImVec2(destinationPosOnScreen.x - dif.x, destinationPosOnScreen.y - dif.y), translationLineColor, 2.0f);

                        char tmps[512];
                        Math::Float3 deltaInfo = modelMatrix.rw.xyz - mMatrixOrigin;
                        int componentInfoIndex = (control - Control::MoveX) * 3;
                        ImFormatString(tmps, sizeof(tmps), translationInfoMask[control - Control::MoveX].data(), deltaInfo[translationInfoIndex[componentInfoIndex]], deltaInfo[translationInfoIndex[componentInfoIndex + 1]], deltaInfo[translationInfoIndex[componentInfoIndex + 2]]);
                        drawList->AddText(ImVec2(destinationPosOnScreen.x + 15, destinationPosOnScreen.y + 15), 0xFF000000, tmps);
                        drawList->AddText(ImVec2(destinationPosOnScreen.x + 14, destinationPosOnScreen.y + 14), 0xFFFFFFFF, tmps);
                    }
                }

                int GetScaleType()
                {
                    ImGuiIO &imGuiIO = ImGui::GetIO();
                    int control = Control::None;

                    // screen
                    if (imGuiIO.MousePos.x >= mScreenSquareMin.x && imGuiIO.MousePos.x <= mScreenSquareMax.x &&
                        imGuiIO.MousePos.y >= mScreenSquareMin.y && imGuiIO.MousePos.y <= mScreenSquareMax.y)
                    {
                        control = Control::ScaleXYZ;
                    }

                    const Math::Float3 direction[3] =
                    {
                        modelMatrix.rx.xyz,
                        modelMatrix.ry.xyz,
                        modelMatrix.rz.xyz,
                    };

                    // compute
                    for (uint32_t index = 0; index < 3 && control == Control::None; index++)
                    {
                        Math::Float3 dirPlaneX, dirPlaneY;
                        bool belowAxisLimit, belowPlaneLimit;
                        ComputeTripodAxisAndVisibility(index, dirPlaneX, dirPlaneY, belowAxisLimit, belowPlaneLimit);
                        dirPlaneX = modelMatrix.rotate(dirPlaneX);
                        dirPlaneY = modelMatrix.rotate(dirPlaneY);

                        const int planeNormal = (index + 2) % 3;
                        const float len = IntersectRayPlane(mRayOrigin, mRayVector, Shapes::Plane(direction[planeNormal], modelMatrix.rw.xyz));
                        Math::Float3 posOnPlane = mRayOrigin + mRayVector * len;

                        const float dx = dirPlaneX.dot((posOnPlane - modelMatrix.rw.xyz) * (1.0f / mScreenFactor));
                        const float dy = dirPlaneY.dot((posOnPlane - modelMatrix.rw.xyz) * (1.0f / mScreenFactor));
                        if (belowAxisLimit && dy > -0.1f && dy < 0.1f && dx > 0.1f  && dx < 1.0f)
                        {
                            control = Control::ScaleX + index;
                        }
                    }

                    return control;
                }

                int GetRotateType()
                {
                    ImGuiIO &imGuiIO = ImGui::GetIO();
                    int control = Control::None;

                    Math::Float3 deltaScreen(imGuiIO.MousePos.x - mScreenSquareCenter.x, imGuiIO.MousePos.y - mScreenSquareCenter.y, 0.0f);
                    float dist = deltaScreen.getLength();
                    if (dist >= (screenRotateSize - 0.005f) * viewPort.size.height && dist < (screenRotateSize + 0.005f) * viewPort.size.height)
                    {
                        control = Control::RotateScreen;
                    }

                    const Math::Float3 planeNormals[] =
                    {
                        modelMatrix.rx.xyz,
                        modelMatrix.ry.xyz,
                        modelMatrix.rz.xyz,
                    };

                    for (uint32_t index = 0; index < 3 && control == Control::None; index++)
                    {
                        // pickup plane
                        Shapes::Plane pickupPlane(planeNormals[index], modelMatrix.rw.xyz);
                        const float len = IntersectRayPlane(mRayOrigin, mRayVector, pickupPlane);
                        Math::Float3 localPos = mRayOrigin + mRayVector * len - modelMatrix.rw.xyz;
                        if (localPos.getNormal().dot(mRayVector) > FLT_EPSILON)
                        {
                            continue;
                        }

                        float distance = localPos.getLength() / mScreenFactor;
                        if (distance > 0.85f && distance < 1.15f)
                        {
                            control = Control::RotateX + index;
                        }
                    }

                    return control;
                }

                int GetMoveType(Math::Float3 *gizmoHitProportion)
                {
                    ImGuiIO &imGuiIO = ImGui::GetIO();
                    int control = Control::None;

                    // screen
                    if (imGuiIO.MousePos.x >= mScreenSquareMin.x && imGuiIO.MousePos.x <= mScreenSquareMax.x &&
                        imGuiIO.MousePos.y >= mScreenSquareMin.y && imGuiIO.MousePos.y <= mScreenSquareMax.y)
                    {
                        control = Control::MoveScreen;
                    }

                    const Math::Float3 direction[3] =
                    {
                        modelMatrix.rx.xyz,
                        modelMatrix.ry.xyz,
                        modelMatrix.rz.xyz,
                    };

                    // compute
                    for (uint32_t index = 0; index < 3 && control == Control::None; index++)
                    {
                        Math::Float3 dirPlaneX, dirPlaneY;
                        bool belowAxisLimit, belowPlaneLimit;
                        ComputeTripodAxisAndVisibility(index, dirPlaneX, dirPlaneY, belowAxisLimit, belowPlaneLimit);
                        dirPlaneX = modelMatrix.rotate(dirPlaneX);
                        dirPlaneY = modelMatrix.rotate(dirPlaneY);

                        const int planeNormal = (index + 2) % 3;
                        const float len = IntersectRayPlane(mRayOrigin, mRayVector, Shapes::Plane(direction[planeNormal], modelMatrix.rw.xyz));
                        Math::Float3 posOnPlane = mRayOrigin + mRayVector * len;

                        const float dx = dirPlaneX.dot((posOnPlane - modelMatrix.rw.xyz) * (1.0f / mScreenFactor));
                        const float dy = dirPlaneY.dot((posOnPlane - modelMatrix.rw.xyz) * (1.0f / mScreenFactor));
                        if (belowAxisLimit && dy > -0.1f && dy < 0.1f && dx > 0.1f  && dx < 1.0f)
                        {
                            control = Control::MoveX + index;
                        }

                        if (belowPlaneLimit && dx >= quadCoords[0] && dx <= quadCoords[4] && dy >= quadCoords[1] && dy <= quadCoords[3])
                        {
                            control = Control::MoveXY + index;
                        }

                        if (gizmoHitProportion)
                        {
                            *gizmoHitProportion = Math::Float3(dx, dy, 0.0f);
                        }
                    }

                    return control;
                }

                void HandleTranslation(Math::Float4x4 &matrix, float *snap)
                {
                    ImGuiIO &imGuiIO = ImGui::GetIO();
                    bool applyRotationLocaly = currentAlignment == Alignment::Local;

                    // move
                    int control = Control::None;
                    if (isUsing)
                    {
                        const float len = IntersectRayPlane(mRayOrigin, mRayVector, mTranslationPlane);
                        Math::Float3 newPos = mRayOrigin + mRayVector * len;

                        // compute delta
                        Math::Float3 newOrigin = newPos - mRelativeOrigin * mScreenFactor;
                        Math::Float3 delta = newOrigin - modelMatrix.rw.xyz;

                        // 1 axis constraint
                        if (currentControl >= Control::MoveX && currentControl <= Control::MoveZ)
                        {
                            int axisIndex = currentControl - Control::MoveX;
                            Math::Float3 const &axisValue = modelMatrix.rows[axisIndex].xyz;
                            float lengthOnAxis = axisValue.dot(delta);
                            delta = axisValue * lengthOnAxis;
                        }

                        // snap
                        if (snap)
                        {
                            Math::Float3 cumulativeDelta = modelMatrix.rw.xyz + delta - mMatrixOrigin;
                            if (applyRotationLocaly)
                            {
                                Math::Float4x4 modelSourceNormalized = sourceModelMatrix;
                                modelSourceNormalized.orthonormalize();
                                Math::Float4x4 modelSourceNormalizedInverse = modelSourceNormalized.getInverse();
                                cumulativeDelta = modelSourceNormalizedInverse.rotate(cumulativeDelta);
                                ComputeSnap(cumulativeDelta, snap);
                                cumulativeDelta = modelSourceNormalized.rotate(cumulativeDelta);
                            }
                            else
                            {
                                ComputeSnap(cumulativeDelta, snap);
                            }

                            delta = mMatrixOrigin + cumulativeDelta - modelMatrix.rw.xyz;
                        }

                        // compute matrix & delta
                        Math::Float4x4 deltaMatrixTranslation = Math::Float4x4::MakeTranslation(delta);
                        matrix = sourceModelMatrix * deltaMatrixTranslation;
                        if (!imGuiIO.MouseDown[0])
                        {
                            isUsing = false;
                        }

                        control = currentControl;
                    }
                    else
                    {
                        // find new possible way to move
                        Math::Float3 gizmoHitProportion;
                        control = GetMoveType(&gizmoHitProportion);
                        isOver = control != Control::None;
                        if (imGuiIO.MouseDown[0] && control != Control::None)
                        {
                            isUsing = true;
                            currentControl = control;
                            const Math::Float3 movePlaneNormal[] =
                            {
                                modelMatrix.ry.xyz,
                                modelMatrix.rz.xyz,
                                modelMatrix.rx.xyz,
                                modelMatrix.rz.xyz,
                                modelMatrix.rx.xyz,
                                modelMatrix.ry.xyz,
                                -mCameraDir,
                            };

                            // pickup plane
                            mTranslationPlane = Shapes::Plane(movePlaneNormal[control - Control::MoveX], modelMatrix.rw.xyz);
                            const float len = IntersectRayPlane(mRayOrigin, mRayVector, mTranslationPlane);
                            mTranslationPlaneOrigin = mRayOrigin + mRayVector * len;
                            mMatrixOrigin = modelMatrix.rw.xyz;

                            mRelativeOrigin = (mTranslationPlaneOrigin - modelMatrix.rw.xyz) * (1.0f / mScreenFactor);
                        }
                    }

                    DrawTranslationGizmo(control);
                }

                void HandleScale(Math::Float4x4 &matrix, float *snap)
                {
                    ImGuiIO &imGuiIO = ImGui::GetIO();

                    int control = Control::None;
                    if (!isUsing)
                    {
                        // find new possible way to scale
                        control = GetScaleType();
                        isOver = control != Control::None;
                        if (imGuiIO.MouseDown[0] && control != Control::None)
                        {
                            isUsing = true;
                            currentControl = control;
                            const Math::Float3 movePlaneNormal[] =
                            {
                                modelMatrix.ry.xyz,
                                modelMatrix.rz.xyz,
                                modelMatrix.rx.xyz,
                                modelMatrix.rz.xyz,
                                modelMatrix.ry.xyz,
                                modelMatrix.rx.xyz,
                                -mCameraDir,
                            };

                            // pickup plane
                            mTranslationPlane = Shapes::Plane(movePlaneNormal[control - Control::ScaleX], modelMatrix.rw.xyz);
                            const float len = IntersectRayPlane(mRayOrigin, mRayVector, mTranslationPlane);
                            mTranslationPlaneOrigin = mRayOrigin + mRayVector * len;
                            mMatrixOrigin = modelMatrix.rw.xyz;
                            mScale.set(1.0f, 1.0f, 1.0f);
                            mRelativeOrigin = (mTranslationPlaneOrigin - modelMatrix.rw.xyz) * (1.0f / mScreenFactor);
                            mScaleValueOrigin = Math::Float3(sourceModelMatrix.rx.getLength(), sourceModelMatrix.ry.getLength(), sourceModelMatrix.rz.getLength());
                            mSaveMousePosx = imGuiIO.MousePos.x;
                        }
                    }

                    // scale
                    if (isUsing)
                    {
                        const float len = IntersectRayPlane(mRayOrigin, mRayVector, mTranslationPlane);
                        Math::Float3 newPos = mRayOrigin + mRayVector * len;
                        Math::Float3 newOrigin = newPos - mRelativeOrigin * mScreenFactor;
                        Math::Float3 delta = newOrigin - modelMatrix.rw.xyz;

                        // 1 axis constraint
                        if (currentControl >= Control::ScaleX && currentControl <= Control::ScaleZ)
                        {
                            int axisIndex = currentControl - Control::ScaleX;
                            Math::Float3 const &axisValue = *(Math::Float3*)&modelMatrix.table[axisIndex];
                            float lengthOnAxis = axisValue.dot(delta);
                            delta = axisValue * lengthOnAxis;

                            Math::Float3 baseVector = mTranslationPlaneOrigin - modelMatrix.rw.xyz;
                            float ratio = axisValue.dot(baseVector + delta) / axisValue.dot(baseVector);

                            mScale[axisIndex] = std::max(ratio, 0.001f);
                        }
                        else
                        {
                            float scaleDelta = (imGuiIO.MousePos.x - mSaveMousePosx)  * 0.01f;
                            mScale.set(std::max(1.0f + scaleDelta, 0.001f));
                        }

                        // snap
                        if (snap)
                        {
                            float scaleSnap[] = { snap[0], snap[0], snap[0] };
                            ComputeSnap(mScale, scaleSnap);
                        }

                        // no 0 allowed
                        for (int index = 0; index < 3; index++)
                        {
                            mScale[index] = std::max(mScale[index], 0.001f);
                        }

                        // compute matrix & delta
                        Math::Float4x4 deltaMatrixScale = Math::Float4x4::MakeScaling(mScale * mScaleValueOrigin);
                        matrix = deltaMatrixScale * modelMatrix;
                        if (!imGuiIO.MouseDown[0])
                        {
                            isUsing = false;
                        }

                        control = currentControl;
                    }

                    DrawScaleGizmo(control);
                }

                void HandleRotation(Math::Float4x4 &matrix, float *snap)
                {
                    ImGuiIO &imGuiIO = ImGui::GetIO();
                    bool applyRotationLocaly = currentAlignment == Alignment::Local;

                    int control = Control::None;
                    if (!isUsing)
                    {
                        control = GetRotateType();
                        isOver = control != Control::None;
                        if (control == Control::RotateScreen)
                        {
                            applyRotationLocaly = true;
                        }

                        if (imGuiIO.MouseDown[0] && control != Control::None)
                        {
                            isUsing = true;
                            currentControl = control;
                            const Math::Float3 rotatePlaneNormal[] =
                            {
                                modelMatrix.rx.xyz,
                                modelMatrix.ry.xyz,
                                modelMatrix.rz.xyz,
                                -mCameraDir,
                            };

                            // pickup plane
                            if (applyRotationLocaly)
                            {
                                mTranslationPlane = Shapes::Plane(rotatePlaneNormal[control - Control::RotateX], modelMatrix.rw.xyz);
                            }
                            else
                            {
                                mTranslationPlane = Shapes::Plane(directionUnary[control - Control::RotateX], sourceModelMatrix.rw.xyz);
                            }

                            const float len = IntersectRayPlane(mRayOrigin, mRayVector, mTranslationPlane);
                            Math::Float3 localPos = mRayOrigin + mRayVector * len - modelMatrix.rw.xyz;
                            mRotationVectorSource = localPos.getNormal();
                            mRotationAngleOrigin = ComputeAngleOnPlane();
                        }
                    }

                    // rotation
                    if (isUsing)
                    {
                        mRotationAngle = ComputeAngleOnPlane();
                        if (snap)
                        {
                            float snapInRadian = Math::DegreesToRadians(snap[0]);
                            ComputeSnap(&mRotationAngle, snapInRadian);
                        }

                        Math::Float3 rotationAxisLocalSpace = inverseModelMatrix.rotate(mTranslationPlane.normal);
                        rotationAxisLocalSpace.normalize();

                        Math::Float4x4 deltaRotation = Math::Float4x4::MakeAngularRotation(rotationAxisLocalSpace, mRotationAngle - mRotationAngleOrigin);
                        mRotationAngleOrigin = mRotationAngle;

                        Math::Float4x4 scaleOrigin = Math::Float4x4::MakeScaling(mModelScaleOrigin);
                        if (applyRotationLocaly)
                        {
                            matrix = scaleOrigin * deltaRotation * modelMatrix;
                        }
                        else
                        {
                            Math::Float4x4 res = sourceModelMatrix;
                            res.rw.xyz = Math::Float3::Zero;

                            matrix = res * deltaRotation;
                            matrix.rw = sourceModelMatrix.rw;
                        }

                        if (!imGuiIO.MouseDown[0])
                        {
                            isUsing = false;
                        }

                        control = currentControl;
                    }

                    DrawRotationGizmo(control);
                }

                void HandleBounds(Shapes::AlignedBox &bounds, Math::Float4x4 &matrix, float *snapValues, LockAxis lockAxis)
                {
                    ImGuiIO &imGuiIO = ImGui::GetIO();
                    ImDrawList* drawList = currentDrawList;

                    // compute best projection axis
                    Math::Float3 bestAxisWorldDirection;
                    int bestAxis = mBoundsBestAxis;
                    if (!isUsing)
                    {
                        if (lockAxis == LockAxis::Automatic)
                        {
                            float bestDot = 0.0f;
                            for (uint32_t index = 0; index < 3; index++)
                            {
                                Math::Float3 dirPlaneNormalWorld = sourceModelMatrix.rotate(directionUnary[index]);
                                dirPlaneNormalWorld.normalize();

                                float dt = (mCameraEye - sourceModelMatrix.rw.xyz).getNormal().dot(dirPlaneNormalWorld);
                                if (std::abs(dt) >= bestDot)
                                {
                                    bestDot = std::abs(dt);
                                    bestAxis = index;
                                    bestAxisWorldDirection = dirPlaneNormalWorld;
                                }
                            }
                        }
                        else
                        {
                            switch (lockAxis)
                            {
                            case LockAxis::X:
                                bestAxis = 0;
                                break;

                            case LockAxis::Y:
                                bestAxis = 1;
                                break;

                            case LockAxis::Z:
                                bestAxis = 2;
                                break;
                            };

                            bestAxisWorldDirection = sourceModelMatrix.rotate(directionUnary[bestAxis]);
                            bestAxisWorldDirection.normalize();
                        }
                    }

                    // corners
                    const uint32_t anchorAlpha = isEnabled ? 0xFF000000 : 0x80000000;
                    const uint32_t anchorColors[3] =
                    {
                        0xFF0000 + anchorAlpha,
                        0x00FF00 + anchorAlpha,
                        0x0000FF + anchorAlpha,
                    };

                    const int secondAxis = (bestAxis + 1) % 3;
                    const int thirdAxis = (bestAxis + 2) % 3;
                    const uint32_t axisColors[4] =
                    {
                        anchorColors[secondAxis],
                        anchorColors[thirdAxis],
                        anchorColors[secondAxis],
                        anchorColors[thirdAxis],
                    };

                    Math::Float3 aabb[4];
                    float *boundsData = (float *)bounds.minimum.data;
                    for (int index = 0; index < 4; index++)
                    {
                        aabb[index][bestAxis] = 0.0f;
                        aabb[index][secondAxis] = boundsData[secondAxis + 3 * (index >> 1)];
                        aabb[index][thirdAxis] = boundsData[thirdAxis + 3 * ((index >> 1) ^ (index & 1))];
                    }

                    static const Shapes::Plane cameraPlane(Math::Float3(0.0f, 0.0f, 1.0f), 0.0f);

                    // draw bounds
                    const Math::Float4x4 boundsMVP = sourceModelMatrix * viewProjectionMatrix;
                    for (int index = 0; index < 4; index++)
                    {
                        auto clipBound1 = boundsMVP.transform(Math::Float4(aabb[index], 1.0f));
                        auto clipBound2 = boundsMVP.transform(Math::Float4(aabb[(index + 1) % 4], 1.0f));

                        auto worldBound1 = getPointFromClipPosition(clipBound1);
                        auto worldBound2 = getPointFromClipPosition(clipBound2);
                        //float boundDistance = std::sqrt(ImLengthSqr(worldBound1 - worldBound2));
                        float boundDistance = aabb[index].getDistance(aabb[(index + 1) % 4]);
                        int stepCount = (int)(boundDistance * 3.0f) + 1;
                        float stepLength = 1.0f / (float)stepCount;
                        for (int step = 0; step < stepCount; step++)
                        {
                            float t1 = (float)step * stepLength;
                            float t2 = (float)step * stepLength + stepLength * 0.5f;
                            ImVec2 worldBoundSS1 = Math::Interpolate(worldBound1, worldBound2, t1);
                            ImVec2 worldBoundSS2 = Math::Interpolate(worldBound1, worldBound2, t2);

                            uint32_t axisColor = axisColors[index];
                            drawList->AddLine(worldBoundSS1, worldBoundSS2, axisColor, 3.0f);
                        }
                    }

                    isOver = false;
                    for (int index = 0; index < 4; index++)
                    {
                        uint32_t axisColor = axisColors[index];
                        uint32_t otherColor = axisColors[(index + 1) % 4];

                        ImVec2 worldBound1 = getPointFromPosition(aabb[index], boundsMVP);
                        ImVec2 worldBound2 = getPointFromPosition(aabb[(index + 1) % 4], boundsMVP);
                        Math::Float3 midPoint = (aabb[index] + aabb[(index + 1) % 4]) * 0.5f;
                        ImVec2 midBound = getPointFromPosition(midPoint, boundsMVP);

						static constexpr float AnchorBigRadius = 10.0f;
						static constexpr float AnchorSmallRadius = 8.0f;
                        bool overBigAnchor = ImLengthSqr(worldBound1 - imGuiIO.MousePos) <= (AnchorBigRadius*AnchorBigRadius);
                        bool overSmallAnchor = ImLengthSqr(midBound - imGuiIO.MousePos) <= (AnchorBigRadius*AnchorBigRadius);
                        uint32_t bigAnchorColor = overBigAnchor ? selectionColor : axisColor;
                        uint32_t otherAnchorColor = overBigAnchor ? selectionColor : otherColor;
                        uint32_t smallAnchorColor = overSmallAnchor ? selectionColor : axisColor;
                        if (index % 2)
                        {
                            drawList->AddCircleFilled(worldBound1, AnchorBigRadius, bigAnchorColor);
                            drawList->AddCircleFilled(worldBound1, AnchorSmallRadius - 2, otherAnchorColor);
                        }
                        else
                        {
                            drawList->AddCircleFilled(worldBound1, AnchorBigRadius, otherAnchorColor);
                            drawList->AddCircleFilled(worldBound1, AnchorSmallRadius - 2, bigAnchorColor);
                        }

                        drawList->AddCircleFilled(midBound, AnchorSmallRadius, smallAnchorColor);

                        isOver |= overBigAnchor | overSmallAnchor;

                        // big anchor on corners
                        int oppositeIndex = (index + 2) % 4;
                        if (!isUsing && isEnabled && overBigAnchor && imGuiIO.MouseDown[0])
                        {
                            mBoundsPivot = sourceModelMatrix.transform(aabb[(index + 2) % 4]);
                            mBoundsAnchor = sourceModelMatrix.transform(aabb[index]);
                            mBoundsPlane = Shapes::Plane(bestAxisWorldDirection, mBoundsAnchor);
                            mBoundsBestAxis = bestAxis;
                            mBoundsAxis[0] = secondAxis;
                            mBoundsAxis[1] = thirdAxis;

                            mBoundsLocalPivot.set(0.0f);
                            mBoundsLocalPivot[secondAxis] = aabb[oppositeIndex][secondAxis];
                            mBoundsLocalPivot[thirdAxis] = aabb[oppositeIndex][thirdAxis];

                            isUsing = true;
                            mBoundsMatrix = sourceModelMatrix;
                        }

                        // small anchor on middle of segment
                        if (!isUsing && isEnabled && overSmallAnchor && imGuiIO.MouseDown[0])
                        {
                            Math::Float3 midPointOpposite = (aabb[(index + 2) % 4] + aabb[(index + 3) % 4]) * 0.5f;
                            mBoundsPivot = sourceModelMatrix.transform(midPointOpposite);
                            mBoundsAnchor = sourceModelMatrix.transform(midPoint);
                            mBoundsPlane = Shapes::Plane(bestAxisWorldDirection, mBoundsAnchor);
                            mBoundsBestAxis = bestAxis;

                            int indices[] =
                            {
                                secondAxis,
                                thirdAxis
                            };

                            mBoundsAxis[0] = indices[index % 2];
                            mBoundsAxis[1] = -1;

                            mBoundsLocalPivot.set(0.0f);
                            mBoundsLocalPivot[mBoundsAxis[0]] = aabb[oppositeIndex][indices[index % 2]];

                            isUsing = true;
                            mBoundsMatrix = sourceModelMatrix;
                        }
                    }

                    if (isUsing)
                    {
                        Math::Float4x4 scale = Math::Float4x4::Identity;

                        // compute projected mouse position on plane
                        const float len = IntersectRayPlane(mRayOrigin, mRayVector, mBoundsPlane);
                        Math::Float3 newPos = mRayOrigin + mRayVector * len;

                        // compute a reference and delta vectors base on mouse move
                        Math::Float3 deltaVector = (newPos - mBoundsPivot).getAbsolute();
                        Math::Float3 referenceVector = (mBoundsAnchor - mBoundsPivot).getAbsolute();

                        // for 1 or 2 axes, compute a ratio that's used for scale and snap it based on resulting length
                        for (int index = 0; index < 2; index++)
                        {
                            int axisIndex = mBoundsAxis[index];
                            if (axisIndex == -1)
                            {
                                continue;
                            }

                            float ratioAxis = 1.0f;
                            Math::Float3 axisDir = mBoundsMatrix[axisIndex].xyz.getAbsolute();

                            float dtAxis = axisDir.dot(referenceVector);
                            float boundSize = boundsData[axisIndex + 3] - boundsData[axisIndex];
                            if (dtAxis > FLT_EPSILON)
                            {
                                ratioAxis = axisDir.dot(deltaVector) / dtAxis;
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

                            scale.rows[axisIndex] *= ratioAxis;
                        }

                        // transform matrix
                        Math::Float4x4 preScale, postScale;
                        preScale = Math::Float4x4::MakeTranslation(-mBoundsLocalPivot);
                        postScale = Math::Float4x4::MakeTranslation(mBoundsLocalPivot);
                        matrix = preScale * scale * postScale * mBoundsMatrix;

                        // info text
                        char tmps[512];
                        ImVec2 destinationPosOnScreen = getPointFromPosition(modelMatrix.rw.xyz, viewProjectionMatrix);
                        ImFormatString(tmps, sizeof(tmps), "X: %.2f Y: %.2f Z:%.2f"
                            , (boundsData[3] - boundsData[0]) * mBoundsMatrix.rows[0].getLength() * scale.rows[0].getLength()
                            , (boundsData[4] - boundsData[1]) * mBoundsMatrix.rows[1].getLength() * scale.rows[1].getLength()
                            , (boundsData[5] - boundsData[2]) * mBoundsMatrix.rows[2].getLength() * scale.rows[2].getLength()
                        );

                        drawList->AddText(ImVec2(destinationPosOnScreen.x + 15, destinationPosOnScreen.y + 15), 0xFF000000, tmps);
                        drawList->AddText(ImVec2(destinationPosOnScreen.x + 14, destinationPosOnScreen.y + 14), 0xFFFFFFFF, tmps);
                    }

                    if (!imGuiIO.MouseDown[0])
                    {
                        isUsing = false;
                    }
                }

                void manipulate(Operation operation, Alignment alignment, Math::Float4x4 &matrix, float *snap, Shapes::AlignedBox *localBounds, LockAxis lockAxis)
                {
                    computeContext(matrix, alignment);

                    // behind camera
                    Math::Float3 camSpacePosition = modelViewProjectionMatrix.transform(Math::Float3(0.0f, 0.0f, 0.0f));
                    if (camSpacePosition.z < 0.001f)
                    {
                        return;
                    }

                    // -- 
                    if (isEnabled)
                    {
                        switch (operation)
                        {
                        case Operation::Rotate:
                            HandleRotation(matrix, snap);
                            break;

                        case Operation::Translate:
                            HandleTranslation(matrix, snap);
                            break;

                        case Operation::Scale:
                            HandleScale(matrix, snap);
                            break;

                        case Operation::Bounds:
                            if (localBounds)
                            {
                                HandleBounds(*localBounds, matrix, snap, lockAxis);
                            }

                            break;
                        };
                    }

                    if (isUsing || isOver)
                    {
                        ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);
                    }
                }

                void drawCube(Math::Float4x4 const &matrix)
                {
                    computeContext(matrix, Alignment::Local);
                    Math::Float4x4 viewInverse = viewMatrix.getInverse();
                    for (int iFace = 0; iFace < 6; iFace++)
                    {
                        const int normalIndex = (iFace % 3);
                        const int perpXIndex = (normalIndex + 1) % 3;
                        const int perpYIndex = (normalIndex + 2) % 3;
                        const float invert = (iFace > 2) ? -1.0f : 1.0f;

                        const Math::Float3 faceCoords[4] =
                        {
                            directionUnary[normalIndex] + directionUnary[perpXIndex] + directionUnary[perpYIndex],
                            directionUnary[normalIndex] + directionUnary[perpXIndex] - directionUnary[perpYIndex],
                            directionUnary[normalIndex] - directionUnary[perpXIndex] - directionUnary[perpYIndex],
                            directionUnary[normalIndex] - directionUnary[perpXIndex] + directionUnary[perpYIndex],
                        };

                        // clipping
                        bool skipFace = false;
                        for (uint32_t iCoord = 0; iCoord < 4; iCoord++)
                        {
                            Math::Float3 camSpacePosition = modelViewProjectionMatrix.transform(faceCoords[iCoord] * 0.5f * invert);
                            if (camSpacePosition.z < 0.001f)
                            {
                                skipFace = true;
                                break;
                            }
                        }

                        if (skipFace)
                        {
                            continue;
                        }

                        // 3D->2D
                        ImVec2 faceCoordsScreen[4];
                        for (uint32_t iCoord = 0; iCoord < 4; iCoord++)
                        {
                            faceCoordsScreen[iCoord] = getPointFromPosition(faceCoords[iCoord] * 0.5f * invert, modelViewProjectionMatrix);
                        }

                        // back face culling 
                        Math::Float3 cullPos, cullNormal;
                        cullPos = matrix.transform(faceCoords[0] * 0.5f * invert);
                        cullNormal = matrix.rotate(directionUnary[normalIndex] * invert);
                        float dt = (cullPos - viewInverse.rw.xyz).getNormal().dot(cullNormal.getNormal());
                        if (dt > 0.0f)
                        {
                            continue;
                        }

                        // draw face with lighter color
                        currentDrawList->AddConvexPolyFilled(faceCoordsScreen, 4, directionColor[normalIndex] | 0x808080);
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

            bool WorkSpace::isUsing(void)
            {
                return context->isUsing;
            }

            void WorkSpace::beginFrame(Math::Float4x4 const &view, Math::Float4x4 const &projection, float x, float y, float width, float height)
            {
                context->beginFrame(view, projection, x, y, width, height);
            }

            void WorkSpace::manipulate(Operation operation, Alignment alignment, Math::Float4x4 &matrix, float *snap, Shapes::AlignedBox *localBounds, LockAxis lockAxis)
            {
                context->manipulate(operation, alignment, matrix, snap, localBounds, lockAxis);
            }

            void WorkSpace::drawCube(Math::Float4x4 const &matrix)
            {
                context->drawCube(matrix);
            }
        }; // Gizmo
    }; // namespace UI
}; // namespace Gek