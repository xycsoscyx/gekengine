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

namespace Gek
{
    namespace UI
    {
        namespace Gizmo
        {
            struct Operation
            {
                enum 
                {
                    None,
                    Translate,
                    Rotate,
                    Scale,
                    Bounds,
                };
            };

            struct Alignment
            {
                enum
                {
                    Local = 0,
                    World,
                };
            };

            struct Context;

            struct WorkSpace
            {
                WorkSpace(void);
                ~WorkSpace(void);

                void beginFrame(void);
                void setViewPort(float x, float y, float width, float height);
                void manipulate(const float *view, const float *projection, int operation, int alignment, float *matrix, float *deltaMatrix = 0, float *snap = 0, float *localBounds = NULL, float *boundsSnap = NULL);

            private:
                Context *context = nullptr;
            };
        }; // namespace Gizmo
    }; // namespace UI
}; // namespace Gek
