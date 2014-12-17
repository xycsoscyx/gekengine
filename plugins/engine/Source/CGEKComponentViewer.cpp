#include "CGEKComponentViewer.h"

REGISTER_COMPONENT(viewer)
REGISTER_SEPARATOR(viewer)
    REGISTER_DESERIALIZE(fieldofview, StrToFloat)
    REGISTER_DESERIALIZE(minviewdistance, StrToFloat)
    REGISTER_DESERIALIZE(maxviewdistance, StrToFloat)
    REGISTER_DESERIALIZE(viewport, StrToFloat4)
    REGISTER_DESERIALIZE(pass, )
END_REGISTER_COMPONENT(viewer)
