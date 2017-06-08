#include "GEK/Utility/JSON.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include "GEK/Utility/Context.hpp"

namespace Gek
{
    namespace JSON
    {
        const Object EmptyObject = Object();

		Object Load(FileSystem::Path const &filePath, const Object &defaultValue)
		{
            std::string error;
			std::string data(FileSystem::Load(filePath, String::Empty));
            auto object = json11::Json::parse(data, error);
            if (error.empty())
            {
                return defaultValue;
            }
            else
            {
                return object;
            }
		}

        void Save(FileSystem::Path const &filePath, Object const &object)
        {
            auto dump = object.dump();
            FileSystem::Save(filePath, dump);
        }

        Object::object &GetObject(Object &object)
        {
            return const_cast<Object::object &>(object.object_items());
        }

        Object::array &GetArray(Object &object)
        {
            return const_cast<Object::array &>(object.array_items());
        }

        std::string From(JSON::Object const &data, std::string const &defaultValue)
        {
            switch (data.type())
            {
            case json11::Json::Type::STRING:
                return data.string_value();

            case json11::Json::Type::BOOL:
                return String::Format("%v", data.bool_value());

            case json11::Json::Type::NUMBER:
                return String::Format("%v", data.number_value());

            default:
                return defaultValue;
            };
        }

        bool From(JSON::Object const &data, bool defaultValue)
        {
            switch (data.type())
            {
            case json11::Json::Type::STRING:
                return String::Convert<bool>(data.string_value(), defaultValue);

            case json11::Json::Type::BOOL:
                return data.bool_value();

            case json11::Json::Type::NUMBER:
                return (data.int_value() != 0);

            default:
                return defaultValue;
            };
        }

        int32_t From(JSON::Object const &data, int32_t defaultValue)
        {
            switch (data.type())
            {
            case json11::Json::Type::STRING:
                return String::Convert<int32_t>(data.string_value(), defaultValue);

            case json11::Json::Type::NUMBER:
                return data.int_value();

            case json11::Json::Type::BOOL:
                return (data.bool_value() ? 1 : 0);

            default:
                return defaultValue;
            };
        }

        float From(JSON::Object const &data, float defaultValue)
        {
            switch (data.type())
            {
            case json11::Json::Type::STRING:
                return String::Convert<float>(data.string_value(), defaultValue);

            case json11::Json::Type::NUMBER:
                return static_cast<float>(data.number_value());

            case json11::Json::Type::BOOL:
                return (data.bool_value() ? 1.0f : 0.0f);

            default:
                return defaultValue;
            };
        }

        Math::Float2 From(JSON::Object const &data, Math::Float2 const &defaultValue)
        {
            if (data.is_array() && data.array_items().size() == 2)
            {
                return Math::Float2(
                    From(data[0], defaultValue.x),
                    From(data[1], defaultValue.y));
            }

            return defaultValue;
        }

        Math::Float3 From(JSON::Object const &data, Math::Float3 const &defaultValue)
        {
            if (data.is_array() && data.array_items().size() == 3)
            {
                return Math::Float3(
                    From(data[0], defaultValue.x),
                    From(data[1], defaultValue.y),
                    From(data[2], defaultValue.y));
            }

            return defaultValue;
        }

        Math::Float4 From(JSON::Object const &data, Math::Float4 const &defaultValue)
        {
            if (data.is_array())
            {
                if (data.array_items().size() == 3)
                {
                    return Math::Float4(
                        From(data[0], defaultValue.x),
                        From(data[1], defaultValue.y),
                        From(data[2], defaultValue.z), 1.0f);
                }
                else if (data.array_items().size() == 4)
                {
                    return Math::Float4(
                        From(data[0], defaultValue.x),
                        From(data[1], defaultValue.y),
                        From(data[2], defaultValue.z),
                        From(data[3], defaultValue.w));
                }
            }

            return defaultValue;
        }

        Math::Quaternion From(JSON::Object const &data, Math::Quaternion const &defaultValue)
        {
            if (data.is_array())
            {
                if (data.array_items().size() == 3)
                {
                    return Math::Quaternion::FromEuler(
                        From(data[0], defaultValue.x),
                        From(data[1], defaultValue.y),
                        From(data[2], defaultValue.z));
                }
                else if (data.array_items().size() == 4)
                {
                    return Math::Quaternion(
                        From(data[0], defaultValue.x),
                        From(data[1], defaultValue.y),
                        From(data[2], defaultValue.z),
                        From(data[3], defaultValue.w));
                }
            }

            return defaultValue;
        }

        JSON::Object To(std::string const &value)
        {
            return value;
        }

        JSON::Object To(bool value)
        {
            return value;
        }

        JSON::Object To(int32_t value)
        {
            return value;
        }

        JSON::Object To(float value)
        {
            return value;
        }

        JSON::Object To(Math::Float2 const &value)
        {
            return json11::Json::array({
                value.x,
                value.y,
            });
        }

        JSON::Object To(Math::Float3 const &value)
        {
            return json11::Json::array({
                value.x,
                value.y,
                value.z,
            });
        }

        JSON::Object To(Math::Float4 const &value)
        {
            return json11::Json::array({
                value.x,
                value.y,
                value.z,
                value.w
            });
        }

        JSON::Object To(Math::Quaternion const &value)
        {
            return json11::Json::array({
                value.x,
                value.y,
                value.z,
                value.w
            });
        }
    }; // namespace JSON
}; // namespace Gek
