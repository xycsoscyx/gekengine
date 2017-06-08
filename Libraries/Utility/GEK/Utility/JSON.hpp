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
#include <json11.hpp>

namespace Gek
{
    namespace JSON
    {
        using Object = json11::Json;

        extern const Object EmptyObject;

        Object Load(FileSystem::Path const &filePath, Object const &defaultValue = EmptyObject);
        void Save(FileSystem::Path const &filePath, Object const &object);

        Object::object &GetObject(Object &object);
        Object::array &GetArray(Object &object);

        std::string From(Object const &object, std::string const &defaultValue = String::Empty);
        bool From(Object const &object, bool defaultValue = false);
        int32_t From(Object const &object, int32_t defaultValue = 0);
        float From(Object const &object, float defaultValue = 0.0f);
        Math::Float2 From(Object const &object, Math::Float2 const &defaultValue = Math::Float2::Zero);
        Math::Float3 From(Object const &object, Math::Float3 const &defaultValue = Math::Float3::Zero);
        Math::Float4 From(Object const &object, Math::Float4 const &defaultValue = Math::Float4::Zero);
        Math::Quaternion From(Object const &object, Math::Quaternion const &defaultValue = Math::Quaternion::Identity);

        Object To(std::string const &value);
        Object To(bool value);
        Object To(int32_t value);
        Object To(float value);
        Object To(Math::Float2 const &value);
        Object To(Math::Float3 const &value);
        Object To(Math::Float4 const &value);
        Object To(Math::Quaternion const &value);
    }; // namespace JSON
}; // namespace Gek
