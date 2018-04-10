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
#include "GEK/Utility/ThreadPool.hpp"
#include <concurrent_unordered_map.h>
#include <concurrent_vector.h>
#include <inttypes.h>
#include <chrono>
#include <any>

#define GEK_PROFILER_ENABLED

namespace Gek
{
	class Profiler
	{
	public:
		using TimeFormat = std::chrono::microseconds;
		using Arguments = std::unordered_map<std::string_view, std::any>;
		static const Arguments EmptyArguments;

	private:
		struct Data;
		std::unique_ptr<Data> data;

	public:
		Profiler(std::string_view fileName = String::Empty);
		~Profiler(void);

		static const TimeFormat GetProfilerTime(void)
		{
			auto currentTime = std::chrono::high_resolution_clock::now().time_since_epoch();
			return std::chrono::duration_cast<TimeFormat>(currentTime);
		}

		void addEvent(std::string_view category, std::string_view name, char eventType, uint64_t eventIdentifier = 0, Hash *threadIdentifier = nullptr, Arguments const &arguments = EmptyArguments);
		void addSpan(std::string_view category, std::string_view name, TimeFormat startTime, TimeFormat endTime, Hash *threadIdentifier = nullptr);

		template <typename FUNCTION, typename... ARGUMENTS>
		static auto Scope(Profiler *profiler, std::string_view category, std::string_view name, FUNCTION function, ARGUMENTS&&... arguments) -> void
		{
#ifdef GEK_PROFILER_ENABLED
			auto startTime = GetProfilerTime();
			function(std::forward<ARGUMENTS>(arguments)...);
			profiler->addSpan(category, name, startTime, GetProfilerTime());
#endif // GEK_PROFILER_ENABLED
		}
	}; // namespace Profiler
}; // namespace Gek

#ifdef GEK_PROFILER_ENABLED
	// CATEGORY - category. Can be filtered by in trace viewer (or at least that's the intention).
	//     A good use is to pass __FILE__, there are macros further below that will do it for you.
	// NAME - name. Pass __FUNCTION__ in most cases, unless you are marking up parts of one.

	// Scopes. In C++, use GEK_PROFILER_SCOPE. In C, always match them within the same scope.
	#define GEK_PROFILER_BEGIN(CATEGORY, NAME) getContext()->addEvent(CATEGORY, NAME, 'B')
	#define GEK_PROFILER_END(CATEGORY, NAME) getContext()->addEvent(CATEGORY, NAME, 'E')

	// Async events. Can span threads. ID identifies which events to connect in the view.
	#define GEK_PROFILER_START(CATEGORY, NAME, IDENTIFIER) getContext()->addEvent(CATEGORY, NAME, 'S', IDENTIFIER)
	#define GEK_PROFILER_STEP(CATEGORY, NAME, IDENTIFIER, STEP) getContext()->addEvent(CATEGORY, NAME, 'T', IDENTIFIER, "STEP"s, STEP)
	#define GEK_PROFILER_FINISH(CATEGORY, NAME, IDENTIFIER) getContext()->addEvent(CATEGORY, NAME, 'F', IDENTIFIER)

	// Flow events. Like async events, but displayed in a more fancy way in the viewer.
	#define GEK_PROFILER_FLOW_START(CATEGORY, NAME, IDENTIFIER) getContext()->addEvent(CATEGORY, NAME, 's', IDENTIFIER)
	#define GEK_PROFILER_FLOW_STEP(CATEGORY, NAME, IDENTIFIER, STEP) getContext()->addEvent(CATEGORY, NAME, 't', IDENTIFIER, "STEP"s, STEP)
	#define GEK_PROFILER_FLOW_FINISH(CATEGORY, NAME, IDENTIFIER) getContext()->addEvent(CATEGORY, NAME, 'f', IDENTIFIER)

	// Note that it's fine to match BEGIN_S with END and BEGIN with END_S, etc.
	#define GEK_PROFILER_BEGIN_VALUE(CATEGORY, NAME, ARGUMENT, VALUE) getContext()->addEvent(CATEGORY, NAME, 'B', 0, ARGUMENT, VALUE)
	#define GEK_PROFILER_END_VALUE(CATEGORY, NAME, ARGUMENT, VALUE) getContext()->addEvent(CATEGORY, NAME, 'E', 0, ARGUMENT, VALUE)

	// Instant events. For things with no duration.
	#define GEK_PROFILER_INSTANT(CATEGORY, NAME) getContext()->addEvent(CATEGORY, NAME, 'I', 0)
	#define GEK_PROFILER_INSTANT_VALUE(CATEGORY, NAME, ARGUMENT, VALUE) getContext()->addEvent(CATEGORY, NAME, 'I', 0, ARGUMENT, VALUE)

	// Counters (can't do multi-value counters yet)
	#define GEK_PROFILER_COUNTER(CATEGORY, NAME, VALUE) getContext()->addEvent(CATEGORY, NAME, 'C', 0, NAME, VALUE)

	// Metadata. Call at the start preferably. Must be const strings.
	#define GEK_PROFILER_META_PROCESS_NAME(NAME) getContext()->addEvent(""sv, "process_name"sv, 'M', 0, nullptr, Gek::Profiler::Arguments({ "name"sv, NAME }))
	#define GEK_PROFILER_META_THREAD_NAME(NAME) getContext()->addEvent(""sv, "thread_name"sv, 'M', 0, nullptr, Gek::Profiler::Arguments({ "name"sv, NAME }))
	#define GEK_PROFILER_META_NAME(NAME, HASH) getContext()->addEvent(""sv, "thread_name"sv, 'M', 0, &HASH, Gek::Profiler::Arguments({ "name"sv, NAME }))
	#define GEK_PROFILER_META_THREAD_SORT_INDEX(INDEX) getContext()->addEvent(""sv, "thread_sort_index"sv, 'M', 0, nullptr, Gek::Profiler::Arguments({ "sort_index"sv, NAME }))
#else
	#define GEK_PROFILER_BEGIN(CATEGORY, NAME)
	#define GEK_PROFILER_END(CATEGORY, NAME)
	#define GEK_PROFILER_START(CATEGORY, NAME, IDENTIFIER)
	#define GEK_PROFILER_STEP(CATEGORY, NAME, IDENTIFIER, STEP)
	#define GEK_PROFILER_FINISH(CATEGORY, NAME, IDENTIFIER)
	#define GEK_PROFILER_FLOW_START(CATEGORY, NAME, IDENTIFIER)
	#define GEK_PROFILER_FLOW_STEP(CATEGORY, NAME, IDENTIFIER, STEP)
	#define GEK_PROFILER_FLOW_FINISH(CATEGORY, NAME, IDENTIFIER)
	#define GEK_PROFILER_BEGIN_VALUE(CATEGORY, NAME, ARGUMENT, VALUE)
	#define GEK_PROFILER_END_VALUE(CATEGORY, NAME, ARGUMENT, VALUE)
	#define GEK_PROFILER_INSTANT(CATEGORY, NAME)
	#define GEK_PROFILER_INSTANT_VALUE(CATEGORY, NAME, ARGUMENT, VALUE)
	#define GEK_PROFILER_COUNTER(CATEGORY, NAME, VALUE)
	#define GEK_PROFILER_META_PROCESS_NAME(NAME)
	#define GEK_PROFILER_META_THREAD_NAME(NAME)
	#define GEK_PROFILER_META_NAME(NAME, HASH)
#define GEK_PROFILER_META_THREAD_SORT_INDEX(INDEX)
#endif // GEK_PROFILER_ENABLED

#define GEK_PROFILER_FUNCTION_SCOPE() GEK_PROFILER_SCOPE(__FILE__, __FUNCTION__);
#define GEK_PROFILER_AUTO_SCOPE(NAME) GEK_PROFILER_SCOPE(__FILE__, NAME);
#define GEK_PROFILER_BEGIN_SCOPE(NAME) Gek::Profiler::Scope(getContext(), __FILE__, NAME, [&](void) -> void
#define GEK_PROFILER_END_SCOPE() )