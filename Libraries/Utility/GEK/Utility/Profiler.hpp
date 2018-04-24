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
#include <variant>
#include <chrono>

#define GEK_PROFILER_ENABLED

namespace Gek
{
	class Profiler
	{
	public:
		using TimeFormat = std::chrono::microseconds;
		using Argument = std::variant<std::string, std::string_view, int32_t, uint32_t, int64_t, uint64_t, float, Hash>;
		using Arguments = std::unordered_map<std::string_view, Argument>;

		static const TimeFormat EmptyTime;
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

		Hash getCurrentThreadIdentifier(void);

		void addEvent(Hash processIdentifier, Hash threadIdentifier, std::string_view category, std::string_view name, TimeFormat startTime, TimeFormat duration, char eventType, Hash eventIdentifier, Arguments const &arguments);

		template <typename FUNCTION, typename... ARGUMENTS>
		static auto Scope(Profiler *profiler, Hash processIdentifier, Hash threadIdentifier, std::string_view category, std::string_view name, Hash eventIdentifier, Arguments const &arguments, FUNCTION function, ARGUMENTS&&... functionArguments) -> void
		{
			auto startTime = GetProfilerTime();
			function(std::forward<ARGUMENTS>(functionArguments)...);
			profiler->addEvent(processIdentifier, threadIdentifier, category, name, startTime, (GetProfilerTime() - startTime), 'X', eventIdentifier, arguments);
		}
	}; // namespace Profiler
}; // namespace Gek

#ifdef GEK_PROFILER_ENABLED
	// CATEGORY - category. Can be filtered by in trace viewer (or at least that's the intention).
	//     A good use is to pass __FILE__, there are macros further below that will do it for you.
	// NAME - name. Pass __FUNCTION__ in most cases, unless you are marking up parts of one.

	// Metadata. Call at the start preferably. Must be const strings.
	#define GEK_PROFILER_SET_PROCESS_NAME(PROFILER, NAME) PROFILER->addEvent(0, 0, "__metadata"sv, "process_name"sv, Gek::Profiler::EmptyTime, Gek::Profiler::EmptyTime, 'M', 0, Gek::Profiler::Arguments{{ "name"sv, NAME }})
	#define GEK_PROFILER_SET_THREAD_NAME(PROFILER, THREAD, NAME) PROFILER->addEvent(0, THREAD, "__metadata"sv, "thread_name"sv, Gek::Profiler::EmptyTime, Gek::Profiler::EmptyTime, 'M', 0, Gek::Profiler::Arguments{{ "name"sv, NAME }})
	#define GEK_PROFILER_SET_THREAD_SORT_INDEX(PROFILER, THREAD, INDEX) PROFILER->addEvent(0, THREAD, "__metadata"sv, "thread_sort_index"sv, Gek::Profiler::EmptyTime, Gek::Profiler::EmptyTime, 'M', 0, Gek::Profiler::Arguments{{ "sort_index"sv, INDEX }})
	#define GEK_PROFILER_SET_CURRENT_THREAD_NAME(PROFILER, NAME) PROFILER->addEvent(0, PROFILER->getCurrentThreadIdentifier(), "__metadata"sv, "thread_name"sv, Gek::Profiler::EmptyTime, Gek::Profiler::EmptyTime, 'M', 0, Gek::Profiler::Arguments{{ "name"sv, NAME }})
	#define GEK_PROFILER_SET_CURRENT_THREAD_SORT_INDEX(PROFILER, INDEX) PROFILER->addEvent(0, PROFILER->getCurrentThreadIdentifier(), "__metadata"sv, "thread_sort_index"sv, Gek::Profiler::EmptyTime, Gek::Profiler::EmptyTime, 'M', 0, Gek::Profiler::Arguments{{ "sort_index"sv, INDEX }})

	// Async events. Can span threads. ID identifies which events to connect in the view.
	#define GEK_PROFILER_ASYNC_BEGIN(PROFILER, CATEGORY, NAME, IDENTIFIER, ...) PROFILER->addEvent(0, PROFILER->getCurrentThreadIdentifier(), CATEGORY, NAME, Gek::Profiler::GetProfilerTime(), Gek::Profiler::EmptyTime, 'S', IDENTIFIER, __VA_ARGS__)
	#define GEK_PROFILER_ASYNC_STEP(PROFILER, CATEGORY, NAME, IDENTIFIER, STEP) PROFILER->addEvent(0, PROFILER->getCurrentThreadIdentifier(), CATEGORY, NAME, Gek::Profiler::GetProfilerTime(), Gek::Profiler::EmptyTime, 'T', IDENTIFIER, Gek::Profiler::Arguments{{ "STEP"s, STEP }})
	#define GEK_PROFILER_ASYNC_END(PROFILER, CATEGORY, NAME, IDENTIFIER) PROFILER->addEvent(0, PROFILER->getCurrentThreadIdentifier(), CATEGORY, NAME, Gek::Profiler::GetProfilerTime(), Gek::Profiler::EmptyTime, 'F', IDENTIFIER, Gek::Profiler::EmptyArguments)

	// Flow events. Like async events, but displayed in a more fancy way in the viewer.
	#define GEK_PROFILER_FLOW_BEGIN(PROFILER, CATEGORY, NAME, IDENTIFIER, ...) PROFILER->addEvent(0, PROFILER->getCurrentThreadIdentifier(), CATEGORY, NAME, Gek::Profiler::GetProfilerTime(), Gek::Profiler::EmptyTime, 's', IDENTIFIER, __VA_ARGS__)
	#define GEK_PROFILER_FLOW_STEP(PROFILER, CATEGORY, NAME, IDENTIFIER, STEP) PROFILER->addEvent(0, PROFILER->getCurrentThreadIdentifier(), CATEGORY, NAME, Gek::Profiler::GetProfilerTime(), Gek::Profiler::EmptyTime, 't', IDENTIFIER, Gek::Profiler::Arguments{{ "STEP"s, STEP }})
	#define GEK_PROFILER_FLOW_END(PROFILER, CATEGORY, NAME, IDENTIFIER) PROFILER->addEvent(0, PROFILER->getCurrentThreadIdentifier(), CATEGORY, NAME, Gek::Profiler::GetProfilerTime(), Gek::Profiler::EmptyTime, 'f', IDENTIFIER, Gek::Profiler::EmptyArguments)

	// Instant events. For things with no duration.
	#define GEK_PROFILER_MOMENT(PROFILER, CATEGORY, NAME, ...) PROFILER->addEvent(0, PROFILER->getCurrentThreadIdentifier(), CATEGORY, NAME, Gek::Profiler::GetProfilerTime(), Gek::Profiler::EmptyTime, 'I', 0, __VA_ARGS__)

	// Counters
	#define GEK_PROFILER_COUNTER(PROFILER, CATEGORY, NAME, ...) PROFILER->addEvent(0, PROFILER->getCurrentThreadIdentifier(), CATEGORY, NAME, Gek::Profiler::GetProfilerTime(), Gek::Profiler::EmptyTime, 'C', 0, __VA_ARGS__)

	#define GEK_PROFILER_BEGIN_SCOPE(PROFILER, NAME) Gek::Profiler::Scope(PROFILER, 0, PROFILER->getCurrentThreadIdentifier(), __FILE__, NAME, 0, Gek::Profiler::EmptyArguments, [&](void) -> void
	#define GEK_PROFILER_BEGIN_SCOPE_ARGUMENTS(PROFILER, NAME, ARGUMENTS) Gek::Profiler::Scope(PROFILER, 0, PROFILER->getCurrentThreadIdentifier(), __FILE__, NAME, 0, ARGUMENTS, [&](void) -> void
	#define GEK_PROFILER_END_SCOPE() )

	#define GEK_PROFILER_BEGIN_CUSTOM_SCOPE(PROFILER, NAME, THREAD) Gek::Profiler::Scope(PROFILER, 0, THREAD, __FILE__, NAME, 0, Gek::Profiler::EmptyArguments, [&](void) -> void
	#define GEK_PROFILER_BEGIN_CUSTOM_SCOPE_ARGUMENTS(PROFILER, NAME, THREAD, ARGUMENTS) Gek::Profiler::Scope(PROFILER, 0, THREAD, __FILE__, NAME, 0, ARGUMENTS, [&](void) -> void
	#define GEK_PROFILER_END_CUSTOM_SCOPE() )
#else
#endif // GEK_PROFILER_ENABLED
