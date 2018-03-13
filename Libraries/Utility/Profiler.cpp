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
#ifdef GEK_PROFILER_ENABLED
		Hash GetThreadIdentifier(void)
		{
			return std::hash<std::thread::id>()(std::this_thread::get_id());;

			std::stringstream thread;
			thread << std::this_thread::get_id();
			return std::stoull(thread.str());
		}

		static const Hash processIdentifier = GetCurrentProcessId();
		static const Hash mainThreadIdentifier = GetThreadIdentifier();
		static thread_local Hash currentThreadIdentifier = 0;

		static std::chrono::nanoseconds startTime;
		static std::unique_ptr<ThreadPool<1>> writePool;
		static std::ofstream fileOutput;

		static concurrency::concurrent_unordered_map<Hash, std::string> nameMap;

		struct Event
		{
			Hash pid;
			Hash tid;
			std::string name;
			std::string category;
			std::chrono::nanoseconds ts;
			std::chrono::nanoseconds dur;
			char ph;
			uint64_t id;
			std::string_view argument;
			std::any value;

			void setCategory(std::string_view category)
			{
				auto backSlash = category.rfind('\\');
				this->category = category.substr(backSlash == std::string_view::npos ? 0 : (backSlash + 1));
			}

			Event(std::string_view namne, std::string_view category, std::chrono::nanoseconds ts, char ph, uint64_t id = 0, std::string_view argument = nullptr, std::any value = nullptr)
				: name(name)
				, ts(ts)
				, ph(ph)
				, pid(processIdentifier)
				, id(id)
				, argument(argument)
				, value(value)
			{
				setCategory(category);
				if (!currentThreadIdentifier)
				{
					currentThreadIdentifier = GetThreadIdentifier();
				}

				tid = currentThreadIdentifier;
			}

			Event(std::string_view namne, std::string_view category, std::chrono::nanoseconds ts, std::chrono::nanoseconds dur)
				: name(name)
				, ts(ts)
				, dur(dur)
				, ph('X')
				, pid(processIdentifier)
			{
				setCategory(category);
				if (!currentThreadIdentifier)
				{
					currentThreadIdentifier = GetThreadIdentifier();
				}

				tid = currentThreadIdentifier;
			}
		};

		static concurrency::concurrent_vector<Event> eventList;
		void WriteEvent(Event const &eventData)
		{
			std::ostringstream eventOutput;

			eventOutput << "\t\t{\"category\":\"" << eventData.category << "\"";
			eventOutput << ", \"ts\":" << eventData.ts.count();
			eventOutput << ", \"ph\":\"" << eventData.ph << "\"";
			eventOutput << ", \"name\":\"" << eventData.name << "\"";
			switch (eventData.ph)
			{
			case 'S':
			case 'T':
			case 'F':
				eventOutput << ", \"id\":" << eventData.id;
				break;

			case 'X':
				eventOutput << ", \"dur\":" << eventData.dur.count();
				break;
			};

			if (!eventData.argument.empty() && eventData.value.has_value())
			{
				eventOutput << ", \"args\":{\"" << eventData.argument << "\":" << std::any_cast<uint64_t>(eventData.value) << "}";
			}

			eventOutput << ", \"pid\":\"" << eventData.pid << "\"";
			eventOutput << ", \"tid\":\"" << eventData.tid << "\"},\n";
			fileOutput << eventOutput.str();
		}

		static void FlushEventList(concurrency::concurrent_vector<Event> &&flushList)
		{
			if (writePool)
			{
				writePool->enqueueAndDetach([flushList = std::move(flushList)](void) -> void
				{
					for (auto &eventData : flushList)
					{
						WriteEvent(eventData);
					}
				}, __FILE__, __LINE__);
			}
		}

		void Initialize(std::string_view fileName)
		{
			startTime = std::chrono::high_resolution_clock::now().time_since_epoch();

			writePool = std::make_unique<ThreadPool<1>>();
			fileOutput.open(String::Format("profile_{}.json", processIdentifier));
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

		void Shutdown(void)
		{
			if (writePool)
			{
				FlushEventList(std::move(eventList));
				writePool->drain(true);
				writePool = nullptr;
			}

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

		void AddEvent(Event const &eventData)
		{
			eventList.push_back(eventData);
			if (eventList.size() > 100)
			{
				concurrency::concurrent_vector<Event> flushList;
				eventList.swap(flushList);
				eventList.reserve(100);
				FlushEventList(std::move(flushList));
			}
		}

		void AddEvent(std::string_view category, std::string_view name, char ph, uint64_t id, std::string_view argument, std::any value)
		{
			AddEvent(Event(name, category, (std::chrono::high_resolution_clock::now().time_since_epoch() - startTime), ph, id, argument, value));
		}

		void AddSpan(std::string_view category, std::string_view name, std::chrono::nanoseconds startTime, std::chrono::nanoseconds endTime)
		{
			AddEvent(Event(name, category, (std::chrono::high_resolution_clock::now().time_since_epoch() - startTime), (endTime - startTime)));
		}
#else
		void Initialize(std::string_view fileName)
		{
		}

		void Shutdown(void)
		{
		}

		void AddEvent(std::string_view category, std::string_view name, char ph, uint64_t id)
		{
		}

		void AddEvent(std::string_view category, std::string_view name, char ph, uint64_t id, std::string_view argument, std::any value)
		{
		}

		void AddSpan(std::string_view category, std::string_view name, std::chrono::nanoseconds startTime, std::chrono::nanoseconds endTime)
		{
		}
#endif
	}; // namespace Profiler
}; // namespace Gek
