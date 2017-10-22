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
#include <vector>
#include <chrono>

namespace Gek
{
    class Profiler
    {
    public:
        class Event
        {
        public:
            Event()
            {
            }

            Event(Profiler *profiler, size_t nameHash);
            ~Event();

            Profiler *profiler;
            std::chrono::time_point<std::chrono::steady_clock> startTime;
            std::chrono::time_point<std::chrono::steady_clock> endTime;
            size_t nameHash;
            DWORD threadIdentifier;
        };

    private:
        ThreadPool<1> writePool;
        concurrency::concurrent_unordered_map<size_t, std::string> nameMap;
        std::chrono::high_resolution_clock clock;
        concurrency::critical_section criticalSection;
        DWORD processIdentifier;
        FILE* file;
        std::atomic<bool> firstRecord;
        std::vector<Event> buffer;

    private:
        void flushQueue(void);
        void addEvent(Event const &event);

    public:
        Profiler(void);
        virtual ~Profiler(void);

        size_t registerName(const char* const name);
        void registerThreadName(const char* const name);
    };
}; // namespace Gek