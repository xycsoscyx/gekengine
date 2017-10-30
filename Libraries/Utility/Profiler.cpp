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

    Profiler::Data::Data(Hash nameIdentifier)
        : nameIdentifier(nameIdentifier)
        , threadIdentifier(getThreadIdentifier())
        , startTime(std::chrono::high_resolution_clock::now().time_since_epoch())
        , endTime(startTime)
    {
    }

    Profiler::Data::Data(Hash nameIdentifier, Hash threadIdentifier, std::chrono::nanoseconds startTime, std::chrono::nanoseconds endTime)
        : nameIdentifier(nameIdentifier)
        , threadIdentifier(threadIdentifier)
        , startTime(startTime)
        , endTime(endTime)
    {
    }

    Profiler::Event::Event(Profiler *profiler, uint64_t nameIdentifier)
        : Data(nameIdentifier)
        , profiler(profiler)
    {
        profiler->beginEvent(nameIdentifier);
    }

    Profiler::Event::~Event()
    {
        profiler->endEvent();
        //endTime = std::chrono::high_resolution_clock::now().time_since_epoch();
        //profiler->addEvent(*this);
    }

    void Profiler::flushQueue()
    {
        writePool.enqueue([this, buffer = move(buffer)](void) -> void
        {
            for (auto &data : buffer)
            {
                auto eventSearch = nameMap.find(data.nameIdentifier);
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
        frame.clear();
    }

    void Profiler::beginEvent(Hash nameIdentifier, std::chrono::nanoseconds *timeStamp, Hash *threadIdentifier)
    {
        auto startTime = (timeStamp ? *timeStamp : std::chrono::high_resolution_clock::now().time_since_epoch());
        Data data(nameIdentifier, (threadIdentifier ? *threadIdentifier : getThreadIdentifier()), startTime, startTime);
        auto insertIterator = frame.push_back(data);
        frameStack.push_back(&(*insertIterator));
        buffer.push_back(data);

        concurrency::critical_section::scoped_lock lock(criticalSection);
        if (buffer.size() > 100)
        {
            flushQueue();
            buffer.clear();
            buffer.reserve(100);
        }
    }

    void Profiler::endEvent(Hash nameIdentifier)
    {
        if (!frameStack.empty())
        {
            auto &data = *frameStack.back();
            frameStack.pop_back();
            if (data.nameIdentifier != nameIdentifier)
            {
                LockedWrite{ std::cerr } << "End event identifier doesn't match data identifier: " << nameIdentifier << " != " << data.nameIdentifier;
            }

            data.endTime = std::chrono::high_resolution_clock::now().time_since_epoch();
        }
        else
        {
            LockedWrite{ std::cerr } << "End event encountered without a starting frame event: " << nameIdentifier;
        }
    }

    void Profiler::addEvent(Hash nameIdentifier, std::chrono::nanoseconds startTime, std::chrono::nanoseconds endTime, Hash *threadIdentifier)
    {
        Data data(nameIdentifier, (threadIdentifier ? *threadIdentifier : getThreadIdentifier()), startTime, endTime);
        frame.push_back(data);
        buffer.push_back(data);

        concurrency::critical_section::scoped_lock lock(criticalSection);
        if (buffer.size() > 100)
        {
            flushQueue();
            buffer.clear();
            buffer.reserve(100);
        }
    }

    void Profiler::endFrame(void)
    {
        history.emplace_back(std::begin(frame), std::end(frame));
        if (history.size() > 100)
        {
            history.pop_front();
        }
    }
#endif
}; // namespace Gek
