/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision: 1143 $
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date: 2016-10-13 13:29:45 -0700 (Thu, 13 Oct 2016) $
#pragma once

#include "GEK/Utility/Hash.hpp"
#include "GEK/Utility/JSON.hpp"
#include <inttypes.h>
#include <chrono>
#include <any>

#define GEK_PROFILER_ENABLED

namespace Gek
{
	namespace Profiler
	{
		void Initialize(std::string_view fileName);
		void Shutdown(void);
		std::string_view GetPoolString(std::string_view name);

		void AddEvent(std::string_view category, std::string_view name, char ph, uint64_t id = 0, std::string_view argument = nullptr, std::any value = nullptr);
		void AddSpan(std::string_view category, std::string_view name, std::chrono::nanoseconds startTime, std::chrono::nanoseconds endTime);

#ifdef GEK_PROFILER_ENABLED
		class ScopedTrace
		{
		private:
			std::string_view category;
			std::string_view name;
			std::chrono::nanoseconds startTime;

		public:
			ScopedTrace(std::string_view category, std::string_view name)
				: category(category)
				, name(name)
				, startTime(std::chrono::high_resolution_clock::now().time_since_epoch())
			{
			}

			~ScopedTrace()
			{
				AddSpan(category, name, startTime, std::chrono::high_resolution_clock::now().time_since_epoch());
			}
		};
#endif // GEK_PROFILER_ENABLED
	}; // namespace Profiler
}; // namespace Gek

#ifdef GEK_PROFILER_ENABLED
	// CATEGORY - category. Can be filtered by in trace viewer (or at least that's the intention).
	//     A good use is to pass __FILE__, there are macros further below that will do it for you.
	// NAME - name. Pass __FUNCTION__ in most cases, unless you are marking up parts of one.

	// Scopes. In C++, use GEK_PROFILER_SCOPE. In C, always match them within the same scope.
	#define GEK_PROFILER_BEGIN(CATEGORY, NAME) AddEvent(CATEGORY, NAME, 'B')
	#define GEK_PROFILER_END(CATEGORY, NAME) AddEvent(CATEGORY, NAME, 'E')
	#define GEK_PROFILER_SCOPE(CATEGORY, NAME) Gek::Profiler::ScopedTrace ____GEK_PROFILER_scope(CATEGORY, NAME)

	// Async events. Can span threads. ID identifies which events to connect in the view.
	#define GEK_PROFILER_START(CATEGORY, NAME, IDENTIFIER) AddEvent(CATEGORY, NAME, 'S', IDENTIFIER)
	#define GEK_PROFILER_STEP(CATEGORY, NAME, IDENTIFIER, STEP) AddEvent(CATEGORY, NAME, 'T', IDENTIFIER, "STEP"s, STEP)
	#define GEK_PROFILER_FINISH(CATEGORY, NAME, IDENTIFIER) AddEvent(CATEGORY, NAME, 'F', IDENTIFIER)

	// Flow events. Like async events, but displayed in a more fancy way in the viewer.
	#define GEK_PROFILER_FLOW_START(CATEGORY, NAME, IDENTIFIER) AddEvent(CATEGORY, NAME, 's', IDENTIFIER)
	#define GEK_PROFILER_FLOW_STEP(CATEGORY, NAME, IDENTIFIER, STEP) AddEvent(CATEGORY, NAME, 't', IDENTIFIER, "STEP"s, STEP)
	#define GEK_PROFILER_FLOW_FINISH(CATEGORY, NAME, IDENTIFIER) AddEvent(CATEGORY, NAME, 'f', IDENTIFIER)

	// Note that it's fine to match BEGIN_S with END and BEGIN with END_S, etc.
	#define GEK_PROFILER_BEGIN_VALUE(CATEGORY, NAME, ARGUMENT, VALUE) AddEvent(CATEGORY, NAME, 'B', 0, ARGUMENT, VALUE)
	#define GEK_PROFILER_END_VALUE(CATEGORY, NAME, ARGUMENT, VALUE) AddEvent(CATEGORY, NAME, 'E', 0, ARGUMENT, VALUE)

	// Instant events. For things with no duration.
	#define GEK_PROFILER_INSTANT(CATEGORY, NAME) AddEvent(CATEGORY, NAME, 'I', 0)
	#define GEK_PROFILER_INSTANT_VALUE(CATEGORY, NAME, ARGUMENT, VALUE) AddEvent(CATEGORY, NAME, 'I', 0, ARGUMENT, VALUE)

	// Counters (can't do multi-value counters yet)
	#define GEK_PROFILER_COUNTER(CATEGORY, NAME, VALUE) AddEvent(CATEGORY, NAME, 'C', 0, NAME, VALUE)

	// Metadata. Call at the start preferably. Must be const strings.
	#define GEK_PROFILER_META_PROCESS_NAME(NAME) AddEvent(""s, "process_name"s, 'M', 0, "name"s, NAME)
	#define GEK_PROFILER_META_THREAD_NAME(NAME) AddEvent(""s, "thread_name"s, 'M', 0, "name"s, NAME)
	#define GEK_PROFILER_META_THREAD_SORT_INDEX(i) AddEvent(""s, "thread_sort_index"s, 'M', 0, "sort_index"s, i)
#else
	#define GEK_PROFILER_BEGIN(CATEGORY, NAME)
	#define GEK_PROFILER_END(CATEGORY, NAME)
	#define GEK_PROFILER_SCOPE(CATEGORY, NAME)
	#define GEK_PROFILER_START(CATEGORY, NAME, IDENTIFIER)
	#define GEK_PROFILER_STEP(CATEGORY, NAME, IDENTIFIER, STEP)
	#define GEK_PROFILER_FINISH(CATEGORY, NAME, IDENTIFIER)
	#define GEK_PROFILER_FLOW_START(CATEGORY, NAME, IDENTIFIER)
	#define GEK_PROFILER_FLOW_STEP(CATEGORY, NAME, IDENTIFIER, STEP)
	#define GEK_PROFILER_FLOW_FINISH(CATEGORY, NAME, IDENTIFIER)
	#define GEK_PROFILER_INSTANT(CATEGORY, NAME)

	#define GEK_PROFILER_BEGIN_VALUE(CATEGORY, NAME, ARGUMENT, astrval)
	#define GEK_PROFILER_END_VALUE(CATEGORY, NAME, ARGUMENT, astrval)
	#define GEK_PROFILER_SCOPE_VALUE(CATEGORY, NAME, ARGUMENT, astrval)

	#define GEK_PROFILER_INSTANT(CATEGORY, NAME)
	#define GEK_PROFILER_INSTANT_VALUE(CATEGORY, NAME, ARGUMENT, astrval)

	// Counters (can't do multi-value counters yet)
	#define GEK_PROFILER_COUNTER(CATEGORY, NAME, VALUE)

	// Metadata. Call at the start preferably. Must be const strings.
	#define GEK_PROFILER_META_PROCESS_NAME(NAME)

	#define GEK_PROFILER_META_THREAD_NAME(NAME)
	#define GEK_PROFILER_META_THREAD_SORT_INDEX(i)
#endif // GEK_PROFILER_ENABLED

#define GEK_PROFILER_FUNCTION_SCOPE() GEK_PROFILER_SCOPE(__FILE__, __FUNCTION__);
#define GEK_PROFILER_AUTO_SCOPE(NAME) GEK_PROFILER_SCOPE(__FILE__, NAME);
#define GEK_PROFILER_BEGIN_SCOPE(NAME) do { GEK_PROFILER_SCOPE(__FILE__, NAME);
#define GEK_PROFILER_END_SCOPE() } while(false);
