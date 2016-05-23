#pragma once

#include "GEK\Utility\String.h"

namespace Gek
{
    namespace Exception
    {
        class Base
        {
        private:
            LPCSTR _file;
            UINT32 _line;
            CStringW _message;

        public:
            template<typename CHAR, typename... ARGUMENTS>
            Base(LPCSTR file, UINT32 line, const CHAR *format, ARGUMENTS... arguments)
                : _file(file)
                , _line(line)
                , _message(String::format(format, arguments...))
            {
            }

            const LPCSTR getFile(void) const
            {
                return _file;
            }

            const UINT32 getLine(void) const
            {
                return _line;
            }

            const LPCWSTR getMessage(void) const
            {
                return _message.GetString();
            }

            __declspec(property(get = getFile)) const LPCSTR file;
            __declspec(property(get = getLine)) const UINT32 line;
            __declspec(property(get = getMessage)) const LPCWSTR message;
        };

        class FileNotFound : public Base
        {
        };
    }; // namespace Exception
}; // namespace Gek

#define GEKEXCEPTION(TYPE, FORMAT, ...)  throw TYPE(__FILE__, __LINE__, FORMAT, __VA_ARGS__)
