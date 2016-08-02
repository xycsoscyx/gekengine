#pragma once

#include "GEK\Utility\String.h"
#include <future>

#define GEK_START_EXCEPTIONS()                                  class Exception : public std::exception { public: using std::exception::exception; };
#define GEK_ADD_EXCEPTION(TYPE)                                 class TYPE : public Exception { public: using Exception::Exception; };

#define GEK_REQUIRE(CHECK)                                      do { if ((CHECK) == false) { _ASSERTE(CHECK); exit(-1); } } while (false)

namespace Gek
{
    template<typename FUNCTION, typename... PARAMETERS>
    void setPromise(std::promise<void> &promise, FUNCTION function, PARAMETERS&&... arguments)
    {
        function(std::forward<PARAMETERS>(arguments)...);
    }

    template<typename RETURN, typename FUNCTION, typename... PARAMETERS>
    void setPromise(std::promise<RETURN> &promise, FUNCTION functin, PARAMETERS&&... arguments)
    {
        promise.set_value(function(std::forward<PARAMETERS>(arguments)...));
    }

    template<typename FUNCTION, typename... PARAMETERS>
    std::future<typename std::result_of<FUNCTION(PARAMETERS...)>::type> asynchronous(FUNCTION function, PARAMETERS&&... arguments)
    {
        using ReturnValue = std::result_of<FUNCTION(PARAMETERS...)>::type;

        std::promise<ReturnValue> promise;
        std::future<ReturnValue> future = promise.get_future();
        std::thread thread([](std::promise<ReturnValue>& promise, FUNCTION function, PARAMETERS&&... arguments)
        {
            try
            {
                setPromise(promise, function, std::forward<PARAMETERS>(arguments)...);
            }
            catch (const std::exception &)
            {
                promise.set_exception(std::current_exception());
            }
        }, std::move(promise), function, std::forward<PARAMETERS>(arguments)...);
        thread.detach();
        return std::move(future);
    }
}; // namespace Gek
