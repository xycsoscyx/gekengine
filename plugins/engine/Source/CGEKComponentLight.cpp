#include "CGEKComponentLight.h"

REGISTER_COMPONENT(light)
    REGISTER_SERIALIZE(color, StrFromFloat3)
    REGISTER_SERIALIZE(range, StrFromFloat)
REGISTER_SEPARATOR(light)
    REGISTER_DESERIALIZE(color, StrToFloat3)
    REGISTER_DESERIALIZE(range, StrToFloat)
END_REGISTER_COMPONENT(light)
