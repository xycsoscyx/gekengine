#include "GEK/Utility/Profiler.hpp"
#include "GEK/Utility/String.hpp"
#include <iostream>
#include <fstream>

namespace Gek
{
	const Profiler::Arguments Profiler::EmptyArguments;

	Hash GetThreadIdentifier(void)
	{
		return std::hash<std::thread::id>()(std::this_thread::get_id());;

		std::stringstream thread;
		thread << std::this_thread::get_id();
		return std::stoull(thread.str());
	}

	Hash GetCurrentThreadIdentifier(void)
	{
		thread_local Hash currentThreadIdentifier = 0;
		if (!currentThreadIdentifier)
		{
			currentThreadIdentifier = GetThreadIdentifier();
		}

		return currentThreadIdentifier;
	}

	struct Profiler::Data
	{
		struct Event
		{
			Hash processIdentifier = 0;
			Hash threadIdentifier = 0;
			std::string_view name = String::Empty;
			std::string_view category = String::Empty;
			TimeFormat startTime;
			TimeFormat duration;
			char eventType = 0;
			uint64_t eventIdentifier = 0;
			Arguments arguments;
		};

		Hash processIdentifier = GetCurrentProcessId();
		Hash mainThreadIdentifier = GetThreadIdentifier();

		ThreadPool<1> writePool;
		std::ofstream fileOutput;
		bool exportedFirstEvent = false;

		concurrency::concurrent_vector<Event> eventList;
		concurrency::critical_section criticalSection;

		template <typename ...ARGUMENTS>
		void addEvent(Hash *threadIdentifier, ARGUMENTS&&... arguments)
		{
			[&](void) -> void
			{
				concurrency::critical_section::scoped_lock lock(criticalSection);
				eventList.push_back({ processIdentifier, (threadIdentifier ? *threadIdentifier : GetCurrentThreadIdentifier()), std::forward<ARGUMENTS>(arguments)... });
			}();

			if (eventList.size() > 100 && criticalSection.try_lock())
			{
				concurrency::concurrent_vector<Data::Event> flushList;
				eventList.swap(flushList);
				eventList.clear();
				eventList.reserve(100);
				criticalSection.unlock();

				if (!flushList.empty())
				{
					writePool.enqueueAndDetach([this, flushList = std::move(flushList)](void) -> void
					{
						for (auto &eventData : flushList)
						{
							auto isFirstEvent = exportedFirstEvent;
							exportedFirstEvent = true;

							std::ostringstream eventOutput;
							eventOutput << "\t\t" << (isFirstEvent ? "," : "") << "{\"category\": \"" << eventData.category << "\"";
							eventOutput << ", \"name\": \"" << eventData.name << "\"";
							eventOutput << ", \"ts\": " << eventData.startTime.count();
							eventOutput << ", \"ph\": \"" << eventData.eventType << "\"";
							switch (eventData.eventType)
							{
							case 'S':
							case 'T':
							case 'F':
								eventOutput << ", \"id\": " << eventData.eventIdentifier;
								break;

							case 'X':
								eventOutput << ", \"dur\": " << eventData.duration.count();
								break;
							};

							if (!eventData.arguments.empty())
							{
								eventOutput << ", \"args\": { ";

								bool firstArgument = false;
								for (auto &argument : eventData.arguments)
								{
									eventOutput << (firstArgument ? "" : ", ") << "\"" << argument.first << "\": ";
									firstArgument = true;

									if (argument.second.type() == typeid(std::string)) eventOutput << std::any_cast<std::string>(argument);
									else if (argument.second.type() == typeid(std::string_view)) eventOutput << std::any_cast<std::string_view>(argument);
									else if (argument.second.type() == typeid(int32_t)) eventOutput << std::any_cast<int32_t>(argument);
									else if (argument.second.type() == typeid(uint32_t)) eventOutput << std::any_cast<uint32_t>(argument);
									else if (argument.second.type() == typeid(int64_t)) eventOutput << std::any_cast<int64_t>(argument);
									else if (argument.second.type() == typeid(uint64_t)) eventOutput << std::any_cast<uint64_t>(argument);
									else if (argument.second.type() == typeid(float)) eventOutput << std::any_cast <float>(argument);
								}

								eventOutput << "}";
							}

							eventOutput << ", \"pid\": \"" << eventData.processIdentifier << "\"";
							eventOutput << ", \"tid\": \"" << eventData.threadIdentifier << "\"},\n";
							fileOutput << eventOutput.str();
						}
					}, __FILE__, __LINE__);
				}
			}
		}
	};

	Profiler::Profiler(std::string_view fileName)
		: data(std::make_unique<Data>())
	{
		data->fileOutput.open(fileName.empty() ? String::Format("profile_{}.json", data->processIdentifier).data() : fileName.data());
		data->fileOutput <<
			"{\n" <<
			"\t\"displayTimeUnit\": \"ms\",\n" <<
			"\t\"traceEvenstartTime\": [\n";
	}

	Profiler::~Profiler(void)
	{
		data->writePool.drain(true);

		data->fileOutput <<
			"\t]\n" <<
			"}";
		data->fileOutput.close();
	}

	static const Profiler::Arguments EmptyArgumentMap;
	void Profiler::addEvent(std::string_view category, std::string_view name, char eventType, uint64_t eventIdentifier, Hash *threadIdentifier, Arguments *arguments)
	{
		auto lasstartTimelash = category.rfind('\\');
		category = category.substr(lasstartTimelash == std::string_view::npos ? 0 : lasstartTimelash + 1);
		static const auto Zero = std::chrono::duration_cast<TimeFormat>(std::chrono::duration<double>(0.0));
		data->addEvent(threadIdentifier, name.data(), category.data(), GetProfilerTime(), Zero, eventType, eventIdentifier, (arguments ? (*arguments) : EmptyArgumentMap));
	}

	void Profiler::addSpan(std::string_view category, std::string_view name, TimeFormat startTime, TimeFormat endTime, Hash *threadIdentifier)
	{
		auto lasstartTimelash = category.rfind('\\');
		category = category.substr(lasstartTimelash == std::string_view::npos ? 0 : lasstartTimelash + 1);
		data->addEvent(threadIdentifier, name.data(), category.data(), startTime, (endTime - startTime), 'X', 0ULL, EmptyArgumentMap);
	}
}; // namespace Gek
