#pragma once

#include "GEK\Utility\String.h"

#define GEK_START_EXCEPTIONS()      struct Exception : public std::exception { Exception(const char *type) : std::exception(type) { } };
#define GEK_ADD_EXCEPTION(TYPE)     struct TYPE : public Exception { TYPE(void) : Exception(#TYPE) { } };
#define GEK_REQUIRE(CHECK)          do { if ((CHECK) == false) { _ASSERTE(CHECK); exit(-1); } } while (false)
