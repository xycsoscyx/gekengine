#include "GEK/Utility/JSON.hpp"

namespace Gek
{
    namespace JSON
    {
        Object Load(FileSystem::Path const& filePath)
        {
            std::ifstream file(filePath.getString());
            if (file.is_open())
            {
                return nlohmann::json::parse(file);
            }
            else
            {
                return Object::object();
            }
        }

        void Save(const Object& object, FileSystem::Path const& filePath)
        {
            FileSystem::Save(filePath, object.dump(4));
        }

        const Object& Find(const Object& object, std::string_view key)
        {
            auto search = object.find(key);
            if (search != std::end(object))
            {
                return search.value();
            }

            static const Object Empty;
            return Empty;
        }

        Object& Find(Object& object, std::string_view key)
        {
            return object[key];
        }

        bool Value(const Object& object, bool defaultValue)
        {
            return object.is_boolean() ? object.get<bool>() : defaultValue;
        }

        float Value(const Object& object, float defaultValue)
        {
            return object.is_number() ? object.get<float>() : defaultValue;
        }

        int32_t Value(const Object& object, int32_t defaultValue)
        {
            return object.is_number() ? object.get<int32_t>() : defaultValue;
        }

        uint32_t Value(const Object& object, uint32_t defaultValue)
        {
            return object.is_number() ? object.get<uint32_t>() : defaultValue;
        }

        std::string Value(const Object& object, std::string_view defaultValue)
        {
            return object.is_string() ? object.get<std::string>() : defaultValue.data();
        }

        bool Value(const Object& object, std::string_view key, bool defaultValue)
        {
            auto search = object.find(key);
            if (search != std::end(object))
            {
                return Value(search.value(), defaultValue);
            }

            return defaultValue;
        }

        float Value(const Object& object, std::string_view key, float defaultValue)
        {
            auto search = object.find(key);
            if (search != std::end(object))
            {
                return Value(search.value(), defaultValue);
            }

            return defaultValue;
        }

        int32_t Value(const Object& object, std::string_view key, int32_t defaultValue)
        {
            auto search = object.find(key);
            if (search != std::end(object))
            {
                return Value(search.value(), defaultValue);
            }

            return defaultValue;
        }

        uint32_t Value(const Object& object, std::string_view key, uint32_t defaultValue)
        {
            auto search = object.find(key);
            if (search != std::end(object))
            {
                return Value(search.value(), defaultValue);
            }

            return defaultValue;
        }

        std::string Value(const Object& object, std::string_view key, std::string_view defaultValue)
        {
            auto search = object.find(key);
            if (search != std::end(object))
            {
                return Value(search.value(), defaultValue);
            }

            return defaultValue.data();
        }

        bool Evaluate(const Object& object, ShuntingYard& shuntingYard, bool defaultValue)
        {
            if (object.is_string())
            {
                return shuntingYard.evaluate(object.get<std::string>()).value_or(defaultValue);
            }

            return object.is_boolean() ? object.get<bool>() : defaultValue;
        }

        float Evaluate(const Object& object, ShuntingYard& shuntingYard, float defaultValue)
        {
            if (object.is_string())
            {
                return shuntingYard.evaluate(object.get<std::string>()).value_or(defaultValue);
            }

            return object.is_number() ? object.get<float>() : defaultValue;
        }

        int32_t Evaluate(const Object& object, ShuntingYard& shuntingYard, int32_t defaultValue)
        {
            if (object.is_string())
            {
                return shuntingYard.evaluate(object.get<std::string>()).value_or(defaultValue);
            }

            return object.is_number() ? object.get<int32_t>() : defaultValue;
        }

        uint32_t Evaluate(const Object& object, ShuntingYard& shuntingYard, uint32_t defaultValue)
        {
            if (object.is_string())
            {
                return shuntingYard.evaluate(object.get<std::string>()).value_or(defaultValue);
            }

            return object.is_number() ? object.get<uint32_t>() : defaultValue;
        }

        Math::Float2 Evaluate(const Object& object, ShuntingYard& shuntingYard, Math::Float2 const& defaultValue)
        {
            if (object.is_array())
            {
                switch (object.size())
                {
                case 1:
                    return Math::Float2(Evaluate(object[0], shuntingYard, 0.0f));

                case 2:
                    return Math::Float2(
                        Evaluate(object[0], shuntingYard, defaultValue.x),
                        Evaluate(object[1], shuntingYard, defaultValue.y));
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
                    return Math::Float3(Evaluate(object[0], shuntingYard, 0.0f));

                case 3:
                    return Math::Float3(
                        Evaluate(object[0], shuntingYard, defaultValue.x),
                        Evaluate(object[1], shuntingYard, defaultValue.y),
                        Evaluate(object[2], shuntingYard, defaultValue.z));
                };
            }

            return defaultValue;
        }

        Math::Float4 Evaluate(const Object& object, ShuntingYard& shuntingYard, Math::Float4 const& defaultValue)
        {
            if (object.is_object())
            {
                const auto& method = object.begin();
                if (String::GetLower(method.key()) == "normalize")
                {
                    return Evaluate(method.value(), shuntingYard, defaultValue).getNormal();
                }
            }
            else if (object.is_array())
            {
                switch (object.size())
                {
                case 1:
                    return Math::Float4(Evaluate(object[0], shuntingYard, 0.0f));

                case 3:
                    return Math::Float4(
                        Evaluate(object[0], shuntingYard, defaultValue.x),
                        Evaluate(object[1], shuntingYard, defaultValue.y),
                        Evaluate(object[2], shuntingYard, defaultValue.z),
                        defaultValue.w);

                case 4:
                    return Math::Float4(
                        Evaluate(object[0], shuntingYard, defaultValue.x),
                        Evaluate(object[1], shuntingYard, defaultValue.y),
                        Evaluate(object[2], shuntingYard, defaultValue.z),
                        Evaluate(object[3], shuntingYard, defaultValue.w));
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
                        Evaluate(object[0], shuntingYard, defaultValue.x),
                        Evaluate(object[1], shuntingYard, defaultValue.y),
                        Evaluate(object[2], shuntingYard, defaultValue.z));

                case 4:
                    return Math::Quaternion(
                        Evaluate(object[0], shuntingYard, defaultValue.x),
                        Evaluate(object[1], shuntingYard, defaultValue.y),
                        Evaluate(object[2], shuntingYard, defaultValue.z),
                        Evaluate(object[3], shuntingYard, defaultValue.w));
                };
            }

            return defaultValue;
        }

        std::string Evaluate(const Object& object, ShuntingYard& shuntingYard, std::string const& defaultValue)
        {
            return Value(object, defaultValue);
        }
    }; // namespace JSON
}; // namespace Gek