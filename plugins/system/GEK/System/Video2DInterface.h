#pragma once

#include "GEK\Math\Common.h"
#include "GEK\Math\Vector2.h"
#include "GEK\Math\Matrix3x2.h"
#include "GEK\Utility\Common.h"
#include <unordered_map>

namespace Gek
{
    namespace Video2D
    {
        namespace FontStyle
        {
            enum
            {
                Normal = 0,
                Italic,
            };
        }; // namespace FontStyle

        struct GradientPoint
        {
            float position;
            Math::Float4 color;
        };

        DECLARE_INTERFACE_IID_(GeometryInterface, IUnknown, "4CA2D559-66C1-46F3-ADFF-9B919AAB4575")
        {
            STDMETHOD(openShape)                    (THIS) PURE;
            STDMETHOD(closeShape)                   (THIS) PURE;

            STDMETHOD_(void, beginModifications)    (THIS_ const Math::Float2 &point, bool fillShape) PURE;
            STDMETHOD_(void, endModifications)      (THIS_ bool openEnded) PURE;

            STDMETHOD_(void, addLine)               (THIS_ const Math::Float2 &point) PURE;
            STDMETHOD_(void, addBezier)             (THIS_ const Math::Float2 &pointA, const Math::Float2 &pointB, const Math::Float2 &pointC) PURE;

            STDMETHOD(createWidened)                (THIS_ float width, float tolerance, GeometryInterface **geometry) PURE;
        };

        DECLARE_INTERFACE_IID_(Interface, IUnknown, "D3B65773-4EB1-46F8-A38D-009CA43CE77F")
        {
            STDMETHOD_(Handle, createBrush)         (THIS_ const Math::Float4 &color) PURE;
            STDMETHOD_(Handle, createBrush)         (THIS_ const std::vector<GradientPoint> &stopPoints, const Rectangle<float> &extents) PURE;

            STDMETHOD_(Handle, createFont)          (THIS_ LPCWSTR face, UINT32 weight, UINT8 style, float size) PURE;

            STDMETHOD(createGeometry)               (THIS_ GeometryInterface **returnObject) PURE;

            STDMETHOD_(void, setTransform)          (THIS_ const Math::Float3x2 &matrix) PURE;

            STDMETHOD_(void, drawText)              (THIS_ const Rectangle<float> &extents, Handle fontHandle, Handle brushHandle, LPCWSTR format, ...) PURE;

            STDMETHOD_(void, drawRectangle)         (THIS_ const Rectangle<float> &extents, Handle brushHandle, bool fillShape) PURE;
            STDMETHOD_(void, drawRectangle)         (THIS_ const Rectangle<float> &extents, const Math::Float2 &cornerRadius, Handle brushHandle, bool fillShape) PURE;

            STDMETHOD_(void, drawGeometry)          (THIS_ GeometryInterface *geometry, Handle brushHandle, bool fillShape) PURE;

            STDMETHOD_(void, beginDraw)             (THIS) PURE;
            STDMETHOD(endDraw)                      (THIS) PURE;
        };
    }; // namespace Video2D
}; // namespace Gek
