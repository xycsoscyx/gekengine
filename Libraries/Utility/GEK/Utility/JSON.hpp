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
#include "nlohmann/json.hpp"

namespace Gek
{
    namespace JSON
    {
        using Object = nlohmann::json;

        Object Load(FileSystem::Path const& filePath);
        void Save(const Object &object, FileSystem::Path const& filePath);

        const Object &Find(const Object& object, std::string_view key);
        Object &Find(Object& object, std::string_view key);

        bool Value(const Object& object, bool defaultValue);
        float Value(const Object& object, float defaultValue);
        int32_t Value(const Object& object, int32_t defaultValue);
        uint32_t Value(const Object& object, uint32_t defaultValue);
        std::string Value(const Object& object, std::string_view defaultValue);

        bool Value(const Object& object, std::string_view key, bool defaultValue);
        float Value(const Object& object, std::string_view key, float defaultValue);
        int32_t Value(const Object& object, std::string_view key, int32_t defaultValue);
        uint32_t Value(const Object& object, std::string_view key, uint32_t defaultValue);
        std::string Value(const Object& object, std::string_view key, std::string_view defaultValue);

        template <typename TYPE>
        TYPE Evaluate(const Object& object, ShuntingYard& shuntingYard, TYPE defaultValue)
        {
            return shuntingYard.evaluate(Value(object, String::Empty)).value_or(defaultValue);
        }

        Math::Float2 Evaluate(const Object& object, ShuntingYard& shuntingYard, Math::Float2 const& defaultValue);
        Math::Float3 Evaluate(const Object& object, ShuntingYard& shuntingYard, Math::Float3 const& defaultValue);
        Math::Float4 Evaluate(const Object& object, ShuntingYard& shuntingYard, Math::Float4 const& defaultValue);
        Math::Quaternion Evaluate(const Object& object, ShuntingYard& shuntingYard, Math::Quaternion const& defaultValue);
        std::string Evaluate(const Object& object, ShuntingYard& shuntingYard, std::string const& defaultValue);

        template <typename TYPE>
        TYPE Evaluate(const Object& object, std::string_view key, ShuntingYard& shuntingYard, TYPE defaultValue)
        {
            return Evaluate(Find(object, key), shuntingYard, defaultValue);
        }
    }; // namespace JSON
}; // namespace Gek