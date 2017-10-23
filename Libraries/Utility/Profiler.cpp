#include "GEK/Utility/Profiler.hpp"
#include "GEK/Utility/String.hpp"

namespace Gek
{
#ifdef GEK_ENABLE_PROFILER
    std::size_t getThreadIdentifier(void)
    {
        return std::hash<std::thread::id>()(std::this_thread::get_id());;

        std::stringstream thread;
        thread << std::this_thread::get_id();
        return std::stoull(thread.str());
    }

    Profiler::Data::Data(std::size_t nameHash)
        : nameHash(nameHash)
        , threadIdentifier(getThreadIdentifier())
        , startTime(std::chrono::high_resolution_clock::now().time_since_epoch())
        , endTime(startTime)
    {
    }

    Profiler::Data::Data(std::size_t nameHash, std::size_t threadIdentifier, std::chrono::nanoseconds startTime, std::chrono::nanoseconds endTime)
        : nameHash(nameHash)
        , threadIdentifier(threadIdentifier)
        , startTime(startTime)
        , endTime(endTime)
    {
    }

    Profiler::Event::Event(Profiler *profiler, uint64_t nameHash)
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
                auto eventSearch = nameMap.find(data.nameHash);
                if (eventSearch != std::end(nameMap))
                {
                    auto threadSearch = nameMap.find(data.threadIdentifier);
                    if (threadSearch == std::end(nameMap))
                    {
                        auto threadInsert = nameMap.insert(std::make_pair(data.threadIdentifier, String::Format("thread_%v", data.threadIdentifier)));
                        threadSearch = threadInsert.first;
                    }

                    fileOutput <<
                        ",\n" \
                        "\t\t {" \
                        "\"name\": \"" << eventSearch->second << "\"" \
                        ", \"ph\": \"X\"" \
                        ", \"pid\": \"%" << processIdentifier << "\"" \
                        ", \"tid\": \"" << threadSearch->second << "\"" \
                        ", \"ts\": " << data.startTime.count() <<
                        ", \"dur\": " << (data.endTime - data.startTime).count() <<
                        "}";
                }
            }
        });
    }

    Profiler::Profiler(void)
        : processIdentifier(GetCurrentProcessId())
    {
        registerThreadName("MainThread");

        buffer.reserve(100);
        fileOutput.open(String::Format("profile_%v.json", processIdentifier));
        fileOutput <<
            "{\n" \
            "\t\"traceEvents\": [";
        fileOutput <<
            "\n" \
            "\t\t {" \
            "\"name\": \"" << "Profiler" << "\"" \
            ", \"ph\": \"B\"" \
            ", \"pid\": \"%" << processIdentifier << "\"" \
            ", \"tid\": \"" << "MainThread" << "\"" \
            ", \"ts\": " << std::chrono::high_resolution_clock::now().time_since_epoch().count() <<
            "}";
    }

    Profiler::~Profiler()
    {
        flushQueue();

        writePool.drain();

        fileOutput <<
            ",\n" \
            "\t\t {" \
            "\"name\": \"" << "Profiler" << "\"" \
            ", \"ph\": \"E\"" \
            ", \"pid\": \"%" << processIdentifier << "\"" \
            ", \"tid\": \"" << "MainThread" << "\"" \
            ", \"ts\": " << std::chrono::high_resolution_clock::now().time_since_epoch().count() <<
            "}";
        fileOutput <<
            "\n" \
            "\t],\n" \
            "\t\"displayTimeUnit\": \"ns\",\n" \
            "\t\"systemTraceEvents\": \"SystemTraceData\",\n" \
            "\t\"otherData\": {\n" \
            "\t\t\"version\": \"GEK Profile Data v1.0\"\n" \
            "\t}\n" \
            "}\n";
        fileOutput.close();
    }

    std::size_t Profiler::registerName(const char* const name)
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
#endif
}; // namespace Gek
