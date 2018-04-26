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

	#define GEK_PROFILER_DEFAULT getContext(), 0, 0
	#define GEK_PROFILER_CUSTOM(PROCESS, THREAD) getContext(), PROCESS, THREAD

	// Metadata. Call at the start preferably. Must be const strings.
	#define GEK_PROFILER_SET_PROCESS_NAME(PROFILER, PROCESS, NAME) PROFILER->addEvent(PROCESS, 0, "__metadata"sv, "process_name"sv, Gek::Profiler::EmptyTime, Gek::Profiler::EmptyTime, 'M', 0, Gek::Profiler::Arguments{{ "name"sv, NAME }})
	#define GEK_PROFILER_SET_THREAD_NAME(PROFILER, THREAD, NAME) PROFILER->addEvent(0, THREAD, "__metadata"sv, "thread_name"sv, Gek::Profiler::EmptyTime, Gek::Profiler::EmptyTime, 'M', 0, Gek::Profiler::Arguments{{ "name"sv, NAME }})
	#define GEK_PROFILER_SET_THREAD_SORT_INDEX(PROFILER, THREAD, INDEX) PROFILER->addEvent(0, THREAD, "__metadata"sv, "thread_sort_index"sv, Gek::Profiler::EmptyTime, Gek::Profiler::EmptyTime, 'M', 0, Gek::Profiler::Arguments{{ "sort_index"sv, INDEX }})

	// Async events. Can span threads. ID identifies which events to connect in the view.
	#define _GEK_PROFILER_ASYNC_BEGIN(PROFILER, PROCESS, THREAD, CATEGORY, NAME, IDENTIFIER, ARGUMENTS) PROFILER->addEvent(PROCESS, THREAD, CATEGORY, NAME, Gek::Profiler::GetProfilerTime(), Gek::Profiler::EmptyTime, 'S', IDENTIFIER, ARGUMENTS)
	#define _GEK_PROFILER_ASYNC_STEP(PROFILER, PROCESS, THREAD, CATEGORY, NAME, IDENTIFIER, STEP) PROFILER->addEvent(PROCESS, THREAD, CATEGORY, NAME, Gek::Profiler::GetProfilerTime(), Gek::Profiler::EmptyTime, 'T', IDENTIFIER, Gek::Profiler::Arguments{{ "step"sv, STEP }})
	#define _GEK_PROFILER_ASYNC_END(PROFILER, PROCESS, THREAD, CATEGORY, NAME, IDENTIFIER) PROFILER->addEvent(PROCESS, THREAD, CATEGORY, NAME, Gek::Profiler::GetProfilerTime(), Gek::Profiler::EmptyTime, 'F', IDENTIFIER, Gek::Profiler::EmptyArguments)
	#define GEK_PROFILER_ASYNC_BEGIN(...) _GEK_PROFILER_ASYNC_BEGIN(__VA_ARGS__)
	#define GEK_PROFILER_ASYNC_STEP(...) _GEK_PROFILER_ASYNC_STEP(__VA_ARGS__)
	#define GEK_PROFILER_ASYNC_END(...) _GEK_PROFILER_ASYNC_END(__VA_ARGS__)

	// Flow events. Like async events, but displayed in a more fancy way in the viewer.
	#define _GEK_PROFILER_FLOW_BEGIN(PROFILER, PROCESS, THREAD, CATEGORY, NAME, IDENTIFIER, ARGUMENTS) PROFILER->addEvent(PROCESS, THREAD, CATEGORY, NAME, Gek::Profiler::GetProfilerTime(), Gek::Profiler::EmptyTime, 's', IDENTIFIER, ARGUMENTS)
	#define _GEK_PROFILER_FLOW_STEP(PROFILER, PROCESS, THREAD, CATEGORY, NAME, IDENTIFIER, STEP) PROFILER->addEvent(PROCESS, THREAD, CATEGORY, NAME, Gek::Profiler::GetProfilerTime(), Gek::Profiler::EmptyTime, 't', IDENTIFIER, Gek::Profiler::Arguments{{ "step"sv, STEP }})
	#define _GEK_PROFILER_FLOW_END(PROFILER, PROCESS, THREAD, CATEGORY, NAME, IDENTIFIER) PROFILER->addEvent(PROCESS, THREAD, CATEGORY, NAME, Gek::Profiler::GetProfilerTime(), Gek::Profiler::EmptyTime, 'f', IDENTIFIER, Gek::Profiler::EmptyArguments)
	#define GEK_PROFILER_FLOW_BEGIN(...) _GEK_PROFILER_FLOW_BEGIN(__VA_ARGS__)
	#define GEK_PROFILER_FLOW_STEP(...) _GEK_PROFILER_FLOW_STEP(__VA_ARGS__)
	#define GEK_PROFILER_FLOW_END(...) _GEK_PROFILER_FLOW_END(__VA_ARGS__)

	// Instant events. For things with no duration.
	#define _GEK_PROFILER_MOMENT(PROFILER, PROCESS, THREAD, CATEGORY, NAME, ARGUMENTS) PROFILER->addEvent(PROCESS, THREAD, CATEGORY, NAME, Gek::Profiler::GetProfilerTime(), Gek::Profiler::EmptyTime, 'I', 0, ARGUMENTS)
	#define GEK_PROFILER_MOMENT(...) _GEK_PROFILER_MOMENT(__VA_ARGS__)

	// Counters
	#define _GEK_PROFILER_COUNTER(PROFILER, PROCESS, THREAD, CATEGORY, NAME, ARGUMENTS) PROFILER->addEvent(PROCESS, THREAD, CATEGORY, NAME, Gek::Profiler::GetProfilerTime(), Gek::Profiler::EmptyTime, 'C', 0, ARGUMENTS)
	#define GEK_PROFILER_COUNTER(...) _GEK_PROFILER_COUNTER(__VA_ARGS__)

	// Lambda encased scope
	#define _GEK_PROFILER_BEGIN_SCOPE(PROFILER, PROCESS, THREAD, CATEGORY, NAME, ARGUMENTS) Gek::Profiler::Scope(PROFILER, PROCESS, THREAD, CATEGORY, NAME, 0, ARGUMENTS, [&](void) -> void
	#define GEK_PROFILER_BEGIN_SCOPE(...) _GEK_PROFILER_BEGIN_SCOPE(__VA_ARGS__)
	#define GEK_PROFILER_END_SCOPE() )
#else
#endif // GEK_PROFILER_ENABLED
