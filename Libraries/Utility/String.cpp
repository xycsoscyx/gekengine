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

        std::string Join(std::vector<std::string> const &list, char delimiter, bool initialDelimiter)
        {
            if (list.empty())
            {
                return Empty;
            }

            auto size = (initialDelimiter ? 1 : 0); // initial length
            size += (list.size() - 1); // insert delimiters between list elements
            for (const auto &string : list)
            {
                size += string.length();
            }

            std::string result;
            result.reserve(size);
            if (initialDelimiter)
            {
                result.append(1U, delimiter);
            }

            result.append(list.front());
            for (auto &string = std::next(std::begin(list), 1); string != std::end(list); ++string)
            {
                result.append(1U, delimiter);
                result.append(*string);
            }

            return result;
        }

        std::vector<std::string> Split(const std::string &string, char delimiter, bool clearSpaces)
        {
            std::string current;
            std::vector<std::string> tokens;
            std::stringstream stream(string);
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

        bool Replace(std::string &string, std::string_view const &searchFor, std::string_view const &replaceWith)
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

        std::wstring Widen(std::string_view const &string)
        {
            return std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t>{}.from_bytes(string.data());
        }

        std::string Narrow(std::wstring_view const &string)
        {
            return std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t>{}.to_bytes(string.data());
        }

        std::string Convert(bool const &value)
        {
            std::stringstream stream;
            stream << std::boolalpha << value;
            return stream.str();
        }

        std::string Convert(float const &value)
        {
            std::stringstream stream;
            stream << std::showpoint << value;
            return stream.str();
        }

        bool Convert(std::string const &string, bool defaultValue)
        {
            bool value;
            std::stringstream stream(string);
            stream >> std::boolalpha >> value;
            return (stream.fail() ? defaultValue : value);
        }

        float Convert(std::string const &string, float defaultValue)
        {
            float value;
            std::stringstream stream(string);
            stream >> value;
            return (stream.fail() ? defaultValue : value);
        }
    }; // namespace String
}; // namespace Gek