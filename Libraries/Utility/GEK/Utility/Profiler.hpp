/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision: 1143 $
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date: 2016-10-13 13:29:45 -0700 (Thu, 13 Oct 2016) $
#pragma once

#define GEK_PROFILE_ENABLE()
#define GEK_PROFILE_START()
#define GEK_PROFILE_STOP()
#define GEK_PROFILE_DUMP(NAME)

#define GEK_PROFILE_AUTO_SCOPE(NAME)
#define GEK_PROFILE_FUNCTION(...)
#define GEK_PROFILE_BEGIN_SCOPE(NAME) \
    [&](void) -> void \
    {

#define GEK_PROFILE_END_SCOPE() \
    }()
