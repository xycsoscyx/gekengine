#include "GEK/Utility/Profiler.hpp"
#include "GEK/Utility/String.hpp"
#include <iostream>
#include <fstream>

namespace Gek
{
	Hash GetThreadIdentifier(void)
	{
		return std::hash<std::thread::id>()(std::this_thread::get_id());;

		std::stringstream thread;
		thread << std::this_thread::get_id();
		return std::stoull(thread.str());
	}

	thread_local Hash currentThreadIdentifier = 0;

	struct Profiler::Data
	{
		struct Event
		{
			Hash pid = 0;
			Hash tid = 0;
			std::string_view name = String::Empty;
			std::string_view category = String::Empty;
			std::chrono::nanoseconds ts;
			std::chrono::nanoseconds dur;
			char ph = 0;
			uint64_t id = 0;
			std::string_view argument = String::Empty;
			std::any value = 0ULL;
		};

		Hash processIdentifier = GetCurrentProcessId();
		Hash mainThreadIdentifier = GetThreadIdentifier();

		ThreadPool<1> writePool;
		std::ofstream fileOutput;

		concurrency::concurrent_unordered_map<Hash, std::string> nameMap;
		concurrency::concurrent_vector<Event> eventList;
		concurrency::critical_section criticalSection;

		template <typename ...ARGUMENTS>
		void addEvent(Hash *threadIdentifier, ARGUMENTS&&... arguments)
		{
			[&](void) -> void
			{
				concurrency::critical_section::scoped_lock lock(criticalSection);
				if (!currentThreadIdentifier)
				{
					currentThreadIdentifier = GetThreadIdentifier();
				}

				eventList.push_back({ processIdentifier, (threadIdentifier ? *threadIdentifier : currentThreadIdentifier), std::forward<ARGUMENTS>(arguments)... });
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
								//eventOutput << ", \"args\": {\"" << eventData.argument << "\": " << std::any_cast<uint64_t>(eventData.value) << "}";
							}

							eventOutput << ", \"pid\": \"" << eventData.pid << "\"";
							auto nameSearch = nameMap.find(eventData.tid);
							if (nameSearch == std::end(nameMap))
							{
								eventOutput << ", \"tid\": \"" << eventData.tid << "\"},\n";
							}
							else
							{
								eventOutput << ", \"tid\": \"" << nameSearch->second << "\"},\n";
							}

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
			"\t\"traceEvents\": [\n";
	}

	Profiler::~Profiler(void)
	{
		data->writePool.drain(true);

		data->fileOutput <<
					"\t\t{" <<
						"\"name\": \"" << "Profiler" << "\"" <<
						", \"ph\": \"X\"" <<
						", \"pid\": \"" << data->processIdentifier << "\"" <<
						", \"tid\": \"" << "Main Thread" << "\"" <<
						", \"ts\": " << std::chrono::high_resolution_clock::now().time_since_epoch().count() <<
						", \"dur\": 0" <<
					"}\n" <<
				"\t],\n" <<
				"\t\"displayTimeUnit\": \"ns\",\n" <<
				"\t\"systemTraceEvents\": \"SystemTraceData\",\n" <<
				"\t\"otherData\": {\n" <<
				"\t\t\"version\": \"GEK Profile TimeStamp v1.0\"\n" <<
				"\t}\n" <<
			"}";
		data->fileOutput.close();
	}

	void Profiler::setThreadName(std::string_view name)
	{
		if (!currentThreadIdentifier)
		{
			currentThreadIdentifier = GetThreadIdentifier();
		}

		data->nameMap.insert(std::make_pair(currentThreadIdentifier, name.data()));
	}

	Hash Profiler::registerName(std::string_view name)
	{
		auto hash = GetHash(name, "GEK Engine Profiler Name"sv);
		data->nameMap.insert(std::make_pair(hash, name.data()));
		return hash;
	}

	void Profiler::addEvent(std::string_view category, std::string_view name, char ph, uint64_t id, std::string_view argument, std::any value, Hash *threadIdentifier)
	{
		auto lastSlash = category.rfind('\\');
		category = category.substr(lastSlash == std::string_view::npos ? 0 : lastSlash + 1);
		static const auto Zero = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::duration<double>(0.0));
		data->addEvent(threadIdentifier, name.data(), category.data(), std::chrono::high_resolution_clock::now().time_since_epoch(), Zero, ph, id, argument, value);
	}

	void Profiler::addSpan(std::string_view category, std::string_view name, std::chrono::nanoseconds startTime, std::chrono::nanoseconds endTime, Hash *threadIdentifier)
	{
		auto lastSlash = category.rfind('\\');
		category = category.substr(lastSlash == std::string_view::npos ? 0 : lastSlash + 1);
		data->addEvent(threadIdentifier, name.data(), category.data(),startTime, (endTime - startTime), 'X', 0ULL, String::Empty, 0ULL);
	}
}; // namespace Gek
