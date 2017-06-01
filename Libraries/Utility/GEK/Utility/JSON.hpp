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
    namespace JSON
    {
        using Object = jsoncons::json;
        using Member = Object::member_type;
        using Array = Object::array;

		extern const Object EmptyObject;

        Object Load(FileSystem::Path const &filePath, Object const &defaultValue = EmptyObject);
        void Save(FileSystem::Path const &filePath, Object const &object);

        JSON::Object const &Get(Object const &object, std::string const &name, Object const &defaultValue = EmptyObject);
        JSON::Object const &At(Object const &object, size_t index, Object const &defaultValue = EmptyObject);

        float From(JSON::Object const &object, ShuntingYard &parser, float defaultValue = 0.0f);
        Math::Float2 From(JSON::Object const &object, ShuntingYard &parser, Math::Float2 const &defaultValue = Math::Float2::Zero);
        Math::Float3 From(JSON::Object const &object, ShuntingYard &parser, Math::Float3 const &defaultValue = Math::Float3::Zero);
        Math::Float4 From(JSON::Object const &object, ShuntingYard &parser, Math::Float4 const &defaultValue = Math::Float4::Zero);
        Math::Quaternion From(JSON::Object const &object, ShuntingYard &parser, Math::Quaternion const &defaultValue = Math::Quaternion::Identity);
        int32_t From(JSON::Object const &object, ShuntingYard &parser, int32_t defaultValue = 0);
        uint32_t From(JSON::Object const &object, ShuntingYard &parser, uint32_t defaultValue = 0);
        bool From(JSON::Object const &object, ShuntingYard &parser, bool defaultValue = false);
        std::string From(JSON::Object const &object, ShuntingYard &parser, std::string const &defaultValue = String::Empty);

        JSON::Object To(float value);
        JSON::Object To(Math::Float2 const &value);
        JSON::Object To(Math::Float3 const &value);
        JSON::Object To(Math::Float4 const &value);
        JSON::Object To(Math::Quaternion const &value);
        JSON::Object To(int32_t value);
        JSON::Object To(uint32_t value);
        JSON::Object To(bool value);
        JSON::Object To(std::string const &value);
    }; // namespace JSON
}; // namespace Gek
