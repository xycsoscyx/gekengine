#include "GEK/Utility/JSON.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include "GEK/Utility/Context.hpp"

namespace Gek
{
    namespace JSON
    {
        const Object EmptyObject = Object::make_object({});
        const Object EmptyArray = Object::make_array();

        std::string Parse(ShuntingYard &shuntingYard, Object const &object, std::string const &defaultValue)
        {
            switch (object.type_id())
            {
            case jsoncons::value_type::null_t:
            case jsoncons::value_type::array_t:
            case jsoncons::value_type::object_t:
                return String::Empty;

            default:
                return object.as_string();
            };
        }

        bool Parse(ShuntingYard &shuntingYard, Object const &object, bool defaultValue)
        {
            switch (object.type_id())
            {
            case jsoncons::value_type::small_string_t:
            case jsoncons::value_type::string_t:
                return shuntingYard.evaluate(object.as_string(), defaultValue) != 0.0f;

            case jsoncons::value_type::bool_t:
                return object.var_.bool_data_cast()->value();

            case jsoncons::value_type::double_t:
                return object.var_.double_data_cast()->value() != 0.0;

            case jsoncons::value_type::integer_t:
                return object.var_.integer_data_cast()->value() != 0;

            case jsoncons::value_type::uinteger_t:
                return object.var_.uinteger_data_cast()->value() != 0;

            default:
                return defaultValue;
            };
        }

        int32_t Parse(ShuntingYard &shuntingYard, Object const &object, int32_t defaultValue)
        {
            switch (object.type_id())
            {
            case jsoncons::value_type::small_string_t:
            case jsoncons::value_type::string_t:
                return static_cast<int32_t>(shuntingYard.evaluate(object.as_string(), defaultValue));

            case jsoncons::value_type::double_t:
                return static_cast<int64_t>(object.var_.double_data_cast()->value());

            case jsoncons::value_type::integer_t:
                return static_cast<int64_t>(object.var_.integer_data_cast()->value());

            case jsoncons::value_type::uinteger_t:
                return static_cast<int64_t>(object.var_.uinteger_data_cast()->value());

            case jsoncons::value_type::bool_t:
                return object.var_.bool_data_cast()->value() ? 1 : 0;

            default:
                return defaultValue;
            };
        }

        uint32_t Parse(ShuntingYard &shuntingYard, Object const &object, uint32_t defaultValue)
        {
            switch (object.type_id())
            {
            case jsoncons::value_type::small_string_t:
            case jsoncons::value_type::string_t:
                return static_cast<uint32_t>(shuntingYard.evaluate(object.as_string(), defaultValue));

            case jsoncons::value_type::double_t:
                return static_cast<uint32_t>(object.var_.double_data_cast()->value());

            case jsoncons::value_type::integer_t:
                return static_cast<uint32_t>(object.var_.integer_data_cast()->value());

            case jsoncons::value_type::uinteger_t:
                return static_cast<uint32_t>(object.var_.uinteger_data_cast()->value());

            case jsoncons::value_type::bool_t:
                return object.var_.bool_data_cast()->value() ? 1 : 0;

            default:
                return defaultValue;
            };
        }

        float Parse(ShuntingYard &shuntingYard, Object const &object, float defaultValue)
        {
            switch (object.type_id())
            {
            case jsoncons::value_type::small_string_t:
            case jsoncons::value_type::string_t:
                return shuntingYard.evaluate(object.as_string(), defaultValue);

            case jsoncons::value_type::double_t:
                return static_cast<float>(object.var_.double_data_cast()->value());

            case jsoncons::value_type::integer_t:
                return static_cast<float>(object.var_.integer_data_cast()->value());

            case jsoncons::value_type::uinteger_t:
                return static_cast<float>(object.var_.uinteger_data_cast()->value());

            case jsoncons::value_type::bool_t:
                return object.var_.bool_data_cast()->value() ? 1.0f : 0.0f;

            case jsoncons::value_type::array_t:
                if (object.size() == 1)
                {
                    Parse(shuntingYard, object.at(0), defaultValue);
                }
            };

            return defaultValue;
        }

        Math::Float2 Parse(ShuntingYard &shuntingYard, Object const &object, Math::Float2 const &defaultValue)
        {
            if (object.is_array())
            {
                if (object.size() == 1)
                {
                    return Math::Float2(
                        Parse(shuntingYard, object.at(0), defaultValue.x),
                        Parse(shuntingYard, object.at(0), defaultValue.y));
                }
                else if (object.size() == 2)
                {
                    return Math::Float2(
                        Parse(shuntingYard, object.at(0), defaultValue.x),
                        Parse(shuntingYard, object.at(1), defaultValue.y));
                }
            }

            return Math::Float2(
                Parse(shuntingYard, object, defaultValue.x),
                Parse(shuntingYard, object, defaultValue.y));
        }

        Math::Float3 Parse(ShuntingYard &shuntingYard, Object const &object, Math::Float3 const &defaultValue)
        {
            if (object.is_array())
            {
                if (object.size() == 1)
                {
                    return Math::Float3(
                        Parse(shuntingYard, object.at(0), defaultValue.x),
                        Parse(shuntingYard, object.at(0), defaultValue.y),
                        Parse(shuntingYard, object.at(0), defaultValue.z));
                }
                else if (object.size() == 3)
                {
                    return Math::Float3(
                        Parse(shuntingYard, object.at(0), defaultValue.x),
                        Parse(shuntingYard, object.at(1), defaultValue.y),
                        Parse(shuntingYard, object.at(2), defaultValue.z));
                }
            }

            return Math::Float3(
                Parse(shuntingYard, object, defaultValue.x),
                Parse(shuntingYard, object, defaultValue.y),
                Parse(shuntingYard, object, defaultValue.z));
        }

        Math::Float4 Parse(ShuntingYard &shuntingYard, Object const &object, Math::Float4 const &defaultValue)
        {
            if (object.is_array())
            {
                if (object.size() == 1)
                {
                    return Math::Float4(
                        Parse(shuntingYard, object.at(0), defaultValue.x),
                        Parse(shuntingYard, object.at(0), defaultValue.y),
                        Parse(shuntingYard, object.at(0), defaultValue.z),
                        Parse(shuntingYard, object.at(0), defaultValue.w));

                }
                else if (object.size() == 3)
                {
                    return Math::Float4(
                        Parse(shuntingYard, object.at(0), defaultValue.x),
                        Parse(shuntingYard, object.at(1), defaultValue.y),
                        Parse(shuntingYard, object.at(2), defaultValue.z),
                        defaultValue.w);
                }
                else if (object.size() == 4)
                {
                    return Math::Float4(
                        Parse(shuntingYard, object.at(0), defaultValue.x),
                        Parse(shuntingYard, object.at(1), defaultValue.y),
                        Parse(shuntingYard, object.at(2), defaultValue.z),
                        Parse(shuntingYard, object.at(3), defaultValue.w));
                }
            }

            return Math::Float4(
                Parse(shuntingYard, object, defaultValue.x),
                Parse(shuntingYard, object, defaultValue.y),
                Parse(shuntingYard, object, defaultValue.z),
                Parse(shuntingYard, object, defaultValue.w));
        }

        Math::Quaternion Parse(ShuntingYard &shuntingYard, Object const &object, Math::Quaternion const &defaultValue)
        {
            if (object.is_array())
            {
                if (object.size() == 3)
                {
                    float pitch = Parse(shuntingYard, object.at(0), Math::Infinity);
                    float yaw = Parse(shuntingYard, object.at(1), Math::Infinity);
                    float roll = Parse(shuntingYard, object.at(2), Math::Infinity);
                    if (pitch != Math::Infinity && yaw != Math::Infinity && roll != Math::Infinity)
                    {
                        return Math::Quaternion::FromEuler(pitch, yaw, roll);
                    }
                }
                else if (object.size() == 4)
                {
                    return Math::Quaternion(
                        Parse(shuntingYard, object.at(0), defaultValue.x),
                        Parse(shuntingYard, object.at(1), defaultValue.y),
                        Parse(shuntingYard, object.at(2), defaultValue.z),
                        Parse(shuntingYard, object.at(3), defaultValue.w));
                }
            }

            return defaultValue;
        }

        std::string Convert(Object const &object, std::string const &defaultValue)
        {
            switch (object.type_id())
            {
            case jsoncons::value_type::null_t:
            case jsoncons::value_type::array_t:
            case jsoncons::value_type::object_t:
                return String::Empty;

            default:
                return object.as_string();
            };
        }

        bool Convert(Object const &object, bool defaultValue)
        {
            switch (object.type_id())
            {
            case jsoncons::value_type::small_string_t:
            case jsoncons::value_type::string_t:
                return String::Convert(object.as_string(), defaultValue) != 0.0f;

            case jsoncons::value_type::bool_t:
                return object.var_.bool_data_cast()->value();

            case jsoncons::value_type::double_t:
                return object.var_.double_data_cast()->value() != 0.0;

            case jsoncons::value_type::integer_t:
                return object.var_.integer_data_cast()->value() != 0;

            case jsoncons::value_type::uinteger_t:
                return object.var_.uinteger_data_cast()->value() != 0;

            default:
                return defaultValue;
            };
        }

        int32_t Convert(Object const &object, int32_t defaultValue)
        {
            switch (object.type_id())
            {
            case jsoncons::value_type::small_string_t:
            case jsoncons::value_type::string_t:
                return static_cast<int32_t>(String::Convert(object.as_string(), defaultValue));

            case jsoncons::value_type::double_t:
                return static_cast<int64_t>(object.var_.double_data_cast()->value());

            case jsoncons::value_type::integer_t:
                return static_cast<int64_t>(object.var_.integer_data_cast()->value());

            case jsoncons::value_type::uinteger_t:
                return static_cast<int64_t>(object.var_.uinteger_data_cast()->value());

            case jsoncons::value_type::bool_t:
                return object.var_.bool_data_cast()->value() ? 1 : 0;

            default:
                return defaultValue;
            };
        }

        uint32_t Convert(Object const &object, uint32_t defaultValue)
        {
            switch (object.type_id())
            {
            case jsoncons::value_type::small_string_t:
            case jsoncons::value_type::string_t:
                return static_cast<uint32_t>(String::Convert(object.as_string(), defaultValue));

            case jsoncons::value_type::double_t:
                return static_cast<uint32_t>(object.var_.double_data_cast()->value());

            case jsoncons::value_type::integer_t:
                return static_cast<uint32_t>(object.var_.integer_data_cast()->value());

            case jsoncons::value_type::uinteger_t:
                return static_cast<uint32_t>(object.var_.uinteger_data_cast()->value());

            case jsoncons::value_type::bool_t:
                return object.var_.bool_data_cast()->value() ? 1 : 0;

            default:
                return defaultValue;
            };
        }

        float Convert(Object const &object, float defaultValue)
        {
            switch (object.type_id())
            {
            case jsoncons::value_type::small_string_t:
            case jsoncons::value_type::string_t:
                return String::Convert(object.as_string(), defaultValue);

            case jsoncons::value_type::double_t:
                return static_cast<float>(object.var_.double_data_cast()->value());

            case jsoncons::value_type::integer_t:
                return static_cast<float>(object.var_.integer_data_cast()->value());

            case jsoncons::value_type::uinteger_t:
                return static_cast<float>(object.var_.uinteger_data_cast()->value());

            case jsoncons::value_type::bool_t:
                return object.var_.bool_data_cast()->value() ? 1.0f : 0.0f;

            default:
                return defaultValue;
            };
        }

        Math::Float2 Convert(Object const &object, Math::Float2 const &defaultValue)
        {
            if (object.is_array() && object.size() == 2)
            {
                return Math::Float2(
                    Convert(object.at(0), defaultValue.x),
                    Convert(object.at(1), defaultValue.y));
            }

            return defaultValue;
        }

        Math::Float3 Convert(Object const &object, Math::Float3 const &defaultValue)
        {
            if (object.is_array() && object.size() == 3)
            {
                return Math::Float3(
                    Convert(object.at(0), defaultValue.x),
                    Convert(object.at(1), defaultValue.y),
                    Convert(object.at(2), defaultValue.z));
            }

            return defaultValue;
        }

        Math::Float4 Convert(Object const &object, Math::Float4 const &defaultValue)
        {
            if (object.is_array())
            {
                if (object.size() == 3)
                {
                    return Math::Float4(
                        Convert(object.at(0), defaultValue.x),
                        Convert(object.at(1), defaultValue.y),
                        Convert(object.at(2), defaultValue.z),
                        defaultValue.z);
                }
                else if (object.size() == 4)
                {
                    return Math::Float4(
                        Convert(object.at(0), defaultValue.x),
                        Convert(object.at(1), defaultValue.y),
                        Convert(object.at(2), defaultValue.z),
                        Convert(object.at(3), defaultValue.w));
                }
            }

            return defaultValue;
        }

        Math::Quaternion Convert(Object const &object, Math::Quaternion const &defaultValue)
        {
            if (object.is_array())
            {
                if (object.size() == 3)
                {
                    float pitch = Convert(object.at(0), Math::Infinity);
                    float yaw = Convert(object.at(1), Math::Infinity);
                    float roll = Convert(object.at(2), Math::Infinity);
                    if (pitch != Math::Infinity && yaw != Math::Infinity && roll != Math::Infinity)
                    {
                        return Math::Quaternion::FromEuler(pitch, yaw, roll);
                    }
                }
                else if (object.size() == 4)
                {
                    return Math::Quaternion(
                        Convert(object.at(0), defaultValue.x),
                        Convert(object.at(1), defaultValue.y),
                        Convert(object.at(2), defaultValue.z),
                        Convert(object.at(3), defaultValue.w));
                }
            }

            return defaultValue;
        }

        Object Load(FileSystem::Path const &filePath)
        {
            std::string object(FileSystem::Load(filePath, String::Empty));
            std::istringstream dataStream(object);
            jsoncons::json_decoder<Object> decoder;
            jsoncons::json_reader reader(dataStream, decoder);

            std::error_code errorCode;
            reader.read(errorCode);
            if (errorCode)
            {
                return EmptyObject;
                std::cerr << errorCode.message() << " at line " << reader.line_number() << " and column " << reader.column_number() << std::endl;
            }
            else
            {
                return decoder.get_result();
            }
        }

        Object Make(std::string const &value)
        {
            return value;
        }

        Object Make(bool value)
        {
            return value;
        }

        Object Make(int32_t value)
        {
            return value;
        }

        Object Make(uint32_t value)
        {
            return value;
        }

        Object Make(float value)
        {
            return value;
        }

        Object Make(Math::Float2 const &value)
        {
            return Object::make_array({
                value.x,
                value.y,
            });
        }

        Object Make(Math::Float3 const &value)
        {
            return Object::make_array({
                value.x,
                value.y,
                value.z,
            });
        }

        Object Make(Math::Float4 const &value)
        {
            return Object::make_array({
                value.x,
                value.y,
                value.z,
                value.w,
            });
        }

        Object Make(Math::Quaternion const &value)
        {
            return Object::make_array({
                value.x,
                value.y,
                value.z,
                value.w,
            });
        }
    }; // namespace JSON
}; // namespace Gek
