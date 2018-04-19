#include "GEK/Utility/Profiler.hpp"
#include "GEK/Utility/String.hpp"
#include <iostream>
#include <fstream>

namespace Gek
{
	const Profiler::Arguments Profiler::EmptyArguments;
	const Profiler::TimeFormat Profiler::EmptyTime;

	template <typename ARGUMENT0, typename ... ARGUMENTS>
	std::ostream & operator << (std::ostream & stream, std::variant<ARGUMENT0, ARGUMENTS...> const & variant)
	{
		std::visit([&](auto && argument)
		{
			stream << argument;
		}, variant);
		return stream;
	}

	Hash GetThreadIdentifier(void)
	{
		return std::hash<std::thread::id>()(std::this_thread::get_id());;

		std::stringstream thread;
		thread << std::this_thread::get_id();
		return std::stoull(thread.str());
	}

	struct Profiler::Data
	{
		struct Event
		{
			Hash processIdentifier = 0;
			Hash threadIdentifier = 0;
			std::string_view category = String::Empty;
			std::string_view name = String::Empty;
			TimeFormat startTime;
			TimeFormat duration;
			char eventType = 0;
			Hash eventIdentifier = 0;
			Arguments arguments;
		};

		Hash mainProcessIdentifier = GetCurrentProcessId();
		Hash mainThreadIdentifier = GetThreadIdentifier();

		ThreadPool<1> writePool;
		std::ofstream fileOutput;
		bool exportedFirstEvent = false;

		concurrency::concurrent_vector<Event> eventList;
		concurrency::critical_section criticalSection;

		template <typename ...ARGUMENTS>
		void addEvent(ARGUMENTS&&... arguments)
		{
			[&](void) -> void
			{
				concurrency::critical_section::scoped_lock lock(criticalSection);
				eventList.push_back({ std::forward<ARGUMENTS>(arguments)... });
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
							auto lastBackSlash = eventData.category.rfind('\\');
							auto category = eventData.category.substr(lastBackSlash == std::string_view::npos ? 0 : lastBackSlash + 1);

							std::ostringstream eventOutput;
							eventOutput << "\t\t" << (exportedFirstEvent ? "," : "") << "{ " <<
								"\"cat\": \"" << category << "\"" <<
								", \"name\": \"" << eventData.name << "\"" <<
								", \"ts\": " << eventData.startTime.count();
							if (eventData.eventType)
							{
								eventOutput << ", \"ph\": \"" << eventData.eventType << "\"";
							}

							if (eventData.eventIdentifier)
							{
								eventOutput << ", \"id\": " << eventData.eventIdentifier;
							}

							if (eventData.eventType == 'X')
							{
								eventOutput << ", \"dur\": " << eventData.duration.count();
							}

							if (!eventData.arguments.empty())
							{
								eventOutput << ", \"args\": {";

								bool exportedFirstArgument = false;
								for (auto &argument : eventData.arguments)
								{
									eventOutput << (exportedFirstArgument ? ", " : " ") << "\"" << argument.first << "\": \"" << argument.second << "\"";
									exportedFirstArgument = true;

								}

								eventOutput << " }";
							}

							eventOutput <<
								", \"pid\": \"" << (eventData.processIdentifier ? eventData.processIdentifier : mainProcessIdentifier) << "\"" <<
								", \"tid\": \"" << eventData.threadIdentifier <<
								"\" }\n";
							fileOutput << eventOutput.str();
							exportedFirstEvent = true;
						}
					}, __FILE__, __LINE__);
				}
			}
		}
	};

	Profiler::Profiler(std::string_view fileName)
		: data(std::make_unique<Data>())
	{
		data->fileOutput.open(fileName.empty() ? String::Format("profile_{}.json", data->mainProcessIdentifier).data() : fileName.data());
		data->fileOutput <<
			"{\n" <<
			"\t\"displayTimeUnit\": \"ms\",\n" <<
			"\t\"traceEvents\": [\n";
	}

	Profiler::~Profiler(void)
	{
		data->writePool.drain(true);

		data->fileOutput <<
			"\t]\n" <<
			"}";
		data->fileOutput.close();
	}

	Hash Profiler::getCurrentThreadIdentifier(void)
	{
		thread_local Hash currentThreadIdentifier = 0;
		if (!currentThreadIdentifier)
		{
			currentThreadIdentifier = GetThreadIdentifier();
		}

		return currentThreadIdentifier;
	}

	void Profiler::addEvent(Hash processIdentifier, Hash threadIdentifier, std::string_view category, std::string_view name, TimeFormat startTime, TimeFormat duration, char eventType, Hash eventIdentifier, Arguments const &arguments)
	{
		static const auto Zero = std::chrono::duration_cast<TimeFormat>(std::chrono::duration<double>(0.0));
		data->addEvent(processIdentifier, threadIdentifier, category.data(), name.data(), startTime, duration, eventType, eventIdentifier, arguments);
	}
}; // namespace Gek