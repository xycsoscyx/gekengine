#include "GEK/Utility/Profiler.hpp"
#include "GEK/Utility/String.hpp"

#ifdef _WIN32
#include <Windows.h>
#endif

#include <time.h>

namespace Gek
{
    Profiler::Data::Data(size_t nameHash)
        : nameHash(nameHash)
        , threadIdentifier(std::hash<std::thread::id>()(std::this_thread::get_id()))
        , startTime(std::chrono::high_resolution_clock::now().time_since_epoch())
        , endTime(startTime)
    {
    }

    Profiler::Data::Data(size_t nameHash, size_t threadIdentifier, std::chrono::nanoseconds startTime, std::chrono::nanoseconds endTime)
        : nameHash(nameHash)
        , threadIdentifier(threadIdentifier)
        , startTime(startTime)
        , endTime(endTime)
    {
    }

    Profiler::Event::Event(Profiler *profiler, size_t nameHash)
        : Data(nameHash)
        , profiler(profiler)
    {
    }

    Profiler::Event::~Event()
    {
        endTime = std::chrono::high_resolution_clock::now().time_since_epoch();
        profiler->addEvent(*this);
    }

    void Profiler::flushQueue()
    {
        writePool.enqueue([this, buffer = move(buffer)](void) -> void
        {
            for (auto &data : buffer)
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

                auto threadSearch = nameMap.find(data.threadIdentifier);
                if (threadSearch == std::end(nameMap))
                {
                    auto threadInsert = nameMap.insert(std::make_pair(data.threadIdentifier, String::Format("thread_%v", data.threadIdentifier)));
                    threadSearch = threadInsert.first;
                }
                
                auto eventSearch = nameMap.find(data.nameHash);
                if (eventSearch == std::end(nameMap))
                {
                    throw std::exception("unknown event name");
                }

                fprintf(file, "\t\t {");
                fprintf(file, "\"name\": \"%s\"", eventSearch->second.c_str());
                fprintf(file, ", \"cat\": \"%s\"", threadSearch->second.c_str());
                fprintf(file, ", \"ph\": \"X\"");
                fprintf(file, ", \"pid\": \"%zd\"", processIdentifier);
                fprintf(file, ", \"tid\": \"%zd\"", data.threadIdentifier);
                fprintf(file, ", \"ts\": %lld", data.startTime.count());
                fprintf(file, ", \"dur\": %lld", (data.endTime - data.startTime).count());
                fprintf(file, "}");
            }
        });
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
        flushQueue();

        writePool.drain();

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
        auto threadIdentifier = std::hash<std::thread::id>()(std::this_thread::get_id());
        nameMap.insert(std::make_pair(threadIdentifier, threadName));
    }

    void Profiler::addEvent(Data const &data)
    {
        buffer.push_back(data);
        concurrency::critical_section::scoped_lock lock(criticalSection);
        if (buffer.size() > 100)
        {
            flushQueue();
            buffer.clear();
            buffer.reserve(100);
        }
    }
}; // namespace Gek
