#include "CGEKComponentViewer.h"

REGISTER_COMPONENT(viewer)
    REGISTER_COMPONENT_DEFAULT_VALUE(fieldofview, 90.0f)
    REGISTER_COMPONENT_DEFAULT_VALUE(mindistance, 0.1f)
    REGISTER_COMPONENT_DEFAULT_VALUE(maxdistance, 100.0f)
    REGISTER_COMPONENT_DEFAULT_VALUE(position, float2(0.0f, 0.0f))
    REGISTER_COMPONENT_DEFAULT_VALUE(size, float2(1.0f, 1.0f))
    REGISTER_COMPONENT_DEFAULT_VALUE(pass, L"")
    REGISTER_COMPONENT_SERIALIZE(viewer)
        REGISTER_COMPONENT_SERIALIZE_VALUE(fieldofview, StrFromFloat)
        REGISTER_COMPONENT_SERIALIZE_VALUE(mindistance, StrFromFloat)
        REGISTER_COMPONENT_SERIALIZE_VALUE(maxdistance, StrFromFloat)
        REGISTER_COMPONENT_SERIALIZE_VALUE(position, StrFromFloat2)
        REGISTER_COMPONENT_SERIALIZE_VALUE(size, StrFromFloat2)
        REGISTER_COMPONENT_SERIALIZE_VALUE(pass, )
    REGISTER_COMPONENT_DESERIALIZE(viewer)
        REGISTER_COMPONENT_DESERIALIZE_VALUE(fieldofview, StrToFloat)
        REGISTER_COMPONENT_DESERIALIZE_VALUE(mindistance, StrToFloat)
        REGISTER_COMPONENT_DESERIALIZE_VALUE(maxdistance, StrToFloat)
        REGISTER_COMPONENT_DESERIALIZE_VALUE(position, StrToFloat2)
        REGISTER_COMPONENT_DESERIALIZE_VALUE(size, StrToFloat2)
        REGISTER_COMPONENT_DESERIALIZE_VALUE(pass, )
END_REGISTER_COMPONENT(viewer)
