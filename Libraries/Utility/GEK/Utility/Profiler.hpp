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
        static Profiler* GetInstance(void);

        void Start(void);
        void End(void);
        void Update(void);
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

    #define GEK_PROFILE_START_SECTION(frames)                                                       \
            Gek::Profiler::GetInstance()->StartSection (frames);

    #define GEK_PROFILE_UPDATE()                                                                    \
            Gek::Profiler::GetInstance()->Update ();

    #define GEK_PROFILE_THREAD(name)                                                                \
            Gek::Profiler::GetInstance()->RegisterThreadName (name);

    #define GEK_PROFILE_EVENT(name)                                                                 \
            static auto __hash##name__ = Gek::Profiler::GetInstance()->RegisterName (name);   \
            Gek::Profiler::TrackEntry __token##name__(__hash##name__);    

    #define GEK_PROFILE_FUNCTION()                                                                  \
            GEK_PROFILE_EVENT(__FUNCTION__)
#else 
    #define GEK_PROFILE_START_SECTION(frames)
    #define GEK_PROFILE_UPDATE()
    #define GEK_PROFILE_THREAD(name)
    #define GEK_PROFILE_EVENT(name)
#endif
}; // namespace Gek