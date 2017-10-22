#include "GEK/Utility/Profiler.hpp"
#include "GEK/Utility/String.hpp"

#ifdef _WIN32
#include <Windows.h>
#endif

#include <time.h>

namespace Gek
{
    void Profiler::flushQueue()
    {
        writePool.enqueue([this, buffer = move(buffer)](void) -> void
        {
            for (auto &event : buffer)
            {
                if (!firstRecord)
                {
                    fprintf(file, "\n");
                    firstRecord = true;
                }
                else
                {
                    fprintf(file, ",\n");
                }

                auto threadNodeSearch = nameMap.find(event.threadIdentifier);
                if (threadNodeSearch == std::end(nameMap))
                {
                    auto threadInsert = nameMap.insert(std::make_pair(event.threadIdentifier, String::Format("thread_%v", event.threadIdentifier)));
                    threadNodeSearch = threadInsert.first;
                }

                fprintf(file, "\t\t {");
                fprintf(file, "\"name\": \"%s\"", nameMap.find(event.nameHash)->second.c_str());
                fprintf(file, ", \"cat\": \"%s\"", threadNodeSearch->second.c_str());
                fprintf(file, ", \"ph\": \"X\"");
                fprintf(file, ", \"pid\": \"%d\"", processIdentifier);
                fprintf(file, ", \"tid\": \"%d\"", event.threadIdentifier);
                fprintf(file, ", \"ts\": %lld", (event.startTime.time_since_epoch()).count());
                fprintf(file, ", \"dur\": %lld", (event.endTime - event.startTime).count());
                fprintf(file, "}");
            }
        });

        buffer.reserve(100);
    }

    Profiler::Profiler(void)
        : firstRecord(false)
        , processIdentifier(GetCurrentProcessId())
    {
        buffer.reserve(100);
        file = fopen(String::Format("profile_%v.json", processIdentifier).c_str(), "wb");
        fprintf(file, "{\n");
        fprintf(file, "\t\"traceEvents\": [");
    }

    Profiler::~Profiler()
    {
        concurrency::critical_section::scoped_lock lock(criticalSection);
        flushQueue();

        fprintf(file, "\n");
        fprintf(file, "\t],\n");

        fprintf(file, "\t\"displayTimeUnit\": \"ns\",\n");
        fprintf(file, "\t\"systemTraceEvents\": \"SystemTraceData\",\n");
        fprintf(file, "\t\"otherData\": {\n");
        fprintf(file, "\t\t\"version\": \"GEK Profile Data v1.0\"\n");
        fprintf(file, "\t}\n");
        fprintf(file, "}\n");

        fclose(file);
    }

    size_t Profiler::registerName(const char* const name)
    {
        size_t hash = std::hash<std::string>()(name);
        nameMap.insert(std::make_pair(hash, name));
        return hash;
    }

    void Profiler::registerThreadName(const char* const threadName)
    {
        size_t hash = std::hash<std::string>()(threadName);
        nameMap.insert(std::make_pair(hash, threadName));
    }

    Profiler::Event::Event(Profiler *profiler, size_t nameHash)
        : profiler(profiler)
        , nameHash(nameHash)
        , startTime(std::chrono::high_resolution_clock::now())
        , endTime(startTime)
        , threadIdentifier(GetCurrentThreadId())
    {
    }

    void Profiler::addEvent(Event const &event)
    {
        concurrency::critical_section::scoped_lock lock(criticalSection);
        buffer.push_back(event);
        if (buffer.size() > 100)
        {
            flushQueue();
        }
    }

    Profiler::Event::~Event()
    {
        endTime = std::chrono::high_resolution_clock::now();
        profiler->addEvent(*this);
    }
}; // namespace Gek
