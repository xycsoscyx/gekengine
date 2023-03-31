/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision: 31a07b88fab4425367fa0aa67fe970fbff7dc9dc $
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date:   Thu Oct 27 08:51:53 2016 -0700 $
#pragma once

#include "GEK/Utility/String.hpp"

namespace Gek
{
    std::mutex LockedWrite::mutex;

    namespace String
    {
        const std::string Empty;
        const std::locale Locale = std::locale::classic();

        void TrimLeft(std::string &string, std::function<bool(char)> checkElement)
        {
            string.erase(std::begin(string), std::find_if(std::begin(string), std::end(string), checkElement));
        }

        void TrimRight(std::string &string, std::function<bool(char)> checkElement)
        {
            string.erase(std::find_if(std::rbegin(string), std::rend(string), checkElement).base(), std::end(string));
        }

        void Trim(std::string &string, std::function<bool(char)> checkElement)
        {
            TrimLeft(string, checkElement);
            TrimRight(string, checkElement);
        }

        std::string GetLower(std::string_view string)
        {
            std::string transformed;
            std::transform(std::begin(string), std::end(string), std::back_inserter(transformed), ::tolower);
            return transformed;
        }

        std::string GetUpper(std::string_view string)
        {
            std::string transformed;
            std::transform(std::begin(string), std::end(string), std::back_inserter(transformed), ::toupper);
            return transformed;
        }

        bool EndsWith(std::string_view value, std::string_view ending)
        {
            return ((ending.size() > value.size()) ? false : std::equal(std::rbegin(ending), std::rend(ending), std::rbegin(value)));
        }

        std::vector<std::string> Split(std::string_view string, char delimiter, bool clearSpaces)
        {
            std::string current;
            std::vector<std::string> tokens;
            std::stringstream stream(std::move(std::string(string)));
            while (std::getline(stream, current, delimiter))
            {
                if (clearSpaces)
                {
                    Trim(current);
                }

                tokens.push_back(current);
            };

            return tokens;
        }

        bool Replace(std::string &string, std::string_view searchFor, std::string_view replaceWith)
        {
            bool did_replace = false;

            size_t position = 0;
            while ((position = string.find(searchFor, position)) != std::string::npos)
            {
                string.replace(position, searchFor.size(), replaceWith);
                position += replaceWith.length();
                did_replace = true;
            };

            return did_replace;
        }

        std::wstring Widen(std::string_view string)
        {
            return std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t>{}.from_bytes(string.data());
        }

        std::string Narrow(std::wstring_view string)
        {
            return std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t>{}.to_bytes(string.data());
        }

        bool Convert(std::string_view string, bool defaultValue)
        {
            std::stringstream stream(std::move(std::string(string)));

            bool value;
            stream >> std::boolalpha >> value;
            return (stream.fail() ? defaultValue : value);
        }
    }; // namespace String
}; // namespace Gek