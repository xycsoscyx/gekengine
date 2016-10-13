/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision: 1143 $
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date: 2016-10-13 13:29:45 -0700 (Thu, 13 Oct 2016) $
#pragma once

#define GEK_START_EXCEPTIONS()      struct Exception : public std::exception { Exception(const char *type) : std::exception(type) { } };
#define GEK_ADD_EXCEPTION(TYPE)     struct TYPE : public Exception { TYPE(void) : Exception(#TYPE) { } };
#define GEK_REQUIRE(CHECK)          do { if ((CHECK) == false) { _ASSERTE(CHECK); exit(-1); } } while (false)
