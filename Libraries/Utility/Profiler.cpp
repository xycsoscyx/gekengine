#include "GEK/Utility/Profiler.hpp"
#include "GEK/Utility/String.hpp"
#include "GEK/Utility/ThreadPool.hpp"
#include <concurrent_unordered_map.h>
#include <concurrent_vector.h>
#include <iostream>
#include <fstream>

namespace Gek
{
	namespace Profiler
	{
		Hash getThreadIdentifier(void)
		{
			return std::hash<std::thread::id>()(std::this_thread::get_id());;

			std::stringstream thread;
			thread << std::this_thread::get_id();
			return std::stoull(thread.str());
		}

		uint64_t processIdentifier = GetCurrentProcessId();
		Hash mainThread = getThreadIdentifier();

		std::chrono::high_resolution_clock clock;
		concurrency::critical_section criticalSection;

		std::unique_ptr<ThreadPool<1>> writePool;
		std::ofstream fileOutput;

		std::chrono::nanoseconds frameTimeStamp;
		concurrency::concurrent_unordered_map<Hash, std::string> nameMap;
		concurrency::concurrent_vector<TimeStamp> timeStampList;

		static void WriteTimeStamp(TimeStamp *timeStamp)
		{
			auto eventSearch = nameMap.find(timeStamp->nameIdentifier);
			if (eventSearch != std::end(nameMap))
			{
				auto threadSearch = nameMap.find(timeStamp->threadIdentifier);
				if (threadSearch == std::end(nameMap))
				{
					auto threadInsert = nameMap.insert(std::make_pair(timeStamp->threadIdentifier, String::Format("thread_{}", timeStamp->threadIdentifier)));
					threadSearch = threadInsert.first;
				}

				switch (timeStamp->type)
				{
				case TimeStamp::Type::Begin:
					fileOutput <<
						",\n" \
						"\t\t {" \
						"\"name\": \"" << eventSearch->second << "\"" \
						", \"ph\": \"B\"" \
						", \"pid\": \"" << processIdentifier << "\"" \
						", \"tid\": \"" << threadSearch->second << "\"" \
						", \"ts\": " << timeStamp->timeStamp.count() <<
						"}";
					break;

				case TimeStamp::Type::End:
					fileOutput <<
						",\n" \
						"\t\t {" \
						"\"name\": \"" << eventSearch->second << "\"" \
						", \"ph\": \"E\"" \
						", \"pid\": \"" << processIdentifier << "\"" \
						", \"tid\": \"" << threadSearch->second << "\"" \
						", \"ts\": " << timeStamp->timeStamp.count() <<
						"}";
					break;

				case TimeStamp::Type::Duration:
					fileOutput <<
						",\n" \
						"\t\t {" \
						"\"name\": \"" << eventSearch->second << "\"" \
						", \"ph\": \"X\"" \
						", \"pid\": \"" << processIdentifier << "\"" \
						", \"tid\": \"" << threadSearch->second << "\"" \
						", \"ts\": " << timeStamp->timeStamp.count() <<
						", \"dur\": " << timeStamp->duration.count() <<
						"}";
					break;
				};
			}
		}

		static void FlushTimeStampList(concurrency::concurrent_vector<TimeStamp> &&flushList)
		{
			if (writePool)
			{
				writePool->enqueue([flushList = move(flushList)](void) mutable -> void
				{
					for (auto &timeStamp : flushList)
					{
						WriteTimeStamp(&timeStamp);
					}
				});
			}
		}

		void Initialize(void)
		{
			writePool = std::make_unique<ThreadPool<1>>();
			fileOutput.open(String::Format("profile_{}.json", processIdentifier));
			fileOutput <<
				"{\n" \
				"\t\"traceEvents\": [";
			fileOutput <<
				"\n" \
				"\t\t {" \
				"\"name\": \"" << "Profiler" << "\"" \
				", \"ph\": \"B\"" \
				", \"pid\": \"" << processIdentifier << "\"" \
				", \"tid\": \"" << "MainThread" << "\"" \
				", \"ts\": " << std::chrono::high_resolution_clock::now().time_since_epoch().count() <<
				"}";
			}

		void Shutdown(char const *outputFileName)
		{
			if (writePool)
			{
				writePool->drain();
				writePool = nullptr;

				for (auto &timeStamp : timeStampList)
				{
					WriteTimeStamp(&timeStamp);
				}
			}

			fileOutput <<
				",\n" \
				"\t\t {" \
				"\"name\": \"" << "Profiler" << "\"" \
				", \"ph\": \"E\"" \
				", \"pid\": \"" << processIdentifier << "\"" \
				", \"tid\": \"" << "MainThread" << "\"" \
				", \"ts\": " << std::chrono::high_resolution_clock::now().time_since_epoch().count() <<
				"}";
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

		Hash RegisterName(std::string_view name)
		{
			auto hash = std::hash<std::string_view>()(name);
			nameMap.insert(std::make_pair(hash, name));
			return hash;
		}

		TimeStamp::TimeStamp(Type type, Hash nameIdentifier, std::chrono::nanoseconds const *timeStamp, Hash const *threadIdentifier)
			: type(type)
			, nameIdentifier(nameIdentifier)
			, threadIdentifier(threadIdentifier ? *threadIdentifier : getThreadIdentifier())
			, timeStamp(timeStamp ? *timeStamp : std::chrono::high_resolution_clock::now().time_since_epoch())
		{
			timeStampList.push_back(*this);
			if (timeStampList.size() > 100)
			{
				FlushTimeStampList(std::move(timeStampList));
				timeStampList.clear();
			}
		}

		TimeStamp::TimeStamp(Hash nameIdentifier, std::chrono::nanoseconds startTime, std::chrono::nanoseconds endTime, Hash const *threadIdentifier)
			: type(Type::Duration)
			, nameIdentifier(nameIdentifier)
			, threadIdentifier(threadIdentifier ? *threadIdentifier : getThreadIdentifier())
			, timeStamp(startTime)
			, duration(endTime - startTime)
		{
		}

		Scope::Scope(Hash nameIdentifier)
			: nameIdentifier(nameIdentifier)
			, timeStamp(std::chrono::high_resolution_clock::now().time_since_epoch())
		{
		}

		Scope::~Scope()
		{
			TimeStamp(nameIdentifier, timeStamp, std::chrono::high_resolution_clock::now().time_since_epoch());
		}
	}; // namespace Profiler
}; // namespace Gek
