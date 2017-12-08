/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision: 1143 $
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date: 2016-10-13 13:29:45 -0700 (Thu, 13 Oct 2016) $
#pragma once

#include <easy/profiler.h>

#define GEK_PROFILE_ENABLE() EASY_PROFILER_ENABLE
#define GEK_PROFILE_START() profiler::startListen()
#define GEK_PROFILE_STOP() profiler::stopListen()
#define GEK_PROFILE_DUMP(NAME) profiler::dumpBlocksToFile(NAME)

#define GEK_PROFILE_AUTO_SCOPE(NAME) EASY_BLOCK(NAME)
#define GEK_PROFILE_FUNCTION(...) EASY_FUNCTION(__VA_ARGS__)
#define GEK_PROFILE_BEGIN_SCOPE(NAME) \
    [&](void) -> void \
    { \
        EASY_BLOCK(NAME);

#define GEK_PROFILE_END_SCOPE() \
        EASY_END_BLOCK; \
    }()
