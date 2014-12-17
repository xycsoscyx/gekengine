#include "CGEKComponentTransform.h"

REGISTER_COMPONENT(transform)
    REGISTER_SERIALIZE(position, StrFromFloat3)
    REGISTER_SERIALIZE(rotation, StrFromQuaternion)
REGISTER_SEPARATOR(transform)
    REGISTER_DESERIALIZE(position, StrToFloat3)
    REGISTER_DESERIALIZE(rotation, StrToQuaternion)
END_REGISTER_COMPONENT(transform)
