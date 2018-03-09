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

#define INTERNAL_MINITRACE_BUFFER_SIZE 1000000
#define GEK_PROFILER_ENABLED

namespace Gek
{
	namespace Profiler
	{
		void Initialize(std::string_view fileName);
		void Shutdown(void);
		void Pause(void);
		void Resume(void);
		void Flush(void);
		double GetTime(void);
		std::string_view GetPoolString(std::string_view name);

		void AddEvent(std::string_view category, std::string_view name, char ph, void *id);
		void AddEvent(std::string_view category, std::string_view name, char ph, void *id, std::string_view argumentName, JSON::Reference argumentValue);

#ifdef GEK_PROFILER_ENABLED
		// Only outputs a block if execution time exceeded the limit.
		// TODO: This will effectively call GetTime twice at the end, which is bad.
		class LimitedTrace
		{
		private:
			std::string_view category;
			std::string_view name;
			double startTime;
			double timeLimit;

		public:
			LimitedTrace(std::string_view category, std::string_view name, double timeLimit)
				: category(category)
				, name(name)
				, timeLimit(timeLimit)
				, startTime(GetTime())
			{
			}

			~LimitedTrace()
			{
				double endTime = GetTime();
				if ((endTime - startTime) >= timeLimit)
				{
					AddEvent(category, name, 'X', &startTime);
				}
			}
		};

		class ScopedTrace
		{
		private:
			std::string_view category;
			std::string_view name;
			double startTime;

		public:
			ScopedTrace(std::string_view category, std::string_view name)
				: category(category)
				, name(name)
				, startTime(GetTime())
			{
			}

			~ScopedTrace()
			{
				AddEvent(category, name, 'X', &startTime);
			}
		};

		class ValueTrace
		{
		private:
			std::string_view category;
			std::string_view name;

		public:
			ValueTrace(std::string_view category, std::string_view name, std::string_view argumentName, JSON::Reference value)
				: category(category)
				, name(name)
			{
				AddEvent(category, name, 'B', 0, argumentName, value);
			}

			~ValueTrace()
			{
				AddEvent(category, name, 'E', 0);
			}
		};
#endif // GEK_PROFILER_ENABLED
	}; // namespace Profiler
}; // namespace Gek

#ifdef GEK_PROFILER_ENABLED
	// c - category. Can be filtered by in trace viewer (or at least that's the intention).
	//     A good use is to pass __FILE__, there are macros further below that will do it for you.
	// n - name. Pass __FUNCTION__ in most cases, unless you are marking up parts of one.

	// Scopes. In C++, use GEK_PROFILER_SCOPE. In C, always match them within the same scope.
	#define GEK_PROFILER_BEGIN(c, n) AddEvent(c, n, 'B', 0)
	#define GEK_PROFILER_END(c, n) AddEvent(c, n, 'E', 0)
	#define GEK_PROFILER_SCOPE(c, n) Gek::Profiler::ScopedTrace ____GEK_PROFILER_scope(c, n)
	#define GEK_PROFILER_SCOPE_LIMIT(c, n, l) Gek::Profiler::Gek::Profiler::LimitedTrace ____GEK_PROFILER_scope(c, n, l)

	// Async events. Can span threads. ID identifies which events to connect in the view.
	#define GEK_PROFILER_START(c, n, id) AddEvent(c, n, 'S', JSON::Object(id))
	#define GEK_PROFILER_STEP(c, n, id, step) AddEvent(c, n, 'T', JSON::Object(id), ArgumentType::StringView, "step"s, JSON::Object(step))
	#define GEK_PROFILER_FINISH(c, n, id) AddEvent(c, n, 'F', JSON::Object(id))

	// Flow events. Like async events, but displayed in a more fancy way in the viewer.
	#define GEK_PROFILER_FLOW_START(c, n, id) AddEvent(c, n, 's', JSON::Object(id))
	#define GEK_PROFILER_FLOW_STEP(c, n, id, step) AddEvent(c, n, 't', JSON::Object(id), ArgumentType::StringView, "step"s, JSON::Object(step))
	#define GEK_PROFILER_FLOW_FINISH(c, n, id) AddEvent(c, n, 'f', JSON::Object(id))

	// The same macros, but with a single named argument which shows up as metadata in the viewer.
	// _I for int.
	// _VALUE is for a const string arg.
	// _S will copy the string, freeing on flush (expensive but sometimes necessary).
	// but required if the string was generated dynamically.

	// Note that it's fine to match BEGIN_S with END and BEGIN with END_S, etc.
	#define GEK_PROFILER_BEGIN_VALUE(c, n, aname, v) AddEvent(c, n, 'B', 0, aname, JSON::Object(v))
	#define GEK_PROFILER_END_VALUE(c, n, aname, v) AddEvent(c, n, 'E', 0, aname, JSON::Object(v))
	#define GEK_PROFILER_SCOPE_VALUE(c, n, aname, v) Gek::Profiler::ValueTrace ____GEK_PROFILER_scope(c, n, aname, JSON::Object(v))

	// Instant events. For things with no duration.
	#define GEK_PROFILER_INSTANT(c, n) AddEvent(c, n, 'I', 0)
	#define GEK_PROFILER_INSTANT_VALUE(c, n, aname, v) AddEvent(c, n, 'I', 0, aname, JSON::Object(v))

	// Counters (can't do multi-value counters yet)
	#define GEK_PROFILER_COUNTER(c, n, val) AddEvent(c, n, 'C', 0, n, JSON::Object(val))

	// Metadata. Call at the start preferably. Must be const strings.
	#define GEK_PROFILER_META_PROCESS_NAME(n) AddEvent(""s, "process_name"s, 'M', 0, "name"s, JSON::Object(n))
	#define GEK_PROFILER_META_THREAD_NAME(n) AddEvent(""s, "thread_name"s, 'M', 0, "name"s, JSON::Object(n))
	#define GEK_PROFILER_META_THREAD_SORT_INDEX(i) AddEvent(""s, "thread_sort_index"s, 'M', 0, "sort_index"s, JSON::Object(i))
#else
	#define GEK_PROFILER_BEGIN(c, n)
	#define GEK_PROFILER_END(c, n)
	#define GEK_PROFILER_SCOPE(c, n)
	#define GEK_PROFILER_START(c, n, id)
	#define GEK_PROFILER_STEP(c, n, id, step)
	#define GEK_PROFILER_FINISH(c, n, id)
	#define GEK_PROFILER_FLOW_START(c, n, id)
	#define GEK_PROFILER_FLOW_STEP(c, n, id, step)
	#define GEK_PROFILER_FLOW_FINISH(c, n, id)
	#define GEK_PROFILER_INSTANT(c, n)

	#define GEK_PROFILER_BEGIN_VALUE(c, n, aname, astrval)
	#define GEK_PROFILER_END_VALUE(c, n, aname, astrval)
	#define GEK_PROFILER_SCOPE_VALUE(c, n, aname, astrval)

	#define GEK_PROFILER_INSTANT(c, n)
	#define GEK_PROFILER_INSTANT_VALUE(c, n, aname, astrval)

	// Counters (can't do multi-value counters yet)
	#define GEK_PROFILER_COUNTER(c, n, val)

	// Metadata. Call at the start preferably. Must be const strings.
	#define GEK_PROFILER_META_PROCESS_NAME(n)

	#define GEK_PROFILER_META_THREAD_NAME(n)
	#define GEK_PROFILER_META_THREAD_SORT_INDEX(i)
#endif // GEK_PROFILER_ENABLED

// Shortcuts for simple function timing with automatic categories and names.
#define GEK_PROFILER_BEGIN_FUNC() GEK_PROFILER_BEGIN(__FILE__, __FUNCTION__)
#define GEK_PROFILER_END_FUNC() GEK_PROFILER_END(__FILE__, __FUNCTION__)
#define GEK_PROFILER_SCOPE_FUNC() GEK_PROFILER_SCOPE(__FILE__, __FUNCTION__)
#define GEK_PROFILER_INSTANT_FUNC() GEK_PROFILER_INSTANT(__FILE__, __FUNCTION__)
#define GEK_PROFILER_SCOPE_FUNC_LIMIT_S(l) Gek::Profiler::LimitedTrace ____GEK_PROFILER_scope(__FILE__, __FUNCTION__, l)
#define GEK_PROFILER_SCOPE_FUNC_LIMIT_MS(l) Gek::Profiler::LimitedTrace ____GEK_PROFILER_scope(__FILE__, __FUNCTION__, (double)l * 0.000001)

// Same, but with a single argument of the usual types.
#define GEK_PROFILER_BEGIN_FUNC_VALUE(aname, arg) GEK_PROFILER_BEGIN_VALUE(__FILE__, __FUNCTION__, aname, arg)
#define GEK_PROFILER_END_FUNC_VALUE(aname, arg) GEK_PROFILER_END_VALUE(__FILE__, __FUNCTION__, aname, arg)
#define GEK_PROFILER_SCOPE_FUNC_VALUE(aname, arg) GEK_PROFILER_SCOPE_VALUE(__FILE__, __FUNCTION__, aname, arg)

#define GEK_PROFILE_FUNCTION_SCOPE() GEK_PROFILER_SCOPE(__FILE__, __FUNCTION__);
#define GEK_PROFILE_AUTO_SCOPE(CATEGORY) GEK_PROFILER_SCOPE(__FILE__, CATEGORY);
#define GEK_PROFILE_BEGIN_SCOPE(CATEGORY) do { GEK_PROFILER_SCOPE(__FILE__, CATEGORY);
#define GEK_PROFILE_END_SCOPE() } while(false);
