/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision: 31a07b88fab4425367fa0aa67fe970fbff7dc9dc $
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date:   Thu Oct 27 08:51:53 2016 -0700 $
#pragma once

#include <string_view>
#include <functional>
#include <algorithm>
#include <sstream>
#include <codecvt>
#include <string>
#include <vector>
#include <format>

using namespace std::string_literals; // enables s-suffix for std::string literals  
using namespace std::string_view_literals; // enables s-suffix for std::string literals  

namespace Gek
{
    namespace String
    {
        extern const std::string Empty;
        extern const std::locale Locale;

        void TrimLeft(std::string& string, std::function<bool(char)> checkElement = [](char ch) { return !std::isspace(ch, Locale); });
        void TrimRight(std::string& string, std::function<bool(char)> checkElement = [](char ch) { return !std::isspace(ch, Locale); });
        void Trim(std::string& string, std::function<bool(char)> checkElement = [](char ch) { return !std::isspace(ch, Locale); });

        std::string GetLower(std::string_view string);
        std::string GetUpper(std::string_view string);

        bool EndsWith(std::string_view value, std::string_view ending);

        template <class ARRAY>
        std::string Join(ARRAY const& strings, std::string_view delimiter)
        {
            std::stringstream combinedString;
            std::copy(std::begin(strings), std::end(strings), std::ostream_iterator<std::string>(combinedString, delimiter.data()));
            return combinedString.str();
        }

        std::vector<std::string> Split(std::string_view string, char delimiter, bool clearSpaces = true);

        bool Replace(std::string &string, std::string_view searchFor, std::string_view replaceWith);

        bool Convert(std::string_view string, bool defaultValue);

        template <typename TYPE>
        TYPE Convert(std::string_view string, TYPE const &defaultValue)
        {
            std::stringstream stream(std::move(std::string(string)));

            TYPE value;
            stream >> value;
            return (stream.fail() ? defaultValue : value);
        }
    }; // namespace String
}; // namespace Gek