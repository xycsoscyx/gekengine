#include "GEK/Utility/JSON.hpp"

namespace Gek
{
    namespace JSON
    {
        Object Load(FileSystem::Path const& filePath)
        {
            std::ifstream file(filePath.getString());
            return nlohmann::json::parse(file);
        }

        void Save(const Object& object, FileSystem::Path const& filePath)
        {
            FileSystem::Save(filePath, object.dump(4));
        }

        Math::Float2 Evaluate(const Object& object, ShuntingYard& shuntingYard, Math::Float2 const& defaultValue)
        {
            if (object.is_array())
            {
                switch (object.size())
                {
                case 1:
                    return Math::Float2(shuntingYard.evaluate(object[0].get<std::string>()).value_or(0.0f));

                case 2:
                    return Math::Float2(
                        shuntingYard.evaluate(object[0].get<std::string>()).value_or(defaultValue.x),
                        shuntingYard.evaluate(object[1].get<std::string>()).value_or(defaultValue.y));
                };
            }

            return defaultValue;
        }

        Math::Float3 Evaluate(const Object& object, ShuntingYard& shuntingYard, Math::Float3 const& defaultValue)
        {
            if (object.is_array())
            {
                switch (object.size())
                {
                case 1:
                    return Math::Float3(shuntingYard.evaluate(object[0].get<std::string>()).value_or(0.0f));

                case 3:
                    return Math::Float3(
                        shuntingYard.evaluate(object[0].get<std::string>()).value_or(defaultValue.x),
                        shuntingYard.evaluate(object[1].get<std::string>()).value_or(defaultValue.y),
                        shuntingYard.evaluate(object[2].get<std::string>()).value_or(defaultValue.z));
                };
            }

            return defaultValue;
        }

        Math::Float4 Evaluate(const Object& object, ShuntingYard& shuntingYard, Math::Float4 const& defaultValue)
        {
            if (object.is_array())
            {
                switch (object.size())
                {
                case 1:
                    return Math::Float4(shuntingYard.evaluate(object[0].get<std::string>()).value_or(0.0f));

                case 3:
                    return Math::Float4(
                        shuntingYard.evaluate(object[0].get<std::string>()).value_or(defaultValue.x),
                        shuntingYard.evaluate(object[1].get<std::string>()).value_or(defaultValue.y),
                        shuntingYard.evaluate(object[2].get<std::string>()).value_or(defaultValue.z),
                        defaultValue.w);

                case 4:
                    return Math::Float4(
                        shuntingYard.evaluate(object[0].get<std::string>()).value_or(defaultValue.x),
                        shuntingYard.evaluate(object[1].get<std::string>()).value_or(defaultValue.y),
                        shuntingYard.evaluate(object[2].get<std::string>()).value_or(defaultValue.z),
                        shuntingYard.evaluate(object[3].get<std::string>()).value_or(defaultValue.w));
                };
            }

            return defaultValue;
        }

        Math::Quaternion Evaluate(const Object& object, ShuntingYard& shuntingYard, Math::Quaternion const& defaultValue)
        {
            if (object.is_array())
            {
                switch (object.size())
                {
                case 3:
                    return Math::Quaternion::MakeEulerRotation(
                        shuntingYard.evaluate(object[0].get<std::string>()).value_or(defaultValue.x),
                        shuntingYard.evaluate(object[1].get<std::string>()).value_or(defaultValue.y),
                        shuntingYard.evaluate(object[2].get<std::string>()).value_or(defaultValue.z));

                case 4:
                    return Math::Quaternion(
                        shuntingYard.evaluate(object[0].get<std::string>()).value_or(defaultValue.x),
                        shuntingYard.evaluate(object[1].get<std::string>()).value_or(defaultValue.y),
                        shuntingYard.evaluate(object[2].get<std::string>()).value_or(defaultValue.z),
                        shuntingYard.evaluate(object[3].get<std::string>()).value_or(defaultValue.w));
                };
            }

            return defaultValue;
        }

        std::string Evaluate(const Object& object, ShuntingYard& shuntingYard, std::string const& defaultValue)
        {
            return defaultValue;
        }
    }; // namespace JSON
}; // namespace Gek