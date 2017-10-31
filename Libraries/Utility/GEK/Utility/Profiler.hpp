/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision: 1143 $
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date: 2016-10-13 13:29:45 -0700 (Thu, 13 Oct 2016) $
#pragma once

#include "GEK/Utility/Hash.hpp"
#include "GEK/Utility/ThreadPool.hpp"
#include <concurrent_unordered_map.h>
#include <concurrent_vector.h>
#include <fstream>
#include <chrono>

#define GEK_ENABLE_PROFILER

namespace Gek
{
#ifdef GEK_ENABLE_PROFILER
    #define GEK_PROFILE_REGISTER_NAME(PROFILER, NAME) PROFILER->registerName(NAME)
    #define GEK_PROFILE_REGISTER_THREAD(PROFILER, NAME) PROFILER->registerThreadName(NAME)
    #define GEK_PROFILE_BEGIN_FRAME(PROFILER) PROFILER->beginFrame()
    #define GEK_PROFILE_END_FRAME(PROFILER) PROFILER->endFrame()
    #define GEK_PROFILE_EVENT(PROFILER, NAME, THREAD, START, END) PROFILER->addEvent(NAME, START, END, &THREAD)

    #define GEK_PROFILE_AUTO_SCOPE(PROFILER, NAME) \
        static const auto __hash##NAME__ = PROFILER->registerName(NAME); \
        Gek::Profiler::Event __event##NAME__(PROFILER, __hash##NAME__);

    #define GEK_PROFILE_FUNCTION(PROFILER) GEK_PROFILE_AUTO_SCOPE(PROFILER, __FUNCTION__)

    #define GEK_PROFILE_BEGIN_SCOPE(PROFILER, NAME) \
        [&](void) -> void \
        { \
            GEK_PROFILE_AUTO_SCOPE(PROFILER, NAME)

    #define GEK_PROFILE_END_SCOPE() \
        }()

    class Profiler
    {
    public:
        struct Data
        {
            Hash nameIdentifier = 0;
            Hash threadIdentifier;
            std::chrono::nanoseconds startTime;
            std::chrono::nanoseconds endTime;

            Data(void) = default;
            Data(Hash nameIdentifier, std::chrono::nanoseconds startTime, Hash *threadIdentifier = nullptr, std::chrono::nanoseconds *endTime = nullptr);
        };

        struct Event
            : public Data
        {
            Event(void) = default;
            Event(Profiler *profiler, uint64_t nameIdentifier);
            ~Event(void);

            Profiler *profiler = nullptr;
        };

        struct Frame
            : public Data
        {
            Frame *parent = nullptr;
            concurrency::concurrent_vector<Frame> children;

            Frame(void) = default;
            Frame(Frame *parent, Hash nameIdentifier, std::chrono::nanoseconds startTime, Hash *threadIdentifier = nullptr, std::chrono::nanoseconds *endTime = nullptr);
            ~Frame(void) = default;
        };

    private:
        uint64_t processIdentifier = 0;
        std::chrono::high_resolution_clock clock;
        concurrency::critical_section criticalSection;
        Hash profilerName;
        Hash mainThread;

        ThreadPool<1> writePool;
        std::ofstream fileOutput;

        concurrency::concurrent_unordered_map<Hash, std::string> nameMap;

        std::shared_ptr<Frame> frame;
        Frame *currentFrame = nullptr;
        std::list<std::shared_ptr<Frame>> history;

    private:
        void writeFrame(Frame *frame);
        void flushFrame(std::shared_ptr<Frame> &&frame);

    public:
        Profiler(void);
        virtual ~Profiler(void);

        Hash registerName(const char* const name);
        void registerThreadName(const char* const name);

        void beginFrame(void);
        void beginEvent(Hash nameIdentifier, std::chrono::nanoseconds *timeStamp = nullptr, Hash *threadIdentifier = nullptr);
        void endEvent(Hash nameIdentifier);
        void addEvent(Hash nameIdentifier, std::chrono::nanoseconds startTime, std::chrono::nanoseconds endTime, Hash *threadIdentifier = nullptr);
        void endFrame(void);
    };
#else
    #define GEK_PROFILE_REGISTER_NAME(PROFILER, NAME) 0
    #define GEK_PROFILE_REGISTER_THREAD(PROFILER, NAME)
    #define GEK_PROFILE_BEGIN_FRAME(PROFILER)
    #define GEK_PROFILE_END_FRAME(PROFILER)
    #define GEK_PROFILE_EVENT(PROFILER, NAME, THREAD, START, END)
    #define GEK_PROFILE_AUTO_SCOPE(PROFILER, NAME)
    #define GEK_PROFILE_FUNCTION(PROFILER)
    #define GEK_PROFILE_BEGIN_SCOPE(PROFILER, NAME) \
        [&](void) -> void \
        { \

    #define GEK_PROFILE_END_SCOPE() \
        }()

    struct Profiler
    {
        virtual ~Profiler(void) = default;
    };
#endif
}; // namespace Gek