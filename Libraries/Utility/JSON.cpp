#include "GEK/Utility/JSON.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include "GEK/Utility/Context.hpp"

namespace Gek
{
    const jsoncons::json JSON::EmptyObject = jsoncons::json::make_object({});
    const jsoncons::json JSON::EmptyArray = jsoncons::json::make_array();

    static std::string Parse(ShuntingYard &shuntingYard, jsoncons::json const &object, std::string const &defaultValue)
    {
        return object.as_string();
    }

    static bool Parse(ShuntingYard &shuntingYard, jsoncons::json const &object, bool defaultValue)
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

    static int32_t Parse(ShuntingYard &shuntingYard, jsoncons::json const &object, int32_t defaultValue)
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

    static uint32_t Parse(ShuntingYard &shuntingYard, jsoncons::json const &object, uint32_t defaultValue)
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

    static float Parse(ShuntingYard &shuntingYard, jsoncons::json const &object, float defaultValue)
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

        default:
            return defaultValue;
        };
    }

    static Math::Float2 Parse(ShuntingYard &shuntingYard, jsoncons::json const &object, Math::Float2 const &defaultValue)
    {
        if (object.is_array() && object.size() == 2)
        {
            return Math::Float2(
                Parse(shuntingYard, object.at(0), defaultValue.x),
                Parse(shuntingYard, object.at(1), defaultValue.y));
        }

        return defaultValue;
    }

    static Math::Float3 Parse(ShuntingYard &shuntingYard, jsoncons::json const &object, Math::Float3 const &defaultValue)
    {
        if (object.is_array() && object.size() == 3)
        {
            return Math::Float3(
                Parse(shuntingYard, object.at(0), defaultValue.x),
                Parse(shuntingYard, object.at(1), defaultValue.y),
                Parse(shuntingYard, object.at(2), defaultValue.z));
        }

        return defaultValue;
    }

    static Math::Float4 Parse(ShuntingYard &shuntingYard, jsoncons::json const &object, Math::Float4 const &defaultValue)
    {
        if (object.is_array())
        {
            if (object.size() == 3)
            {
                return Math::Float4(
                    Parse(shuntingYard, object.at(0), defaultValue.x),
                    Parse(shuntingYard, object.at(1), defaultValue.y),
                    Parse(shuntingYard, object.at(2), defaultValue.z), 1.0f);
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

        return defaultValue;
    }

    static Math::Quaternion Parse(ShuntingYard &shuntingYard, jsoncons::json const &object, Math::Quaternion const &defaultValue)
    {
        if (object.is_array())
        {
            if (object.size() == 3)
            {
                return Math::Quaternion::FromEuler(
                    Parse(shuntingYard, object.at(0), defaultValue.x),
                    Parse(shuntingYard, object.at(1), defaultValue.y),
                    Parse(shuntingYard, object.at(2), defaultValue.z));
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

    static std::string Convert(jsoncons::json const &object, std::string const &defaultValue)
    {
        return object.as_string();
    }

    static bool Convert(jsoncons::json const &object, bool defaultValue)
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

    static int32_t Convert(jsoncons::json const &object, int32_t defaultValue)
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

    static uint32_t Convert(jsoncons::json const &object, uint32_t defaultValue)
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

    static float Convert(jsoncons::json const &object, float defaultValue)
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

    static Math::Float2 Convert(jsoncons::json const &object, Math::Float2 const &defaultValue)
    {
        if (object.is_array() && object.size() == 2)
        {
            return Math::Float2(
                Convert(object.at(0), defaultValue.x),
                Convert(object.at(1), defaultValue.y));
        }

        return defaultValue;
    }

    static Math::Float3 Convert(jsoncons::json const &object, Math::Float3 const &defaultValue)
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

    static Math::Float4 Convert(jsoncons::json const &object, Math::Float4 const &defaultValue)
    {
        if (object.is_array())
        {
            if (object.size() == 3)
            {
                return Math::Float4(
                    Convert(object.at(0), defaultValue.x),
                    Convert(object.at(1), defaultValue.y),
                    Convert(object.at(2), defaultValue.z), 1.0f);
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

    static Math::Quaternion Convert(jsoncons::json const &object, Math::Quaternion const &defaultValue)
    {
        if (object.is_array())
        {
            if (object.size() == 3)
            {
                return Math::Quaternion::FromEuler(
                    Convert(object.at(0), defaultValue.x),
                    Convert(object.at(1), defaultValue.y),
                    Convert(object.at(2), defaultValue.z));
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

    JSON::JSON(jsoncons::json &object)
        : object(object)
    {
    }

    JSON &JSON::operator = (jsoncons::json &object)
    {
        this->object = object;
        return (*this);
    }

    jsoncons::json JSON::Load(FileSystem::Path const &filePath)
    {
        std::string object(FileSystem::Load(filePath, String::Empty));
        std::istringstream dataStream(object);
        jsoncons::json_decoder<jsoncons::json> decoder;
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

    void JSON::Save(FileSystem::Path const &filePath)
    {
        std::ostringstream stream;
        stream << jsoncons::pretty_print(object);
        FileSystem::Save(filePath, stream.str());
    }

    JSON::Members JSON::getMembers(void) const
    {
        return (object.is_object() ? ((jsoncons::json const &)object).object_range() : EmptyObject.object_range());
    }

    JSON::Elements JSON::getElements(void) const
    {
        return (object.is_array() ? ((jsoncons::json const &)object).array_range() : EmptyArray.array_range());
    }

    jsoncons::json const &JSON::get(std::string const &name) const
    {
        if (object.is_object())
        {
            auto objectSearch = object.find(name);
            if (objectSearch != object.end_members())
            {
                return objectSearch->value;
            }
        }

        return EmptyObject;
    }

    jsoncons::json const &JSON::at(size_t index) const
    {
        if (object.is_array())
        {
            return object.at(index);
        }
    }

    std::string JSON::convert(std::string const &defaultValue)
    {
        return Convert(object, defaultValue);
    }

    bool JSON::convert(bool defaultValue)
    {
        return Convert(object, defaultValue);
    }

    int32_t JSON::convert(int32_t defaultValue)
    {
        return Convert(object, defaultValue);
    }

    uint32_t JSON::convert(uint32_t defaultValue)
    {
        return Convert(object, defaultValue);
    }

    float JSON::convert(float defaultValue)
    {
        return Convert(object, defaultValue);
    }

    Math::Float2 JSON::convert(Math::Float2 const &defaultValue)
    {
        return Convert(object, defaultValue);
    }

    Math::Float3 JSON::convert(Math::Float3 const &defaultValue)
    {
        return Convert(object, defaultValue);
    }

    Math::Float4 JSON::convert(Math::Float4 const &defaultValue)
    {
        return Convert(object, defaultValue);
    }

    Math::Quaternion JSON::convert(Math::Quaternion const &defaultValue)
    {
        return Convert(object, defaultValue);
    }

    std::string JSON::parse(ShuntingYard &shuntingYard, std::string const &defaultValue)
    {
        return Parse(shuntingYard, object, defaultValue);
    }

    bool JSON::parse(ShuntingYard &shuntingYard, bool defaultValue)
    {
        return Parse(shuntingYard, object, defaultValue);
    }

    int32_t JSON::parse(ShuntingYard &shuntingYard, int32_t defaultValue)
    {
        return Parse(shuntingYard, object, defaultValue);
    }

    uint32_t JSON::parse(ShuntingYard &shuntingYard, uint32_t defaultValue)
    {
        return Parse(shuntingYard, object, defaultValue);
    }

    float JSON::parse(ShuntingYard &shuntingYard, float defaultValue)
    {
        return Parse(shuntingYard, object, defaultValue);
    }

    Math::Float2 JSON::parse(ShuntingYard &shuntingYard, Math::Float2 const &defaultValue)
    {
        return Parse(shuntingYard, object, defaultValue);
    }

    Math::Float3 JSON::parse(ShuntingYard &shuntingYard, Math::Float3 const &defaultValue)
    {
        return Parse(shuntingYard, object, defaultValue);
    }

    Math::Float4 JSON::parse(ShuntingYard &shuntingYard, Math::Float4 const &defaultValue)
    {
        return Parse(shuntingYard, object, defaultValue);
    }

    Math::Quaternion JSON::parse(ShuntingYard &shuntingYard, Math::Quaternion const &defaultValue)
    {
        return Parse(shuntingYard, object, defaultValue);
    }

    jsoncons::json JSON::Make(std::string const &value)
    {
        return value;
    }

    jsoncons::json JSON::Make(bool value)
    {
        return value;
    }

    jsoncons::json JSON::Make(int32_t value)
    {
        return value;
    }

    jsoncons::json JSON::Make(uint32_t value)
    {
        return value;
    }

    jsoncons::json JSON::Make(float value)
    {
        return value;
    }

    jsoncons::json JSON::Make(Math::Float2 const &value)
    {
        return jsoncons::json::make_array({
            value.x,
            value.y,
        });
    }

    jsoncons::json JSON::Make(Math::Float3 const &value)
    {
        return jsoncons::json::make_array({
            value.x,
            value.y,
            value.z,
        });
    }

    jsoncons::json JSON::Make(Math::Float4 const &value)
    {
        return jsoncons::json::make_array({
            value.x,
            value.y,
            value.z,
            value.w,
        });
    }

    jsoncons::json JSON::Make(Math::Quaternion const &value)
    {
        return jsoncons::json::make_array({
            value.x,
            value.y,
            value.z,
            value.w,
        });
    }
}; // namespace Gek
