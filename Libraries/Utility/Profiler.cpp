#include "GEK/Utility/Profiler.hpp"
#include "GEK/Utility/String.hpp"
#include <iostream>

namespace Gek
{
    namespace System
    {
        Hash getThreadIdentifier(void)
        {
            return std::hash<std::thread::id>()(std::this_thread::get_id());;

            std::stringstream thread;
            thread << std::this_thread::get_id();
            return std::stoull(thread.str());
        }

        Profiler::TimeStamp::TimeStamp(Type type, Hash nameIdentifier, std::chrono::nanoseconds const *timeStamp, Hash const *threadIdentifier)
            : type(type)
            , nameIdentifier(nameIdentifier)
            , threadIdentifier(threadIdentifier ? *threadIdentifier : getThreadIdentifier())
            , timeStamp(timeStamp ? *timeStamp : std::chrono::high_resolution_clock::now().time_since_epoch())
        {
        }

        Profiler::TimeStamp::TimeStamp(Hash nameIdentifier, std::chrono::nanoseconds startTime, std::chrono::nanoseconds endTime, Hash const *threadIdentifier)
            : type(Type::Duration)
            , nameIdentifier(nameIdentifier)
            , threadIdentifier(threadIdentifier ? *threadIdentifier : getThreadIdentifier())
            , timeStamp(startTime)
            , duration(endTime - startTime)
        {
        }

        Profiler::ScopedEvent::ScopedEvent(Profiler *profiler, uint64_t nameIdentifier)
            : nameIdentifier(nameIdentifier)
            , profiler(profiler)
            , timeStamp(std::chrono::high_resolution_clock::now().time_since_epoch())
        {
        }

        Profiler::ScopedEvent::~ScopedEvent()
        {
            profiler->addTimeStamp(TimeStamp(nameIdentifier, timeStamp, std::chrono::high_resolution_clock::now().time_since_epoch()));
        }

        void Profiler::writeTimeStamp(TimeStamp *timeStamp)
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

        void Profiler::flushTimeStampList(concurrency::concurrent_vector<TimeStamp> &&flushList)
        {
            writePool.enqueue([this, flushList = move(flushList)](void) mutable -> void
            {
                for (auto &timeStamp : flushList)
                {
                    writeTimeStamp(&timeStamp);
                }
            });
        }

        Profiler::Profiler(void)
            : processIdentifier(GetCurrentProcessId())
            , mainThread(getThreadIdentifier())
        {
            registerThreadName("CPU Main Thread");
            profilerName = registerName("Profiler Frame");

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

        Profiler::~Profiler()
        {
            writePool.drain();
            for (auto &timeStamp : timeStampList)
            {
                writeTimeStamp(&timeStamp);
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

        Hash Profiler::registerName(std::string_view name)
        {
            auto hash = std::hash<std::string_view>()(name);
            nameMap.insert(std::make_pair(hash, name));
            return hash;
        }

        void Profiler::registerThreadName(std::string_view threadName)
        {
            auto threadIdentifier = getThreadIdentifier();
            nameMap.insert(std::make_pair(threadIdentifier, threadName));
        }

        void Profiler::beginFrame(void)
        {
            frameTimeStamp = std::chrono::high_resolution_clock::now().time_since_epoch();
        }

        void Profiler::addTimeStamp(TimeStamp const &timeStamp)
        {
            timeStampList.push_back(timeStamp);
        }

        void Profiler::endFrame(void)
        {
            addTimeStamp(TimeStamp(profilerName, frameTimeStamp, std::chrono::high_resolution_clock::now().time_since_epoch(), &mainThread));
            concurrency::critical_section::scoped_lock lock(criticalSection);
            flushTimeStampList(std::move(timeStampList));
            timeStampList.clear();
        }
    }; // namespace System
}; // namespace Gek
