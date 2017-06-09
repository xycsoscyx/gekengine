/// @file
/// @author Todd Zupan <toddzupan@gmail.com>
/// @version $Revision$
/// @section LICENSE
/// https://en.wikipedia.org/wiki/MIT_License
/// @section DESCRIPTION
/// Last Changed: $Date$
#pragma once

#include "GEK/Math/Vector2.hpp"
#include "GEK/Math/Vector3.hpp"
#include "GEK/Math/Vector4.hpp"
#include "GEK/Math/Quaternion.hpp"
#include "GEK/Utility/String.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include "GEK/Utility/ShuntingYard.hpp"
#include <jsoncons/json.hpp>

namespace Gek
{
    class JSON
    {
    public:
        static const jsoncons::json EmptyObject;
        static const jsoncons::json EmptyArray;

        using Members = jsoncons::range<jsoncons::json::const_object_iterator>;
        using Elements = jsoncons::range<jsoncons::json::const_array_iterator>;

    private:
        jsoncons::json &object;

    public:
        JSON(jsoncons::json &object);
        JSON &operator = (jsoncons::json &object);

        static jsoncons::json Load(FileSystem::Path const &filePath);
        void Save(FileSystem::Path const &filePath);

        Members getMembers(void) const;
        Elements getElements(void) const;

        jsoncons::json const &get(std::string const &name) const;
        jsoncons::json const &at(size_t index) const;

        std::string convert(std::string const &defaultValue = String::Empty);
        bool convert(bool defaultValue = false);
        int32_t convert(int32_t defaultValue = 0);
        uint32_t convert(uint32_t defaultValue = 0);
        float convert(float defaultValue = 0.0f);
        Math::Float2 convert(Math::Float2 const &defaultValue = Math::Float2::Zero);
        Math::Float3 convert(Math::Float3 const &defaultValue = Math::Float3::Zero);
        Math::Float4 convert(Math::Float4 const &defaultValue = Math::Float4::Zero);
        Math::Quaternion convert(Math::Quaternion const &defaultValue = Math::Quaternion::Identity);

        std::string parse(ShuntingYard &shuntingYard, std::string const &defaultValue = String::Empty);
        bool parse(ShuntingYard &shuntingYard, bool defaultValue = false);
        int32_t parse(ShuntingYard &shuntingYard, int32_t defaultValue = 0);
        uint32_t parse(ShuntingYard &shuntingYard, uint32_t defaultValue = 0);
        float parse(ShuntingYard &shuntingYard, float defaultValue = 0.0f);
        Math::Float2 parse(ShuntingYard &shuntingYard, Math::Float2 const &defaultValue = Math::Float2::Zero);
        Math::Float3 parse(ShuntingYard &shuntingYard, Math::Float3 const &defaultValue = Math::Float3::Zero);
        Math::Float4 parse(ShuntingYard &shuntingYard, Math::Float4 const &defaultValue = Math::Float4::Zero);
        Math::Quaternion parse(ShuntingYard &shuntingYard, Math::Quaternion const &defaultValue = Math::Quaternion::Identity);

        static jsoncons::json Make(std::string const &value);
        static jsoncons::json Make(bool value);
        static jsoncons::json Make(int32_t value);
        static jsoncons::json Make(uint32_t value);
        static jsoncons::json Make(float value);
        static jsoncons::json Make(Math::Float2 const &value);
        static jsoncons::json Make(Math::Float3 const &value);
        static jsoncons::json Make(Math::Float4 const &value);
        static jsoncons::json Make(Math::Quaternion const &value);
    };
}; // namespace Gek
