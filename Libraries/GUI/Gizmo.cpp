#include "GEK/GUI/Gizmo.hpp"
#include "GEK/Shapes/Plane.hpp"

namespace Gek
{
    namespace UI
    {
        namespace Gizmo
        {
            const float screenRotateSize = 0.1f;

            static const float angleLimit = 0.96f;
            static const float planeLimit = 0.2f;

            void FPU_MatrixF_x_MatrixF(const float *a, const float *b, float *r)
            {
                r[0] = a[0] * b[0] + a[1] * b[4] + a[2] * b[8] + a[3] * b[12];
                r[1] = a[0] * b[1] + a[1] * b[5] + a[2] * b[9] + a[3] * b[13];
                r[2] = a[0] * b[2] + a[1] * b[6] + a[2] * b[10] + a[3] * b[14];
                r[3] = a[0] * b[3] + a[1] * b[7] + a[2] * b[11] + a[3] * b[15];

                r[4] = a[4] * b[0] + a[5] * b[4] + a[6] * b[8] + a[7] * b[12];
                r[5] = a[4] * b[1] + a[5] * b[5] + a[6] * b[9] + a[7] * b[13];
                r[6] = a[4] * b[2] + a[5] * b[6] + a[6] * b[10] + a[7] * b[14];
                r[7] = a[4] * b[3] + a[5] * b[7] + a[6] * b[11] + a[7] * b[15];

                r[8] = a[8] * b[0] + a[9] * b[4] + a[10] * b[8] + a[11] * b[12];
                r[9] = a[8] * b[1] + a[9] * b[5] + a[10] * b[9] + a[11] * b[13];
                r[10] = a[8] * b[2] + a[9] * b[6] + a[10] * b[10] + a[11] * b[14];
                r[11] = a[8] * b[3] + a[9] * b[7] + a[10] * b[11] + a[11] * b[15];

                r[12] = a[12] * b[0] + a[13] * b[4] + a[14] * b[8] + a[15] * b[12];
                r[13] = a[12] * b[1] + a[13] * b[5] + a[14] * b[9] + a[15] * b[13];
                r[14] = a[12] * b[2] + a[13] * b[6] + a[14] * b[10] + a[15] * b[14];
                r[15] = a[12] * b[3] + a[13] * b[7] + a[14] * b[11] + a[15] * b[15];
            }

            struct matrix_t
            {
            public:
                union
                {
                    float table[4][4];
                    float data[16];
                    struct
                    {
                        Math::Float4 rx, ry, rz, rw;
                    };

                    Math::Float4 rows[4];
                };

                matrix_t(const matrix_t& other)
                {
                    memcpy(data, other.data, sizeof(matrix_t));
                }

                matrix_t()
                {
                }

                operator float * ()
                {
                    return data;
                }

                operator const float* () const
                {
                    return data;
                }

                Math::Float4 &operator [] (size_t index)
                {
                    return rows[index];
                }

                Math::Float4 const &operator [] (size_t index) const
                {
                    return rows[index];
                }

                matrix_t &operator = (matrix_t const &matrix)
                {
                    memcpy(data, matrix.data, sizeof(matrix_t));
                    return (*this);
                }

                void MakeTranslation(const Math::Float3& vt)
                {
                    rx.set(1.0f, 0.0f, 0.0f, 0.0f);
                    ry.set(0.0f, 1.0f, 0.0f, 0.0f);
                    rz.set(0.0f, 0.0f, 1.0f, 0.0f);
                    rw.set(vt.x, vt.y, vt.z, 1.0f);
                }

                void MakeScaling(const Math::Float3& s)
                {
                    rx.set(s.x, 0.0f, 0.0f, 0.0f);
                    ry.set(0.0f, s.y, 0.0f, 0.0f);
                    rz.set(0.0f, 0.0f, s.z, 0.0f);
                    rw.set(0.0f, 0.0f, 0.0f, 1.0f);
                }

                void MakeRotation(const Math::Float3 & axis, float angle);

                matrix_t operator * (const matrix_t &matrix) const
                {
                    matrix_t tmp;
                    FPU_MatrixF_x_MatrixF((float*)this, (float*)&matrix, (float*)&tmp);
                    return tmp;
                }

                matrix_t &operator *= (const matrix_t &matrix)
                {
                    matrix_t tmp = *this;
                    FPU_MatrixF_x_MatrixF((float*)&tmp, (float*)&matrix, (float*)this);
                    return (*this);
                }

                float getDeterminant() const
                {
                    return
                        table[0][0] * table[1][1] * table[2][2] + 
                        table[0][1] * table[1][2] * table[2][0] + 
                        table[0][2] * table[1][0] * table[2][1] -
                        table[0][2] * table[1][1] * table[2][0] -
                        table[0][1] * table[1][0] * table[2][2] -
                        table[0][0] * table[1][2] * table[2][1];
                }

                void getInverse(const matrix_t &srcMatrix);

                void SetToIdentity()
                {
                    rx.set(1.0f, 0.0f, 0.0f, 0.0f);
                    ry.set(0.0f, 1.0f, 0.0f, 0.0f);
                    rz.set(0.0f, 0.0f, 1.0f, 0.0f);
                    rw.set(0.0f, 0.0f, 0.0f, 1.0f);
                }

                void transpose()
                {
                    matrix_t tmpm;
                    for (int l = 0; l < 4; l++)
                    {
                        for (int c = 0; c < 4; c++)
                        {
                            tmpm.table[l][c] = table[c][l];
                        }
                    }

                    (*this) = tmpm;
                }

                void orthonormalize()
                {
                    rx.normalize();
                    ry.normalize();
                    rz.normalize();
                }

                Math::Float4 transform(const Math::Float4& vector) const;
                Math::Float3 transform(const Math::Float3& vector) const;
                Math::Float3 rotate(const Math::Float3& vector) const;
            };

            Math::Float4 matrix_t::transform(const Math::Float4& vector) const
            {
                Math::Float4 out;
                out.x = vector.x * table[0][0] + vector.y * table[1][0] + vector.z * table[2][0] + vector.w * table[3][0];
                out.y = vector.x * table[0][1] + vector.y * table[1][1] + vector.z * table[2][1] + vector.w * table[3][1];
                out.z = vector.x * table[0][2] + vector.y * table[1][2] + vector.z * table[2][2] + vector.w * table[3][2];
                out.w = vector.x * table[0][3] + vector.y * table[1][3] + vector.z * table[2][3] + vector.w * table[3][3];
                return out;
            }

            Math::Float3 matrix_t::transform(const Math::Float3& vector) const
            {
                Math::Float3 out;
                out.x = vector.x * table[0][0] + vector.y * table[1][0] + vector.z * table[2][0] + table[3][0];
                out.y = vector.x * table[0][1] + vector.y * table[1][1] + vector.z * table[2][1] + table[3][1];
                out.z = vector.x * table[0][2] + vector.y * table[1][2] + vector.z * table[2][2] + table[3][2];
                return out;
            }

            Math::Float3 matrix_t::rotate(const Math::Float3& vector) const
            {
                Math::Float3 out;
                out.x = vector.x * table[0][0] + vector.y * table[1][0] + vector.z * table[2][0];
                out.y = vector.x * table[0][1] + vector.y * table[1][1] + vector.z * table[2][1];
                out.z = vector.x * table[0][2] + vector.y * table[1][2] + vector.z * table[2][2];
                return out;
            }

            void matrix_t::getInverse(const matrix_t &srcMatrix)
            {
                // transpose matrix
                float src[16];
                for (int index = 0; index < 4; ++index)
                {
                    src[index] = srcMatrix.data[index * 4];
                    src[index + 4] = srcMatrix.data[index * 4 + 1];
                    src[index + 8] = srcMatrix.data[index * 4 + 2];
                    src[index + 12] = srcMatrix.data[index * 4 + 3];
                }

                // calculate pairs for first 8 elements (cofactors)
                float tmp[12]; // temp array for pairs
                tmp[0] = src[10] * src[15];
                tmp[1] = src[11] * src[14];
                tmp[2] = src[9] * src[15];
                tmp[3] = src[11] * src[13];
                tmp[4] = src[9] * src[14];
                tmp[5] = src[10] * src[13];
                tmp[6] = src[8] * src[15];
                tmp[7] = src[11] * src[12];
                tmp[8] = src[8] * src[14];
                tmp[9] = src[10] * src[12];
                tmp[10] = src[8] * src[13];
                tmp[11] = src[9] * src[12];

                // calculate first 8 elements (cofactors)
                data[0] = (tmp[0] * src[5] + tmp[3] * src[6] + tmp[4] * src[7]) - (tmp[1] * src[5] + tmp[2] * src[6] + tmp[5] * src[7]);
                data[1] = (tmp[1] * src[4] + tmp[6] * src[6] + tmp[9] * src[7]) - (tmp[0] * src[4] + tmp[7] * src[6] + tmp[8] * src[7]);
                data[2] = (tmp[2] * src[4] + tmp[7] * src[5] + tmp[10] * src[7]) - (tmp[3] * src[4] + tmp[6] * src[5] + tmp[11] * src[7]);
                data[3] = (tmp[5] * src[4] + tmp[8] * src[5] + tmp[11] * src[6]) - (tmp[4] * src[4] + tmp[9] * src[5] + tmp[10] * src[6]);
                data[4] = (tmp[1] * src[1] + tmp[2] * src[2] + tmp[5] * src[3]) - (tmp[0] * src[1] + tmp[3] * src[2] + tmp[4] * src[3]);
                data[5] = (tmp[0] * src[0] + tmp[7] * src[2] + tmp[8] * src[3]) - (tmp[1] * src[0] + tmp[6] * src[2] + tmp[9] * src[3]);
                data[6] = (tmp[3] * src[0] + tmp[6] * src[1] + tmp[11] * src[3]) - (tmp[2] * src[0] + tmp[7] * src[1] + tmp[10] * src[3]);
                data[7] = (tmp[4] * src[0] + tmp[9] * src[1] + tmp[10] * src[2]) - (tmp[5] * src[0] + tmp[8] * src[1] + tmp[11] * src[2]);

                // calculate pairs for second 8 elements (cofactors)
                tmp[0] = src[2] * src[7];
                tmp[1] = src[3] * src[6];
                tmp[2] = src[1] * src[7];
                tmp[3] = src[3] * src[5];
                tmp[4] = src[1] * src[6];
                tmp[5] = src[2] * src[5];
                tmp[6] = src[0] * src[7];
                tmp[7] = src[3] * src[4];
                tmp[8] = src[0] * src[6];
                tmp[9] = src[2] * src[4];
                tmp[10] = src[0] * src[5];
                tmp[11] = src[1] * src[4];

                // calculate second 8 elements (cofactors)
                data[8] = (tmp[0] * src[13] + tmp[3] * src[14] + tmp[4] * src[15]) - (tmp[1] * src[13] + tmp[2] * src[14] + tmp[5] * src[15]);
                data[9] = (tmp[1] * src[12] + tmp[6] * src[14] + tmp[9] * src[15]) - (tmp[0] * src[12] + tmp[7] * src[14] + tmp[8] * src[15]);
                data[10] = (tmp[2] * src[12] + tmp[7] * src[13] + tmp[10] * src[15]) - (tmp[3] * src[12] + tmp[6] * src[13] + tmp[11] * src[15]);
                data[11] = (tmp[5] * src[12] + tmp[8] * src[13] + tmp[11] * src[14]) - (tmp[4] * src[12] + tmp[9] * src[13] + tmp[10] * src[14]);
                data[12] = (tmp[2] * src[10] + tmp[5] * src[11] + tmp[1] * src[9]) - (tmp[4] * src[11] + tmp[0] * src[9] + tmp[3] * src[10]);
                data[13] = (tmp[8] * src[11] + tmp[0] * src[8] + tmp[7] * src[10]) - (tmp[6] * src[10] + tmp[9] * src[11] + tmp[1] * src[8]);
                data[14] = (tmp[6] * src[9] + tmp[11] * src[11] + tmp[3] * src[8]) - (tmp[10] * src[11] + tmp[2] * src[8] + tmp[7] * src[9]);
                data[15] = (tmp[10] * src[10] + tmp[4] * src[8] + tmp[9] * src[9]) - (tmp[8] * src[9] + tmp[11] * src[10] + tmp[5] * src[8]);

                // calculate determinant
                float det = src[0] * data[0] + src[1] * data[1] + src[2] * data[2] + src[3] * data[3];

                // calculate matrix inverse
                float invdet = 1 / det;
                for (int step = 0; step < 16; ++step)
                {
                    data[step] *= invdet;
                }
            }

            void matrix_t::MakeRotation(const Math::Float3 & axis, float angle)
            {
                float length2 = axis.getMagnitude();
                if (length2 < FLT_EPSILON)
                {
                    SetToIdentity();
                    return;
                }

                Math::Float3 n = axis * (1.0f / std::sqrt(length2));
                float s = std::sin(angle);
                float c = std::cos(angle);
                float k = 1.0f - c;

                float xx = n.x * n.x * k + c;
                float yy = n.y * n.y * k + c;
                float zz = n.z * n.z * k + c;
                float xy = n.x * n.y * k;
                float yz = n.y * n.z * k;
                float zx = n.z * n.x * k;
                float xs = n.x * s;
                float ys = n.y * s;
                float zs = n.z * s;

                table[0][0] = xx;
                table[0][1] = xy + zs;
                table[0][2] = zx - ys;
                table[0][3] = 0.0f;
                table[1][0] = xy - zs;
                table[1][1] = yy;
                table[1][2] = yz + xs;
                table[1][3] = 0.0f;
                table[2][0] = zx + ys;
                table[2][1] = yz - xs;
                table[2][2] = zz;
                table[2][3] = 0.0f;
                table[3][0] = 0.0f;
                table[3][1] = 0.0f;
                table[3][2] = 0.0f;
                table[3][3] = 1.0f;
            }

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
                "Z : %5.2f",
                "XYZ : %5.2f"
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

            enum MOVETYPE
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
            };

            struct Context
            {
                Context()
                    : mbUsing(false)
                    , mbEnable(true)
                {
                }

                ImDrawList* mDrawList;

                Alignment mMode;
                matrix_t mViewMat;
                matrix_t mProjectionMat;
                matrix_t mModel;
                matrix_t mModelInverse;
                matrix_t mModelSource;
                matrix_t mModelSourceInverse;
                matrix_t mMVP;
                matrix_t mViewProjection;

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

                bool mbUsing;
                bool mbEnable;

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
                matrix_t mBoundsMatrix;

                //
                int mCurrentOperation;

                float mX = 0.0f;
                float mY = 0.0f;
                float mWidth = 0.0f;
                float mHeight = 0.0f;

                ImVec2 worldToPos(const Math::Float3& worldPos, const matrix_t& mat)
                {
                    ImGuiIO& io = ImGui::GetIO();

                    Math::Float4 trans = mat.transform(Math::Float4(worldPos, 1.0f));
                    trans *= 0.5f / trans.w;
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

                    matrix_t mViewProjInverse;
                    mViewProjInverse.getInverse(mViewMat * mProjectionMat);

                    float mox = ((io.MousePos.x - mX) / mWidth) * 2.0f - 1.0f;
                    float moy = (1.0f - ((io.MousePos.y - mY) / mHeight)) * 2.0f - 1.0f;

                    Math::Float4 rayStart;
                    rayStart = mViewProjInverse.transform(Math::Float4(mox, moy, 0.0f, 1.0f));
                    rayOrigin = rayStart.xyz;
                    rayOrigin *= 1.0f / rayStart.w;
                    Math::Float4 rayEnd;
                    rayEnd = mViewProjInverse.transform(Math::Float4(mox, moy, 1.0f, 1.0f));
                    rayEnd *= 1.0f / rayEnd.w;
                    rayDir = (rayEnd.xyz - rayOrigin).getNormal();
                }

                float IntersectRayPlane(const Math::Float3 & rOrigin, const Math::Float3& rVector, const Shapes::Plane& plane)
                {
                    float numer = plane.getDistance(rOrigin);
                    float denom = plane.normal.dot(rVector);

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
                    return mbUsing;
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
                    }
                }

                float GetUniform(const Math::Float3& position, const matrix_t& mat)
                {
                    return mat.transform(Math::Float4(position, 1.0f)).w;
                }

                void ComputeContext(const float *view, const float *projection, float *matrix, Alignment mode)
                {
                    mMode = mode;
                    mViewMat = *(matrix_t*)view;
                    mProjectionMat = *(matrix_t*)projection;

                    if (mode == Alignment::Local)
                    {
                        mModel = *(matrix_t*)matrix;
                        mModel.orthonormalize();
                    }
                    else
                    {
                        mModel.MakeTranslation(((matrix_t*)matrix)->rw.xyz);
                    }

                    mModelSource = *(matrix_t*)matrix;
                    mModelScaleOrigin.set(mModelSource.rx.getLength(), mModelSource.ry.getLength(), mModelSource.rz.getLength());

                    mModelInverse.getInverse(mModel);
                    mModelSourceInverse.getInverse(mModelSource);
                    mViewProjection = mViewMat * mProjectionMat;
                    mMVP = mModel * mViewProjection;

                    matrix_t viewInverse;
                    viewInverse.getInverse(mViewMat);
                    mCameraDir = viewInverse.rz.xyz;
                    mCameraEye = viewInverse.rw.xyz;
                    mCameraRight = viewInverse.rx.xyz;
                    mCameraUp = viewInverse.ry.xyz;
                    mScreenFactor = 0.15f * GetUniform(mModel.rw.xyz, mViewProjection);

                    ImVec2 centerSSpace = worldToPos(Math::Float3(0.0f, 0.0f, 0.0f), mMVP);
                    mScreenSquareCenter = centerSSpace;
                    mScreenSquareMin = ImVec2(centerSSpace.x - 10.0f, centerSSpace.y - 10.0f);
                    mScreenSquareMax = ImVec2(centerSSpace.x + 10.0f, centerSSpace.y + 10.0f);

                    ComputeCameraRay(mRayOrigin, mRayVector);
                }

                void ComputeColors(ImU32 *colors, int type, Operation operation)
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
                        Math::Float3 dirPlaneNormalWorld;
                        dirPlaneNormalWorld = mModel.rotate(directionUnary[planeNormal]);
                        dirPlaneNormalWorld.normalize();

                        Math::Float3 dirPlaneXWorld = mModel.rotate(dirPlaneX);
                        dirPlaneXWorld.normalize();

                        Math::Float3 dirPlaneYWorld = mModel.rotate(dirPlaneY);
                        dirPlaneYWorld.normalize();

                        Math::Float3 cameraEyeToGizmo = (mModel.rw.xyz - mCameraEye).getNormal();
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
                    Math::Float3 localPos = (mRayOrigin + mRayVector * len - mModel.rw.xyz).getNormal();

                    Math::Float3 perpendicularVector;
                    perpendicularVector = mRotationVectorSource.cross(mTranslationPlane.normal);
                    perpendicularVector.normalize();
                    float acosAngle = std::clamp(localPos.dot(mRotationVectorSource), -0.9999f, 0.9999f);
                    float angle = std::acos(acosAngle);
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
                    Math::Float3 cameraToModelNormalized = mModelInverse.rotate((mModel.rw.xyz - mCameraEye).getNormal());
                    for (int axis = 0; axis < 3; axis++)
                    {
                        ImVec2 circlePos[halfCircleSegmentCount];
                        float angleStart = std::atan2(cameraToModelNormalized[(4 - axis) % 3], cameraToModelNormalized[(3 - axis) % 3]) + Math::Pi * 0.5f;
                        for (uint32_t index = 0; index < halfCircleSegmentCount; index++)
                        {
                            float ng = angleStart + Math::Pi * ((float)index / (float)halfCircleSegmentCount);
                            Math::Float3 axisPos = Math::Float3(std::cos(ng), std::sin(ng), 0.0f);
                            Math::Float3 pos = Math::Float3(axisPos[axis], axisPos[(axis + 1) % 3], axisPos[(axis + 2) % 3]) * mScreenFactor;
                            circlePos[index] = worldToPos(pos, mMVP);
                        }

                        drawList->AddPolyline(circlePos, halfCircleSegmentCount, colors[3 - axis], false, 5, true);
                    }

                    drawList->AddCircle(worldToPos(mModel.rw.xyz, mViewProjection), screenRotateSize * mHeight, colors[0], 64, 5);
                    if (mbUsing)
                    {
                        ImVec2 circlePos[halfCircleSegmentCount + 1];

                        circlePos[0] = worldToPos(mModel.rw.xyz, mViewProjection);
                        for (uint32_t index = 1; index < halfCircleSegmentCount; index++)
                        {
                            float ng = mRotationAngle * ((float)(index - 1) / (float)(halfCircleSegmentCount - 1));

                            matrix_t rotateVectorMatrix;
                            rotateVectorMatrix.MakeRotation(mTranslationPlane.normal, ng);

                            Math::Float3 pos = rotateVectorMatrix.transform(mRotationVectorSource) * mScreenFactor;
                            circlePos[index] = worldToPos(pos + mModel.rw.xyz, mViewProjection);
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

                void DrawHatchedAxis(const Math::Float3& axis)
                {
                    for (int step = 1; step < 10; step++)
                    {
                        ImVec2 baseSSpace2 = worldToPos(axis * 0.05f * (float)(step * 2) * mScreenFactor, mMVP);
                        ImVec2 worldDirSSpace2 = worldToPos(axis * 0.05f * (float)(step * 2 + 1) * mScreenFactor, mMVP);
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
                    Math::Float3 scaleDisplay(1.0f, 1.0f, 1.0f);
                    if (mbUsing)
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
                            {
                                DrawHatchedAxis(dirPlaneX * scaleDisplay[index]);
                            }
                        }
                    }

                    if (mbUsing)
                    {
                        ImVec2 destinationPosOnScreen = worldToPos(mModel.rw.xyz, mViewProjection);

                        char tmps[512];
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
                    {
                        return;
                    }

                    // colors
                    ImU32 colors[7];
                    ComputeColors(colors, type, Operation::Translate);

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
                            ImVec2 baseSSpace = worldToPos(dirPlaneX * 0.1f * mScreenFactor, mMVP);
                            ImVec2 worldDirSSpace = worldToPos(dirPlaneX * mScreenFactor, mMVP);
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
                                Math::Float3 cornerWorldPos = (dirPlaneX * quadUV[step * 2] + dirPlaneY  * quadUV[step * 2 + 1]) * mScreenFactor;
                                screenQuadPts[step] = worldToPos(cornerWorldPos, mMVP);
                            }

                            drawList->AddConvexPolyFilled(screenQuadPts, 4, colors[index + 4], true);
                        }
                    }

                    if (mbUsing)
                    {
                        ImVec2 sourcePosOnScreen = worldToPos(mMatrixOrigin, mViewProjection);
                        ImVec2 destinationPosOnScreen = worldToPos(mModel.rw.xyz, mViewProjection);
                        Math::Float3 dif(destinationPosOnScreen.x - sourcePosOnScreen.x, destinationPosOnScreen.y - sourcePosOnScreen.y, 0.0f);
                        dif.normalize();
                        dif *= 5.0f;

                        drawList->AddCircle(sourcePosOnScreen, 6.0f, translationLineColor);
                        drawList->AddCircle(destinationPosOnScreen, 6.0f, translationLineColor);
                        drawList->AddLine(ImVec2(sourcePosOnScreen.x + dif.x, sourcePosOnScreen.y + dif.y), ImVec2(destinationPosOnScreen.x - dif.x, destinationPosOnScreen.y - dif.y), translationLineColor, 2.0f);

                        char tmps[512];
                        Math::Float3 deltaInfo = mModel.rw.xyz - mMatrixOrigin;
                        int componentInfoIndex = (type - MOVE_X) * 3;
                        ImFormatString(tmps, sizeof(tmps), translationInfoMask[type - MOVE_X], deltaInfo[translationInfoIndex[componentInfoIndex]], deltaInfo[translationInfoIndex[componentInfoIndex + 1]], deltaInfo[translationInfoIndex[componentInfoIndex + 2]]);
                        drawList->AddText(ImVec2(destinationPosOnScreen.x + 15, destinationPosOnScreen.y + 15), 0xFF000000, tmps);
                        drawList->AddText(ImVec2(destinationPosOnScreen.x + 14, destinationPosOnScreen.y + 14), 0xFFFFFFFF, tmps);
                    }
                }

                bool isZero(float v)
                {
                    return (v > -0.000001f && v < 0.000001f);
                }

                bool isPointInside(int x, int y)
                {
                    return (x >= mX && x <= (mX + mWidth) && y >= mY && y <= (mY + mHeight));
                }

                bool isClipped(float num, float denom)
                {
                    float t;
                    if (isZero(denom))
                    {
                        return num < 0.0;
                    }

                    t = num / denom;
                    if (denom > 0)
                    {
                        if (t > 1.0f)
                        {
                            return false;
                        }
                    }
                    else
                    {
                        if (t < 0.0f)
                        {
                            return false;
                        }
                    }

                    return true;
                }

                bool isInside(int x1, int y1, int x2, int y2)
                {
                    float dx = x2 - x1;
                    float dy = y2 - y1;
                    if (isZero(dx) && isZero(dy) && isPointInside(x1, y1))
                    {
                        return true;
                    }

                    if (isClipped(mX - x1, dx) &&
                        isClipped(x1 - (mX + mWidth), -dx) &&
                        isClipped(mY - y1, dy) &&
                        isClipped(y1 - (mY + mHeight), -dy))
                    {
                        return true;
                    }

                    return false;
                }

                void HandleBounds(float *bounds, matrix_t *matrix, float *snapValues)
                {
                    ImGuiIO& io = ImGui::GetIO();
                    ImDrawList* drawList = mDrawList;

                    // compute best projection axis
                    Math::Float3 bestAxisWorldDirection;
                    int bestAxis = mBoundsBestAxis;
                    if (!mbUsing)
                    {
                        float bestDot = 0.0f;
                        for (uint32_t index = 0; index < 3; index++)
                        {
                            Math::Float3 dirPlaneNormalWorld = mModelSource.rotate(directionUnary[index]);
                            dirPlaneNormalWorld.normalize();

                            float dt = (mCameraEye - mModelSource.rw.xyz).getNormal().dot(dirPlaneNormalWorld);
                            if (std::abs(dt) >= bestDot)
                            {
                                bestDot = std::abs(dt);
                                bestAxis = index;
                                bestAxisWorldDirection = dirPlaneNormalWorld;
                            }
                        }
                    }

                    // corners
                    Math::Float3 aabb[4];

                    static const uint32_t anchorColors[3] = 
                    {
                        0xFF0000,
                        0x00FF00,
                        0x0000FF,
                    };

                    int secondAxis = (bestAxis + 1) % 3;
                    int thirdAxis = (bestAxis + 2) % 3;

                    const uint32_t axisColors[4] =
                    {
                        anchorColors[secondAxis],
                        anchorColors[thirdAxis],
                        anchorColors[secondAxis],
                        anchorColors[thirdAxis],
                    };

                    for (int index = 0; index < 4; index++)
                    {
                        aabb[index][bestAxis] = 0.0f;
                        aabb[index][secondAxis] = bounds[secondAxis + 3 * (index >> 1)];
                        aabb[index][thirdAxis] = bounds[thirdAxis + 3 * ((index >> 1) ^ (index & 1))];
                    }

                    // draw bounds
                    uint32_t anchorAlpha = mbEnable ? 0xFF000000 : 0x80000000;
                    matrix_t boundsMVP = mModelSource * mViewProjection;
                    for (int index = 0; index < 4; index++)
                    {
                        uint32_t axisColor = axisColors[index] + anchorAlpha;

                        ImVec2 worldBound1 = worldToPos(aabb[index], boundsMVP);
                        ImVec2 worldBound2 = worldToPos(aabb[(index + 1) % 4], boundsMVP);
                        float boundDistance = std::sqrt(ImLengthSqr(worldBound1 - worldBound2));
                        int stepCount = (int)(boundDistance / 10.0f);
                        float stepLength = 1.0f / (float)stepCount;
                        for (int step = 0; step < stepCount; step++)
                        {
                            float t1 = (float)step * stepLength;
                            float t2 = (float)step * stepLength + stepLength * 0.5f;
                            ImVec2 worldBoundSS1 = ImLerp(worldBound1, worldBound2, ImVec2(t1, t1));
                            ImVec2 worldBoundSS2 = ImLerp(worldBound1, worldBound2, ImVec2(t2, t2));
                            if (isInside(worldBoundSS1.x, worldBoundSS1.y, worldBoundSS2.x, worldBoundSS2.y))
                            {
                                drawList->AddLine(worldBoundSS1, worldBoundSS2, axisColor, 3.0f);
                            }
                        }
                    }

                    for (int index = 0; index < 4; index++)
                    {
                        uint32_t axisColor = axisColors[index] + anchorAlpha;
                        uint32_t otherColor = axisColors[(index + 1) % 4] + anchorAlpha;

                        ImVec2 worldBound1 = worldToPos(aabb[index], boundsMVP);
                        ImVec2 worldBound2 = worldToPos(aabb[(index + 1) % 4], boundsMVP);
                        Math::Float3 midPoint = (aabb[index] + aabb[(index + 1) % 4]) * 0.5f;
                        ImVec2 midBound = worldToPos(midPoint, boundsMVP);

                        static const float AnchorBigRadius = 10.0f;
                        static const float AnchorSmallRadius = 8.0f;
                        bool overBigAnchor = ImLengthSqr(worldBound1 - io.MousePos) <= (AnchorBigRadius*AnchorBigRadius);
                        bool overSmallAnchor = ImLengthSqr(midBound - io.MousePos) <= (AnchorBigRadius*AnchorBigRadius);
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

                        // big anchor on corners
                        int oppositeIndex = (index + 2) % 4;
                        if (!mbUsing && mbEnable && overBigAnchor && io.MouseDown[0])
                        {
                            mBoundsPivot = mModelSource.transform(aabb[(index + 2) % 4]);
                            mBoundsAnchor = mModelSource.transform(aabb[index]);
                            mBoundsPlane = Shapes::Plane(bestAxisWorldDirection, mBoundsAnchor);
                            mBoundsBestAxis = bestAxis;
                            mBoundsAxis[0] = secondAxis;
                            mBoundsAxis[1] = thirdAxis;

                            mBoundsLocalPivot.set(0.0f);
                            mBoundsLocalPivot[secondAxis] = aabb[oppositeIndex][secondAxis];
                            mBoundsLocalPivot[thirdAxis] = aabb[oppositeIndex][thirdAxis];

                            mbUsing = true;
                            mBoundsMatrix = mModelSource;
                        }

                        // small anchor on middle of segment
                        if (!mbUsing && mbEnable && overSmallAnchor && io.MouseDown[0])
                        {
                            Math::Float3 midPointOpposite = (aabb[(index + 2) % 4] + aabb[(index + 3) % 4]) * 0.5f;
                            mBoundsPivot = mModelSource.transform(midPointOpposite);
                            mBoundsAnchor = mModelSource.transform(midPoint);
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
                            mBoundsLocalPivot[mBoundsAxis[0]] = aabb[oppositeIndex][indices[index % 2]];// bounds[mBoundsAxis[0]] * (((index + 1) & 2) ? 1.0f : -1.0f);

                            mbUsing = true;
                            mBoundsMatrix = mModelSource;
                        }
                    }

                    if (mbUsing)
                    {
                        matrix_t scale;
                        scale.SetToIdentity();

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
                            float boundSize = bounds[axisIndex + 3] - bounds[axisIndex];
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
                        matrix_t preScale, postScale;
                        preScale.MakeTranslation(-mBoundsLocalPivot);
                        postScale.MakeTranslation(mBoundsLocalPivot);
                        matrix_t res = preScale * scale * postScale * mBoundsMatrix;
                        *matrix = res;

                        // info text
                        char tmps[512];
                        ImVec2 destinationPosOnScreen = worldToPos(mModel.rw.xyz, mViewProjection);
                        ImFormatString(tmps, sizeof(tmps), "X: %.2f Y: %.2f Z:%.2f"
                            , (bounds[3] - bounds[0]) * mBoundsMatrix.rows[0].getLength() * scale.rows[0].getLength()
                            , (bounds[4] - bounds[1]) * mBoundsMatrix.rows[1].getLength() * scale.rows[1].getLength()
                            , (bounds[5] - bounds[2]) * mBoundsMatrix.rows[2].getLength() * scale.rows[2].getLength()
                        );

                        drawList->AddText(ImVec2(destinationPosOnScreen.x + 15, destinationPosOnScreen.y + 15), 0xFF000000, tmps);
                        drawList->AddText(ImVec2(destinationPosOnScreen.x + 14, destinationPosOnScreen.y + 14), 0xFFFFFFFF, tmps);
                    }

                    if (!io.MouseDown[0])
                    {
                        mbUsing = false;
                    }
                }

                int GetScaleType()
                {
                    ImGuiIO& io = ImGui::GetIO();
                    int type = NONE;

                    // screen
                    if (io.MousePos.x >= mScreenSquareMin.x && io.MousePos.x <= mScreenSquareMax.x &&
                        io.MousePos.y >= mScreenSquareMin.y && io.MousePos.y <= mScreenSquareMax.y)
                    {
                        type = SCALE_XYZ;
                    }

                    const Math::Float3 direction[3] =
                    {
                        mModel.rx.xyz,
                        mModel.ry.xyz,
                        mModel.rz.xyz,
                    };

                    // compute
                    for (uint32_t index = 0; index < 3 && type == NONE; index++)
                    {
                        Math::Float3 dirPlaneX, dirPlaneY;
                        bool belowAxisLimit, belowPlaneLimit;
                        ComputeTripodAxisAndVisibility(index, dirPlaneX, dirPlaneY, belowAxisLimit, belowPlaneLimit);
                        dirPlaneX = mModel.rotate(dirPlaneX);
                        dirPlaneY = mModel.rotate(dirPlaneY);

                        const int planeNormal = (index + 2) % 3;
                        const float len = IntersectRayPlane(mRayOrigin, mRayVector, Shapes::Plane(direction[planeNormal], mModel.rw.xyz));
                        Math::Float3 posOnPlane = mRayOrigin + mRayVector * len;

                        const float dx = dirPlaneX.dot((posOnPlane - mModel.rw.xyz) * (1.0f / mScreenFactor));
                        const float dy = dirPlaneY.dot((posOnPlane - mModel.rw.xyz) * (1.0f / mScreenFactor));
                        if (belowAxisLimit && dy > -0.1f && dy < 0.1f && dx > 0.1f  && dx < 1.0f)
                        {
                            type = SCALE_X + index;
                        }
                    }

                    return type;
                }

                int GetRotateType()
                {
                    ImGuiIO& io = ImGui::GetIO();
                    int type = NONE;

                    Math::Float3 deltaScreen(io.MousePos.x - mScreenSquareCenter.x, io.MousePos.y - mScreenSquareCenter.y, 0.0f);
                    float dist = deltaScreen.getLength();
                    if (dist >= (screenRotateSize - 0.005f) * mHeight && dist < (screenRotateSize + 0.005f) * mHeight)
                    {
                        type = ROTATE_SCREEN;
                    }

                    const Math::Float3 planeNormals[] =
                    {
                        mModel.rx.xyz,
                        mModel.ry.xyz,
                        mModel.rz.xyz,
                    };

                    for (uint32_t index = 0; index < 3 && type == NONE; index++)
                    {
                        // pickup plane
                        Shapes::Plane pickupPlane(planeNormals[index], mModel.rw.xyz);
                        const float len = IntersectRayPlane(mRayOrigin, mRayVector, pickupPlane);
                        Math::Float3 localPos = mRayOrigin + mRayVector * len - mModel.rw.xyz;
                        if (localPos.getNormal().dot(mRayVector) > FLT_EPSILON)
                        {
                            //continue;
                        }

                        float distance = localPos.getLength() / mScreenFactor;
                        if (distance > 0.85f && distance < 1.15f)
                        {
                            type = ROTATE_X + index;
                        }
                    }

                    return type;
                }

                int GetMoveType(Math::Float3 *gizmoHitProportion)
                {
                    ImGuiIO& io = ImGui::GetIO();
                    int type = NONE;

                    // screen
                    if (io.MousePos.x >= mScreenSquareMin.x && io.MousePos.x <= mScreenSquareMax.x &&
                        io.MousePos.y >= mScreenSquareMin.y && io.MousePos.y <= mScreenSquareMax.y)
                    {
                        type = MOVE_SCREEN;
                    }

                    const Math::Float3 direction[3] =
                    {
                        mModel.rx.xyz,
                        mModel.ry.xyz,
                        mModel.rz.xyz,
                    };

                    // compute
                    for (uint32_t index = 0; index < 3 && type == NONE; index++)
                    {
                        Math::Float3 dirPlaneX, dirPlaneY;
                        bool belowAxisLimit, belowPlaneLimit;
                        ComputeTripodAxisAndVisibility(index, dirPlaneX, dirPlaneY, belowAxisLimit, belowPlaneLimit);
                        dirPlaneX = mModel.rotate(dirPlaneX);
                        dirPlaneY = mModel.rotate(dirPlaneY);

                        const int planeNormal = (index + 2) % 3;
                        const float len = IntersectRayPlane(mRayOrigin, mRayVector, Shapes::Plane(direction[planeNormal], mModel.rw.xyz));
                        Math::Float3 posOnPlane = mRayOrigin + mRayVector * len;

                        const float dx = dirPlaneX.dot((posOnPlane - mModel.rw.xyz) * (1.0f / mScreenFactor));
                        const float dy = dirPlaneY.dot((posOnPlane - mModel.rw.xyz) * (1.0f / mScreenFactor));
                        if (belowAxisLimit && dy > -0.1f && dy < 0.1f && dx > 0.1f  && dx < 1.0f)
                        {
                            type = MOVE_X + index;
                        }

                        if (belowPlaneLimit && dx >= quadUV[0] && dx <= quadUV[4] && dy >= quadUV[1] && dy <= quadUV[3])
                        {
                            type = MOVE_XY + index;
                        }

                        if (gizmoHitProportion)
                        {
                            *gizmoHitProportion = Math::Float3(dx, dy, 0.0f);
                        }
                    }

                    return type;
                }

                void HandleTranslation(float *matrix, float *deltaMatrix, float *snap)
                {
                    ImGuiIO& io = ImGui::GetIO();
                    bool applyRotationLocaly = mMode == Alignment::Local;

                    // move
                    int type = NONE;
                    if (mbUsing)
                    {
                        const float len = IntersectRayPlane(mRayOrigin, mRayVector, mTranslationPlane);
                        Math::Float3 newPos = mRayOrigin + mRayVector * len;

                        // compute delta
                        Math::Float3 newOrigin = newPos - mRelativeOrigin * mScreenFactor;
                        Math::Float3 delta = newOrigin - mModel.rw.xyz;

                        // 1 axis constraint
                        if (mCurrentOperation >= MOVE_X && mCurrentOperation <= MOVE_Z)
                        {
                            int axisIndex = mCurrentOperation - MOVE_X;
                            const Math::Float3& axisValue = mModel.rows[axisIndex].xyz;
                            float lengthOnAxis = axisValue.dot(delta);
                            delta = axisValue * lengthOnAxis;
                        }

                        // snap
                        if (snap)
                        {
                            Math::Float3 cumulativeDelta = mModel.rw.xyz + delta - mMatrixOrigin;
                            if (applyRotationLocaly)
                            {
                                matrix_t modelSourceNormalized = mModelSource;
                                modelSourceNormalized.orthonormalize();
                                matrix_t modelSourceNormalizedInverse;
                                modelSourceNormalizedInverse.getInverse(modelSourceNormalized);
                                cumulativeDelta = modelSourceNormalizedInverse.rotate(cumulativeDelta);
                                ComputeSnap(cumulativeDelta, snap);
                                cumulativeDelta = modelSourceNormalized.rotate(cumulativeDelta);
                            }
                            else
                            {
                                ComputeSnap(cumulativeDelta, snap);
                            }

                            delta = mMatrixOrigin + cumulativeDelta - mModel.rw.xyz;
                        }

                        // compute matrix & delta
                        matrix_t deltaMatrixTranslation;
                        deltaMatrixTranslation.MakeTranslation(delta);
                        if (deltaMatrix)
                        {
                            memcpy(deltaMatrix, deltaMatrixTranslation.data, sizeof(matrix_t));
                        }


                        matrix_t res = mModelSource * deltaMatrixTranslation;
                        *(matrix_t*)matrix = res;

                        if (!io.MouseDown[0])
                        {
                            mbUsing = false;
                        }

                        type = mCurrentOperation;
                    }
                    else
                    {
                        // find new possible way to move
                        Math::Float3 gizmoHitProportion;
                        type = GetMoveType(&gizmoHitProportion);
                        if (io.MouseDown[0] && type != NONE)
                        {
                            mbUsing = true;
                            mCurrentOperation = type;
                            const Math::Float3 movePlaneNormal[] =
                            {
                                mModel.ry.xyz,
                                mModel.rz.xyz,
                                mModel.rx.xyz,
                                mModel.rz.xyz,
                                mModel.rx.xyz,
                                mModel.ry.xyz,
                                -mCameraDir,
                            };

                            // pickup plane
                            mTranslationPlane = Shapes::Plane(movePlaneNormal[type - MOVE_X], mModel.rw.xyz);
                            const float len = IntersectRayPlane(mRayOrigin, mRayVector, mTranslationPlane);
                            mTranslationPlaneOrigin = mRayOrigin + mRayVector * len;
                            mMatrixOrigin = mModel.rw.xyz;

                            mRelativeOrigin = (mTranslationPlaneOrigin - mModel.rw.xyz) * (1.0f / mScreenFactor);
                        }
                    }

                    DrawTranslationGizmo(type);
                }

                void HandleScale(float *matrix, float *deltaMatrix, float *snap)
                {
                    ImGuiIO& io = ImGui::GetIO();

                    int type = NONE;
                    if (!mbUsing)
                    {
                        // find new possible way to scale
                        type = GetScaleType();
                        if (io.MouseDown[0] && type != NONE)
                        {
                            mbUsing = true;
                            mCurrentOperation = type;
                            const Math::Float3 movePlaneNormal[] =
                            {
                                mModel.ry.xyz,
                                mModel.rz.xyz,
                                mModel.rx.xyz,
                                mModel.rz.xyz,
                                mModel.ry.xyz,
                                mModel.rx.xyz,
                                -mCameraDir,
                            };

                            // pickup plane
                            mTranslationPlane = Shapes::Plane(movePlaneNormal[type - SCALE_X], mModel.rw.xyz);
                            const float len = IntersectRayPlane(mRayOrigin, mRayVector, mTranslationPlane);
                            mTranslationPlaneOrigin = mRayOrigin + mRayVector * len;
                            mMatrixOrigin = mModel.rw.xyz;
                            mScale.set(1.0f, 1.0f, 1.0f);
                            mRelativeOrigin = (mTranslationPlaneOrigin - mModel.rw.xyz) * (1.0f / mScreenFactor);
                            mScaleValueOrigin = Math::Float3(mModelSource.rx.getLength(), mModelSource.ry.getLength(), mModelSource.rz.getLength());
                            mSaveMousePosx = io.MousePos.x;
                        }
                    }

                    // scale
                    if (mbUsing)
                    {
                        const float len = IntersectRayPlane(mRayOrigin, mRayVector, mTranslationPlane);
                        Math::Float3 newPos = mRayOrigin + mRayVector * len;
                        Math::Float3 newOrigin = newPos - mRelativeOrigin * mScreenFactor;
                        Math::Float3 delta = newOrigin - mModel.rw.xyz;

                        // 1 axis constraint
                        if (mCurrentOperation >= SCALE_X && mCurrentOperation <= SCALE_Z)
                        {
                            int axisIndex = mCurrentOperation - SCALE_X;
                            const Math::Float3& axisValue = *(Math::Float3*)&mModel.table[axisIndex];
                            float lengthOnAxis = axisValue.dot(delta);
                            delta = axisValue * lengthOnAxis;

                            Math::Float3 baseVector = mTranslationPlaneOrigin - mModel.rw.xyz;
                            float ratio = axisValue.dot(baseVector + delta) / axisValue.dot(baseVector);

                            mScale[axisIndex] = std::max(ratio, 0.001f);
                        }
                        else
                        {
                            float scaleDelta = (io.MousePos.x - mSaveMousePosx)  * 0.01f;
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
                        matrix_t deltaMatrixScale;
                        deltaMatrixScale.MakeScaling(mScale * mScaleValueOrigin);

                        matrix_t res = deltaMatrixScale * mModel;
                        *(matrix_t*)matrix = res;

                        if (deltaMatrix)
                        {
                            deltaMatrixScale.MakeScaling(mScale);
                            memcpy(deltaMatrix, deltaMatrixScale.data, sizeof(matrix_t));
                        }

                        if (!io.MouseDown[0])
                        {
                            mbUsing = false;
                        }

                        type = mCurrentOperation;
                    }

                    DrawScaleGizmo(type);
                }

                void HandleRotation(float *matrix, float *deltaMatrix, float *snap)
                {
                    ImGuiIO& io = ImGui::GetIO();
                    bool applyRotationLocaly = mMode == Alignment::Local;

                    int type = NONE;
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
                            const Math::Float3 rotatePlaneNormal[] =
                            {
                                mModel.rx.xyz,
                                mModel.ry.xyz,
                                mModel.rz.xyz,
                                -mCameraDir,
                            };

                            // pickup plane
                            if (applyRotationLocaly)
                            {
                                mTranslationPlane = Shapes::Plane(rotatePlaneNormal[type - ROTATE_X], mModel.rw.xyz);
                            }
                            else
                            {
                                mTranslationPlane = Shapes::Plane(directionUnary[type - ROTATE_X], mModelSource.rw.xyz);
                            }

                            const float len = IntersectRayPlane(mRayOrigin, mRayVector, mTranslationPlane);
                            Math::Float3 localPos = mRayOrigin + mRayVector * len - mModel.rw.xyz;
                            mRotationVectorSource = localPos.getNormal();
                            mRotationAngleOrigin = ComputeAngleOnPlane();
                        }
                    }

                    // rotation
                    if (mbUsing)
                    {
                        mRotationAngle = ComputeAngleOnPlane();
                        if (snap)
                        {
                            float snapInRadian = Math::DegreesToRadians(snap[0]);
                            ComputeSnap(&mRotationAngle, snapInRadian);
                        }

                        Math::Float3 rotationAxisLocalSpace = mModelInverse.rotate(mTranslationPlane.normal);
                        rotationAxisLocalSpace.normalize();

                        matrix_t deltaRotation;
                        deltaRotation.MakeRotation(rotationAxisLocalSpace, mRotationAngle - mRotationAngleOrigin);
                        mRotationAngleOrigin = mRotationAngle;

                        matrix_t scaleOrigin;
                        scaleOrigin.MakeScaling(mModelScaleOrigin);

                        if (applyRotationLocaly)
                        {
                            *(matrix_t*)matrix = scaleOrigin * deltaRotation * mModel;
                        }
                        else
                        {
                            matrix_t res = mModelSource;
                            res.rw.set(0.0f);

                            *(matrix_t*)matrix = res * deltaRotation;
                            ((matrix_t*)matrix)->rw = mModelSource.rw;
                        }

                        if (deltaMatrix)
                        {
                            *(matrix_t*)deltaMatrix = mModelInverse * deltaRotation * mModel;
                        }

                        if (!io.MouseDown[0])
                        {
                            mbUsing = false;
                        }

                        type = mCurrentOperation;
                    }

                    DrawRotationGizmo(type);
                }

                void Manipulate(const float *view, const float *projection, Operation operation, Alignment mode, float *matrix, float *deltaMatrix, float *snap, float *localBounds, float *boundsSnap)
                {
                    ComputeContext(view, projection, matrix, mode);

                    // set delta to identity 
                    if (deltaMatrix)
                    {
                        ((matrix_t*)deltaMatrix)->SetToIdentity();
                    }

                    // behind camera
                    Math::Float3 camSpacePosition = mMVP.transform(Math::Float3(0.0f, 0.0f, 0.0f));
                    if (camSpacePosition.z < 0.001f)
                    {
                        return;
                    }

                    // -- 
                    if (mbEnable)
                    {
                        switch (operation)
                        {
                        case Operation::Rotate:
                            HandleRotation(matrix, deltaMatrix, snap);
                            break;

                        case Operation::Translate:
                            HandleTranslation(matrix, deltaMatrix, snap);
                            break;

                        case Operation::Scale:
                            HandleScale(matrix, deltaMatrix, snap);
                            break;

                        case Operation::Bounds:
                            HandleBounds(localBounds, (matrix_t*)matrix, boundsSnap);
                            break;
                        };
                    }
                }

                void DrawCube(const float *view, const float *projection, float *matrix)
                {
                    matrix_t viewInverse;
                    viewInverse.getInverse(*(matrix_t*)view);
                    const matrix_t& model = *(matrix_t*)matrix;
                    matrix_t res = *(matrix_t*)matrix * *(matrix_t*)view * *(matrix_t*)projection;
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
                            Math::Float3 camSpacePosition = res.transform(faceCoords[iCoord] * 0.5f * invert);
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
                            faceCoordsScreen[iCoord] = worldToPos(faceCoords[iCoord] * 0.5f * invert, res);
                        }

                        // back face culling 
                        Math::Float3 cullPos, cullNormal;
                        cullPos = model.transform(faceCoords[0] * 0.5f * invert);
                        cullNormal = model.rotate(directionUnary[normalIndex] * invert);
                        float dt = (cullPos - viewInverse.rw.xyz).getNormal().dot(cullNormal.getNormal());
                        if (dt > 0.0f)
                        {
                            continue;
                        }

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

            void WorkSpace::beginFrame(float x, float y, float width, float height)
            {
                context->BeginFrame();
                context->SetRect(x, y, width, height);
            }

            void WorkSpace::manipulate(float const *view, float const *projection, Operation operation, Alignment alignment, float *matrix, float *deltaMatrix, float *snap, float *localBounds, float *boundsSnap)
            {
                context->Manipulate(view, projection, operation, alignment, matrix, deltaMatrix, snap, localBounds, boundsSnap);
            }
        }; // Gizmo
    }; // namespace UI
}; // namespace Gek