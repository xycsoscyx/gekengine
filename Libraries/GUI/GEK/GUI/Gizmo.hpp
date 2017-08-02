/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision: 1d50799f92d32e762c5d618bca16d2fe7fbeb872 $
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date:   Fri Oct 21 04:24:02 2016 +0000 $
#pragma once

#include "GEK/GUI/Utilities.hpp"
#include "GEK/Math/Matrix4x4.hpp"
#include "GEK/Math/Vector3.hpp"
#include "GEK/Shapes/AlignedBox.hpp"

namespace Gek
{
    namespace UI
    {
        namespace Gizmo
        {
            enum class Operation
            {
                None,
                Translate,
                Rotate,
                Scale,
                Bounds,
            };

            enum class Alignment
            {
                Local = 0,
                World,
            };

            enum class LockAxis
            {
                Automatic = 0,
                X,
                Y,
                Z,
            };

            struct Context;

            struct WorkSpace
            {
                WorkSpace(void);
                ~WorkSpace(void);

                void beginFrame(float x, float y, float width, float height);
                void manipulate(Math::Float4x4 const &view, Math::Float4x4 const &projection, Operation operation, Alignment alignment, Math::Float4x4 &matrix, float *snap = nullptr, Shapes::AlignedBox *localBounds = nullptr, LockAxis lockAxis = LockAxis::Automatic);

            private:
                Context *context = nullptr;
            };
        }; // namespace Gizmo
    }; // namespace UI
}; // namespace Gek
