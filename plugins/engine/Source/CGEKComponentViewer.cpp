#include "CGEKComponentViewer.h"

REGISTER_COMPONENT(viewer)
REGISTER_SEPARATOR(viewer)
    REGISTER_DESERIALIZE(fov, StrToFloat)
    REGISTER_DESERIALIZE(mindistance, StrToFloat)
    REGISTER_DESERIALIZE(maxdistance, StrToFloat)
    REGISTER_DESERIALIZE(viewport, StrToFloat4)
    REGISTER_DESERIALIZE(pass, )
END_REGISTER_COMPONENT(viewer)
