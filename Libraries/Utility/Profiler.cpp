#include "GEK/Utility/Profiler.hpp"
#include "GEK/Utility/String.hpp"

#ifdef _WIN32
#include <Windows.h>
#endif

#include <time.h>

namespace Gek
{
#ifdef GEK_ENABLE_PROFILER
    Profiler* Profiler::GetInstance()
    {
        static Profiler instance;
        return &instance;
    }

    Profiler::Profiler()
    {
        file = nullptr;
        firstRecord = 0;
        numberOfFrames = 0;
        processIdentifier = GetCurrentProcessId();

        bufferIndex = 0;
        bufferSize = 1024;
        buffer.reserve(bufferSize);

        outQueueBufferindex = 0;
        outQueueBufferSize = 1024 * 512;
        outQueueBuffer.reserve(outQueueBufferSize);
    }

    Profiler::~Profiler()
    {
    }

    void Profiler::StartSection(int numberOfFrames)
    {
        if (file)
        {
            fclose(file);
        }

        time_t rawtime;
        time(&rawtime);
        struct tm * timeinfo = localtime(&rawtime);

        char fileName[80];
        strftime(fileName, sizeof(fileName), "profile_%H%M%S%m%d%Y.json", timeinfo);

        file = fopen(fileName, "wb");
        fprintf(file, "{\n");
        fprintf(file, "\t\"traceEvents\": [");
        this->numberOfFrames = numberOfFrames;
    }

    void Profiler::EndSection()
    {
        concurrency::critical_section::scoped_lock lock(criticalSection);
        FlushQueue();

        fprintf(file, "\n");
        fprintf(file, "\t],\n");

        fprintf(file, "\t\"displayTimeUnit\": \"ns\",\n");
        fprintf(file, "\t\"systemTraceEvents\": \"SystemTraceData\",\n");
        fprintf(file, "\t\"otherData\": {\n");
        fprintf(file, "\t\t\"version\": \"My Application v1.0\"\n");
        fprintf(file, "\t}\n");
        fprintf(file, "}\n");

        fclose(file);
        file = nullptr;
        firstRecord = 0;
    }

    void Profiler::Update()
    {
        if (file)
        {
            QueueEvents();
            if (--numberOfFrames == 0)
            {
                EndSection();
            }
        }
    }
    
    size_t Profiler::RegisterName(const char* const name)
    {
        size_t hash = std::hash<std::string>()(name);
        eventMap.insert(std::make_pair(hash, name));
        return hash;
    }

    void Profiler::RegisterThreadName(const char* const threadName)
    {
        size_t hash = std::hash<std::string>()(threadName);
        eventMap.insert(std::make_pair(hash, threadName));
    }

    void Profiler::QueueEvents()
    {
        concurrency::critical_section::scoped_lock lock(criticalSection);
        if ((outQueueBufferindex + bufferIndex) > outQueueBufferSize)
        {
            FlushQueue();
            outQueueBufferindex = 0;
        }

        memcpy(&outQueueBuffer[outQueueBufferindex], buffer.data(), bufferIndex * sizeof(TrackEntry));
        outQueueBufferindex += bufferIndex;
        bufferIndex = 0;
    }


    void Profiler::FlushQueue()
    {
        for (int index = 0; index < outQueueBufferindex; index++)
        {
            const TrackEntry& event = outQueueBuffer[index];
            if (!firstRecord)
            {
                fprintf(file, "\n");
                firstRecord = true;
            }
            else
            {
                fprintf(file, ",\n");
            }

            auto threadNodeSearch = eventMap.find(event.threadIdentifier);
            if (threadNodeSearch == std::end(eventMap))
            {
                auto threadInsert = eventMap.insert(std::make_pair(event.threadIdentifier, String::Format("thread_%v", event.threadIdentifier)));
                threadNodeSearch = threadInsert.first;
            }

            fprintf(file, "\t\t {");
            fprintf(file, "\"name\": \"%s\"", eventMap.find(event.nameHash)->second.c_str());
            fprintf(file, ", \"cat\": \"%s\"", threadNodeSearch->second.c_str());
            fprintf(file, ", \"ph\": \"X\"");
            fprintf(file, ", \"pid\": \"%d\"", processIdentifier);
            fprintf(file, ", \"tid\": \"%d\"", event.threadIdentifier);
            fprintf(file, ", \"ts\": %f", std::chrono::duration<double>(event.startTime.time_since_epoch()).count());
            fprintf(file, ", \"dur\": %f", std::chrono::duration<double>(event.endTime - event.startTime).count());
            fprintf(file, "}");
        }

        outQueueBufferindex = 0;
    }



    Profiler::TrackEntry::TrackEntry(size_t nameCRC)
    {
        Profiler* const instance = Profiler::GetInstance();
        if (instance->file)
        {
            nameHash = nameCRC;
            startTime = endTime = std::chrono::high_resolution_clock::now();
            threadIdentifier = GetCurrentThreadId();
        }
    }

    Profiler::TrackEntry::~TrackEntry()
    {
        Profiler* const instance = Profiler::GetInstance();
        endTime = std::chrono::high_resolution_clock::now();
        if (instance->file)
        {
            concurrency::critical_section::scoped_lock lock(instance->criticalSection);
            if (instance->bufferIndex >= instance->bufferSize)
            {
                std::vector<TrackEntry> buffer(2 * instance->bufferSize);
                memcpy(buffer.data(), instance->buffer.data(), instance->bufferIndex * sizeof(TrackEntry));
                instance->bufferSize = instance->bufferSize * 2;
                instance->buffer = buffer;
            }

            instance->buffer[instance->bufferIndex] = *this;
            instance->bufferIndex++;
        }
    }
#endif
}; // namespace Gek
