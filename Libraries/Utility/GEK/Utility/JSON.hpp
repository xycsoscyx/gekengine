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

        template <typename TYPE>
        TYPE Evaluate(const Object& object, ShuntingYard& shuntingYard, TYPE defaultValue)
        {
            return shuntingYard.evaluate(object[0].get<std::string>()).value_or(defaultValue);
        }

        Math::Float2 Evaluate(const Object& object, ShuntingYard& shuntingYard, Math::Float2 const& defaultValue);
        Math::Float3 Evaluate(const Object& object, ShuntingYard& shuntingYard, Math::Float3 const& defaultValue);
        Math::Float4 Evaluate(const Object& object, ShuntingYard& shuntingYard, Math::Float4 const& defaultValue);
        Math::Quaternion Evaluate(const Object& object, ShuntingYard& shuntingYard, Math::Quaternion const& defaultValue);
        std::string Evaluate(const Object& object, ShuntingYard& shuntingYard, std::string const& defaultValue);
    }; // namespace JSON
}; // namespace Gek