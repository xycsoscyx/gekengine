#include "GEK/Utility/Profiler.hpp"
#include "GEK/Utility/String.hpp"
#include <iostream>

namespace Gek
{
#ifdef GEK_ENABLE_PROFILER
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

    Profiler::ScopedEvent::ScopedEvent(Profiler *profiler, uint64_t nameIdentifier)
        : nameIdentifier(nameIdentifier)
        , profiler(profiler)
    {
        profiler->addTimeStamp(TimeStamp(TimeStamp::Type::Begin, nameIdentifier, &std::chrono::high_resolution_clock::now().time_since_epoch()));
    }

    Profiler::ScopedEvent::~ScopedEvent()
    {
        profiler->addTimeStamp(TimeStamp(TimeStamp::Type::End, nameIdentifier, &std::chrono::high_resolution_clock::now().time_since_epoch()));
    }

    void Profiler::writeTimeStamp(TimeStamp *timeStamp)
    {
        auto eventSearch = nameMap.find(timeStamp->nameIdentifier);
        if (eventSearch != std::end(nameMap))
        {
            auto threadSearch = nameMap.find(timeStamp->threadIdentifier);
            if (threadSearch == std::end(nameMap))
            {
                auto threadInsert = nameMap.insert(std::make_pair(timeStamp->threadIdentifier, String::Format("thread_%v", timeStamp->threadIdentifier)));
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

        fileOutput.open(String::Format("profile_%v.json", processIdentifier));
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

    Hash Profiler::registerName(const char* const name)
    {
        auto hash = std::hash<std::string>()(name);
        nameMap.insert(std::make_pair(hash, name));
        return hash;
    }

    void Profiler::registerThreadName(const char* const threadName)
    {
        auto threadIdentifier = getThreadIdentifier();
        nameMap.insert(std::make_pair(threadIdentifier, threadName));
    }

    void Profiler::beginFrame(void)
    {
        addTimeStamp(TimeStamp(TimeStamp::Type::Begin, profilerName, nullptr, &mainThread));
    }

    void Profiler::addTimeStamp(TimeStamp const &timeStamp)
    {
        timeStampList.push_back(timeStamp);
    }

    void Profiler::endFrame(void)
    {
        addTimeStamp(TimeStamp(TimeStamp::Type::End, profilerName, nullptr, &mainThread));
        concurrency::critical_section::scoped_lock lock(criticalSection);
        flushTimeStampList(std::move(timeStampList));
        timeStampList.clear();
    }
#endif
}; // namespace Gek
