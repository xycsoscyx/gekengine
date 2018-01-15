/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision: 1143 $
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date: 2016-10-13 13:29:45 -0700 (Thu, 13 Oct 2016) $
#pragma once

#include "GEK/Utility/Hash.hpp"
#include <chrono>

namespace Gek
{
	namespace Profiler
	{
		struct TimeStamp
		{
			enum class Type
			{
				Begin = 0,
				End,
				Duration,
			};

			Type type = Type::Begin;
			Hash nameIdentifier = 0;
			Hash threadIdentifier = 0;
			std::chrono::nanoseconds timeStamp, duration;

			TimeStamp(void) = default;
			TimeStamp(Type type, Hash nameIdentifier, std::chrono::nanoseconds const *timeStamp = nullptr, Hash const *threadIdentifier = nullptr);
			TimeStamp(Hash nameIdentifier, std::chrono::nanoseconds startTime, std::chrono::nanoseconds endTime, Hash const *threadIdentifier = nullptr);
		};

		struct Scope
		{
			Hash nameIdentifier = 0;
			std::chrono::nanoseconds timeStamp;

			Scope(void) = default;
			Scope(Hash nameIdentifier);
			~Scope(void);
		};

		void Initialize(void);
		void Shutdown(char const *outputFileName = nullptr);
		Hash RegisterName(std::string_view name);
	}; // namespace Profiler
}; // namespace Gek

#define GEK_PROFILE_EVENT(PROFILER, NAME, THREAD, START_TIME, END_TIME) Gek::System::Profiler::TimeStamp(NAME, START_TIME, END_TIME, &THREAD))

#define _GEK_PROFILER_NAME(NAME, LINE) __profiler_name_##LINE

#define GEK_PROFILE_FUNCTION_SCOPE() \
	static const auto _GEK_PROFILER_NAME(__FUNCTION__, __LINE__) = Profiler::RegisterName(__FUNCTION__); \
	Profiler::Scope __profiler_scope(_GEK_PROFILER_NAME(__FUNCTION__, __LINE__));

#define GEK_PROFILE_AUTO_SCOPE(NAME) \
	static const auto _GEK_PROFILER_NAME(NAME, __LINE__) = Profiler::RegisterName(NAME); \
	Profiler::Scope __profiler_scope(_GEK_PROFILER_NAME(NAME, __LINE__));

#define GEK_PROFILE_BEGIN_SCOPE(NAME) \
    [&](void) -> void \
    { \
		static const auto _GEK_PROFILER_NAME(NAME, __LINE__) = Profiler::RegisterName(NAME); \
		Profiler::Scope __profiler_scope(_GEK_PROFILER_NAME(NAME, __LINE__));

#define GEK_PROFILE_END_SCOPE() \
    }();
