/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision: 1143 $
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date: 2016-10-13 13:29:45 -0700 (Thu, 13 Oct 2016) $
#pragma once

#include "GEK/Utility/ThreadPool.hpp"
#include <concurrent_unordered_map.h>
#include <concurrent_vector.h>
#include <fstream>
#include <chrono>

#define GEK_PROFILE_SCOPE(PROFILER, NAME) \
    static const auto __hash##NAME__ = PROFILER->registerName(NAME); \
    Gek::Profiler::Event __event##NAME__(PROFILER, __hash##NAME__);

#define GEK_PROFILE_FUNCTION(PROFILER) GEK_PROFILE_SCOPE(PROFILER, __FUNCTION__)

namespace Gek
{
    class Profiler
    {
    public:
        struct Data
        {
            std::size_t nameHash = 0;
            std::size_t threadIdentifier;
            std::chrono::nanoseconds startTime;
            std::chrono::nanoseconds endTime;

            Data(void) = default;
            Data(std::size_t nameHash);
            Data(std::size_t nameHash, std::size_t threadIdentifier, std::chrono::nanoseconds startTime, std::chrono::nanoseconds endTime);
        };

        struct Event
            : public Data
        {
            Event(void) = default;
            Event(Profiler *profiler, uint64_t nameHash);
            ~Event(void);

            Profiler *profiler = nullptr;
        };

    private:
        uint64_t processIdentifier = 0;
        std::chrono::high_resolution_clock clock;
        concurrency::critical_section criticalSection;

        ThreadPool<1> writePool;
        std::ofstream fileOutput;

        concurrency::concurrent_unordered_map<std::size_t, std::string> nameMap;
        concurrency::concurrent_vector<Data> buffer;

    private:
        void flushQueue(void);

    public:
        Profiler(void);
        virtual ~Profiler(void);

        std::size_t registerName(const char* const name);
        void registerThreadName(const char* const name);
        void addEvent(Data const &data);
    };
}; // namespace Gek
