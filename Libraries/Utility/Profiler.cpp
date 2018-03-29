#include "GEK/Utility/Profiler.hpp"
#include "GEK/Utility/String.hpp"
#include <iostream>
#include <fstream>

namespace Gek
{
#ifdef GEK_PROFILER_ENABLED
	Hash GetThreadIdentifier(void)
	{
		return std::hash<std::thread::id>()(std::this_thread::get_id());;

		std::stringstream thread;
		thread << std::this_thread::get_id();
		return std::stoull(thread.str());
	}

	thread_local Hash currentThreadIdentifier = 0;

	Profiler::Profiler(std::string_view fileName)
		: processIdentifier(GetCurrentProcessId())
		, mainThreadIdentifier(GetThreadIdentifier())
	{
		startTime = std::chrono::high_resolution_clock::now().time_since_epoch();

		fileOutput.open(fileName.empty() ? String::Format("profile_{}.json", processIdentifier).data() : fileName.data());
		fileOutput <<
			"{\n" \
			"\t\"traceEvents\": [";
		fileOutput <<
			"\n" \
			"\t\t{" \
			"\"name\": \"" << "Profiler" << "\"" \
			", \"ph\": \"B\"" \
			", \"pid\": \"" << processIdentifier << "\"" \
			", \"tid\": \"" << "MainThread" << "\"" \
			", \"ts\": 0" <<
			"},\n";
	}

	Profiler::~Profiler(void)
	{
		flush();
		writePool.drain(true);

		fileOutput <<
			"\t\t{" \
			"\"name\": \"" << "Profiler" << "\"" \
			", \"ph\": \"E\"" \
			", \"pid\": \"" << processIdentifier << "\"" \
			", \"tid\": \"" << "MainThread" << "\"" \
			", \"ts\": " << (std::chrono::high_resolution_clock::now().time_since_epoch() - startTime).count() <<
			"}\n";
		fileOutput <<
			"\n" \
			"\t],\n" \
			"\t\"displayTimeUnit\": \"ns\",\n" \
			"\t\"systemTraceEvents\": \"SystemTraceData\",\n" \
			"\t\"otherData\": {\n" \
			"\t\t\"version\": \"GEK Profile TimeStamp v1.0\"\n" \
			"\t}\n" \
			"}\n";
		fileOutput.close();
	}

	void Profiler::flush(void)
	{
		if (eventList.size() > 100)
		{
			concurrency::concurrent_vector<Event> flushList;
			eventList.swap(flushList);
			eventList.reserve(100);

			if (!flushList.empty())
			{
				writePool.enqueueAndDetach([this, flushList = std::move(flushList)](void) -> void
				{
					for (auto &eventData : flushList)
					{
						std::ostringstream eventOutput;
						eventOutput << "\t\t{\"category\": \"" << eventData.category << "\"";
						eventOutput << ", \"name\": \"" << eventData.name << "\"";
						eventOutput << ", \"ts\": " << eventData.ts.count();
						eventOutput << ", \"ph\": \"" << eventData.ph << "\"";
						switch (eventData.ph)
						{
						case 'S':
						case 'T':
						case 'F':
							eventOutput << ", \"id\": " << eventData.id;
							break;

						case 'X':
							eventOutput << ", \"dur\": " << eventData.dur.count();
							break;
						};

						if (!eventData.argument.empty() && eventData.value.has_value())
						{
							eventOutput << ", \"args\": {\"" << eventData.argument << "\": " << std::any_cast<uint64_t>(eventData.value) << "}";
						}

						eventOutput << ", \"pid\": \"" << eventData.pid << "\"";
						eventOutput << ", \"tid\": \"" << eventData.tid << "\"},\n";
						fileOutput << eventOutput.str();
					}
				}, __FILE__, __LINE__);
			}
		}
	}

	void Profiler::addEvent(std::string_view category, std::string_view name, char ph, uint64_t id, std::string_view argument, std::any value)
	{
		if (!currentThreadIdentifier)
		{
			currentThreadIdentifier = GetThreadIdentifier();
		}

		auto lastSlash = category.rfind('\\');
		category = category.substr(lastSlash == std::string_view::npos ? 0 : lastSlash + 1);
		static const auto Zero = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::duration<double>(0.0));
		eventList.push_back(Event { processIdentifier, currentThreadIdentifier, name.data(), category.data(), (std::chrono::high_resolution_clock::now().time_since_epoch() - startTime), Zero, ph, id, argument, value });
		flush();
	}

	void Profiler::addSpan(std::string_view category, std::string_view name, std::chrono::nanoseconds startTime, std::chrono::nanoseconds endTime)
	{
		if (!currentThreadIdentifier)
		{
			currentThreadIdentifier = GetThreadIdentifier();
		}

		auto lastSlash = category.rfind('\\');
		category = category.substr(lastSlash == std::string_view::npos ? 0 : lastSlash + 1);
		eventList.push_back({ processIdentifier, currentThreadIdentifier, name.data(), category.data(), (std::chrono::high_resolution_clock::now().time_since_epoch() - startTime), (endTime - startTime), 'X', 0, String::Empty, 0ULL });
		flush();
	}
#else
#endif
}; // namespace Gek
