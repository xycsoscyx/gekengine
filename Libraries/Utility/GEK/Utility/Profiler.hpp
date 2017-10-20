/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision: 1143 $
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date: 2016-10-13 13:29:45 -0700 (Thu, 13 Oct 2016) $
#pragma once

#include <chrono>
#include <concurrent_unordered_map.h>
#include <vector>

namespace Gek
{
#define GEK_ENABLE_PROFILER
#ifdef GEK_ENABLE_PROFILER
    class Profiler final
    {
    public:
        static Profiler* GetInstance();

        void Update();
        void StartSection(int numberOfFrames);
        size_t RegisterName(const char* const name);
        void RegisterThreadName(const char* const name);

        class TrackEntry
        {
        public:
            TrackEntry()
            {
            }

            TrackEntry(size_t nameCRC);
            ~TrackEntry();

            std::chrono::time_point<std::chrono::steady_clock> startTime;
            std::chrono::time_point<std::chrono::steady_clock> endTime;
            size_t nameHash;
            DWORD threadIdentifier;
        };

    private:
        Profiler();
        ~Profiler();

        void EndSection();
        void FlushQueue();
        void QueueEvents();

        concurrency::concurrent_unordered_map<size_t, std::string> eventMap;
        std::chrono::high_resolution_clock clock;
        concurrency::critical_section criticalSection;
        DWORD processIdentifier;
        FILE* file;
        int numberOfFrames;
        int firstRecord;
        long bufferIndex;
        long bufferSize;
        std::vector<TrackEntry> buffer;

        int outQueueBufferindex;
        int outQueueBufferSize;
        std::vector<TrackEntry> outQueueBuffer;
    };

    #define GEK_PROFILE_START_SECTION(frames)                                                   \
            Profiler::GetInstance()->StartSection (frames);

    #define GEK_PROFILE_UPDATE()                                                                \
            Profiler::GetInstance()->Update ();

    #define GEK_PROFILE_THREAD(name)                                                            \
            Profiler::GetInstance()->RegisterThreadName (name);

    #define GEK_PROFILE_EVENT(name)                                                             \
            static auto __crcEventName__ = Profiler::GetInstance()->RegisterName (name);    \
            Profiler::TrackEntry ___trackerEntry___(__crcEventName__);    
#else 
    #define GEK_PROFILE_START_SECTION(frames)
    #define GEK_PROFILE_UPDATE()
    #define GEK_PROFILE_THREAD(name)
    #define GEK_PROFILE_EVENT(name)
#endif
}; // namespace Gek