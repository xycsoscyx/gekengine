#pragma once

#include "GEK\Math\Common.h"
#include "GEK\Math\Color.h"
#include "GEK\Math\Vector4.h"
#include "GEK\Math\Matrix3x2.h"
#include "GEK\Shapes\Rectangle.h"
#include <unordered_map>

namespace Gek
{
    namespace Video
    {
        enum class FontStyle : UINT8
        {
            Normal = 0,
            Italic,
        };

        enum class InterpolationMode : UINT8
        {
            NearestNeighbor = 0,
            Linear,
            Cubic,
            MultiSampleLinear,
            Anisotropic,
            HighQualityCubic,
        };

        struct GradientPoint
        {
            float position;
            Math::Color color;

            GradientPoint(float position, const Math::Color &color)
                : position(position)
                , color(color)
            {
            }
        };
    }; // namespace Video

    DECLARE_INTERFACE_IID(OverlayGeometry, "4CA2D559-66C1-46F3-ADFF-9B919AAB4575") : virtual public IUnknown
    {
        STDMETHOD(openShape)                    (THIS) PURE;
        STDMETHOD(closeShape)                   (THIS) PURE;

        STDMETHOD_(void, beginModifications)    (THIS_ const Math::Float2 &point, bool fillShape) PURE;
        STDMETHOD_(void, endModifications)      (THIS_ bool openEnded) PURE;

        STDMETHOD_(void, addLine)               (THIS_ const Math::Float2 &point) PURE;
        STDMETHOD_(void, addBezier)             (THIS_ const Math::Float2 &pointA, const Math::Float2 &pointB, const Math::Float2 &pointC) PURE;

        STDMETHOD(createWidened)                (THIS_ float width, float tolerance, OverlayGeometry **geometry) PURE;
    };

    DECLARE_INTERFACE_IID(OverlaySystem, "D3B65773-4EB1-46F8-A38D-009CA43CE77F") : virtual public IUnknown
    {
        STDMETHOD(createBrush)                  (THIS_ IUnknown **returnObject, const Math::Color &color) PURE;
        STDMETHOD(createBrush)                  (THIS_ IUnknown **returnObject, const std::vector<Video::GradientPoint> &stopPoints, const Shapes::Rectangle<float> &extents) PURE;

        STDMETHOD(createFont)                   (THIS_ IUnknown **returnObject, LPCWSTR face, UINT32 weight, Video::FontStyle style, float size) PURE;

        STDMETHOD(loadBitmap)                   (THIS_ IUnknown **returnObject, LPCWSTR fileName) PURE;

        STDMETHOD(createGeometry)               (THIS_ OverlayGeometry **returnObject) PURE;

        STDMETHOD_(void, setTransform)          (THIS_ const Math::Float3x2 &matrix) PURE;

        STDMETHOD_(void, drawText)              (THIS_ const Shapes::Rectangle<float> &extents, IUnknown *font, IUnknown *brush, LPCWSTR format, ...) PURE;

        STDMETHOD_(void, drawRectangle)         (THIS_ const Shapes::Rectangle<float> &extents, IUnknown *brush, bool fillShape) PURE;
        STDMETHOD_(void, drawRectangle)         (THIS_ const Shapes::Rectangle<float> &extents, const Math::Float2 &cornerRadius, IUnknown *brush, bool fillShape) PURE;

        STDMETHOD_(void, drawBitmap)            (THIS_ IUnknown *bitmap, const Shapes::Rectangle<float> &destinationExtents, Video::InterpolationMode interpolationMode = Video::InterpolationMode::NearestNeighbor, float opacity = 1.0) PURE;
        STDMETHOD_(void, drawBitmap)            (THIS_ IUnknown *bitmap, const Shapes::Rectangle<float> &destinationExtents, const Shapes::Rectangle<float> &sourceExtents, Video::InterpolationMode interpolationMode = Video::InterpolationMode::NearestNeighbor, float opacity = 1.0) PURE;

        STDMETHOD_(void, drawGeometry)          (THIS_ OverlayGeometry *geometry, IUnknown *brush, bool fillShape) PURE;

        STDMETHOD_(void, beginDraw)             (THIS) PURE;
        STDMETHOD(endDraw)                      (THIS) PURE;
    };
}; // namespace Gek
