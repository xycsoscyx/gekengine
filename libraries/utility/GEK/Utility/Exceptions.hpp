/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision: 895bd642687b9b4b70b544b22192a2aa11a6e721 $
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date:   Thu Oct 13 20:39:05 2016 +0000 $
#pragma once

#define GEK_START_EXCEPTIONS()      struct Exception : public std::exception { Exception(const char *type) : std::exception(type) { } };
#define GEK_ADD_EXCEPTION(TYPE)     struct TYPE : public Exception { TYPE(void) : Exception(#TYPE) { } };
#define GEK_REQUIRE(CHECK)          do { if (!(CHECK)) { _ASSERTE(CHECK); exit(-1); } } while (false)
