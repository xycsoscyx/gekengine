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

    Profiler::Data::Data(Hash nameIdentifier, std::chrono::nanoseconds startTime, Hash *threadIdentifier, std::chrono::nanoseconds *endTime)
        : nameIdentifier(nameIdentifier)
        , threadIdentifier(threadIdentifier ? *threadIdentifier : getThreadIdentifier())
        , startTime(startTime)
        , endTime(endTime ? *endTime : startTime)
    {
    }

    Profiler::Event::Event(Profiler *profiler, uint64_t nameIdentifier)
        : Data(nameIdentifier, std::chrono::high_resolution_clock::now().time_since_epoch())
        , profiler(profiler)
    {
        profiler->beginEvent(nameIdentifier);
    }

    Profiler::Event::~Event()
    {
        profiler->endEvent(nameIdentifier);
        //endTime = std::chrono::high_resolution_clock::now().time_since_epoch();
        //profiler->addEvent(*this);
    }

    Profiler::Frame::Frame(Frame *parent, Hash nameIdentifier, std::chrono::nanoseconds startTime, Hash *threadIdentifier, std::chrono::nanoseconds *endTime)
        : Data(nameIdentifier, startTime, threadIdentifier, endTime)
        , parent(parent)
    {
    }

    void Profiler::writeFrame(Frame *frame)
    {
        auto eventSearch = nameMap.find(frame->nameIdentifier);
        if (eventSearch != std::end(nameMap))
        {
            auto threadSearch = nameMap.find(frame->threadIdentifier);
            if (threadSearch == std::end(nameMap))
            {
                auto threadInsert = nameMap.insert(std::make_pair(frame->threadIdentifier, String::Format("thread_%v", frame->threadIdentifier)));
                threadSearch = threadInsert.first;
            }

            fileOutput <<
                ",\n" \
                "\t\t {" \
                "\"name\": \"" << eventSearch->second << "\"" \
                ", \"ph\": \"X\"" \
                ", \"pid\": \"" << processIdentifier << "\"" \
                ", \"tid\": \"" << threadSearch->second << "\"" \
                ", \"ts\": " << frame->startTime.count() <<
                ", \"dur\": " << (frame->endTime - frame->startTime).count() <<
                "}";

            for (auto &child : frame->children)
            {
                writeFrame(&child);
            }
        }
    }

    void Profiler::flushFrame(std::shared_ptr<Frame> &&frame)
    {
        writePool.enqueue([this, frame = move(frame)](void) mutable -> void
        {
            writeFrame(frame.get());
        });
    }

    Profiler::Profiler(void)
        : processIdentifier(GetCurrentProcessId())
    {
        registerThreadName("MainThread");
        mainThread = getThreadIdentifier();
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
        for (auto &frame : history)
        {
            writeFrame(frame.get());
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
        frame = std::make_shared<Frame>(nullptr, profilerName, std::chrono::high_resolution_clock::now().time_since_epoch(), &mainThread);
        currentFrame = frame.get();
    }

    void Profiler::beginEvent(Hash nameIdentifier, std::chrono::nanoseconds *timeStamp, Hash *threadIdentifier)
    {
        auto insertIterator = currentFrame->children.push_back(Frame(currentFrame, nameIdentifier, (timeStamp ? *timeStamp : std::chrono::high_resolution_clock::now().time_since_epoch())));
        currentFrame = &(*insertIterator);
    }

    void Profiler::endEvent(Hash nameIdentifier)
    {
        if (currentFrame)
        {
            if (currentFrame->nameIdentifier != nameIdentifier)
            {
                LockedWrite{ std::cerr } << "End event identifier doesn't match data identifier: " << nameIdentifier << " != " << currentFrame->nameIdentifier;
            }

            currentFrame->endTime = std::chrono::high_resolution_clock::now().time_since_epoch();
            currentFrame = currentFrame->parent;
        }
        else
        {
            LockedWrite{ std::cerr } << "End event encountered without a starting frame event: " << nameIdentifier;
        }
    }

    void Profiler::addEvent(Hash nameIdentifier, std::chrono::nanoseconds startTime, std::chrono::nanoseconds endTime, Hash *threadIdentifier)
    {
        currentFrame->children.push_back(Frame(currentFrame, nameIdentifier, startTime, threadIdentifier, &endTime));
    }

    void Profiler::endFrame(void)
    {
        history.emplace_back(std::move(frame));
        currentFrame = nullptr;

        if (history.size() > 100)
        {
            flushFrame(std::move(history.front()));
            history.pop_front();
        }
    }
#endif
}; // namespace Gek
