/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision: 1143 $
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date: 2016-10-13 13:29:45 -0700 (Thu, 13 Oct 2016) $
#pragma once

#include <easy/profiler.h>

#define GEK_PROFILE_REGISTER_NAME(PROFILER, NAME) 0
#define GEK_PROFILE_REGISTER_THREAD(PROFILER, NAME)
#define GEK_PROFILE_BEGIN_FRAME(PROFILER)
#define GEK_PROFILE_END_FRAME(PROFILER)
#define GEK_PROFILE_EVENT(PROFILER, NAME, THREAD, START_TIME, END_TIME)
#define GEK_PROFILE_AUTO_SCOPE(PROFILER, NAME)
#define GEK_PROFILE_FUNCTION(PROFILER)
#define GEK_PROFILE_BEGIN_SCOPE(PROFILER, NAME) \
    [&](void) -> void \
    { \

#define GEK_PROFILE_END_SCOPE() \
    }()

namespace Gek
{
    namespace System
    {
        class Profiler
        {
        };
    }; // namespace System
}; // namespace Gek
