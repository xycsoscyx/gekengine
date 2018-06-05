#include "GEK/Utility/JSON.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include "GEK/Utility/Context.hpp"
#include <jsoncons/json.hpp>

namespace Gek
{
    const JSON::Array JSON::EmptyArray = JSON::Array();
    const JSON::Object JSON::EmptyObject = JSON::Object();
    const JSON JSON::Empty = JSON();

    JSON GetFromJSON(jsoncons::json const &object)
    {
        if (object.is_empty() || object.is_null())
        {
            return JSON::Empty;
        }

        JSON value;
        switch (object.type_id())
        {
        case jsoncons::value_type::small_string_t:
        case jsoncons::value_type::string_t:
            value = object.as_string();
            break;

        case jsoncons::value_type::bool_t:
            value = object.as_bool();
            break;

        case jsoncons::value_type::double_t:
            value = float(object.as_double());
            break;

        case jsoncons::value_type::integer_t:
            value = object.as_integer();
            break;

        case jsoncons::value_type::uinteger_t:
            value = object.as_uinteger();
            break;

        case jsoncons::value_type::array_t:
            value = JSON::Array(object.size());
            for (size_t index = 0; index < object.size(); ++index)
            {
                value[index] = GetFromJSON(object[index]);
            }

            break;

        case jsoncons::value_type::object_t:
            value = JSON::Object();
            for (auto &pair : object.members())
            {
                value[pair.name()] = GetFromJSON(pair.value());
            }

            break;
        };

        return value;
    }

    void JSON::load(FileSystem::Path const &filePath)
    {
        std::string object(FileSystem::Load(filePath, String::Empty));
        std::istringstream dataStream(object);
        jsoncons::json_decoder<jsoncons::json> decoder;
        jsoncons::json_reader reader(dataStream, decoder);

        std::error_code errorCode;
        reader.read(errorCode);
        if (errorCode)
        {
            LockedWrite{ std::cerr } << errorCode.message() << " at line " << reader.line_number() << ", and column " << reader.column_number();
        }
        else
        {
            *this = GetFromJSON(decoder.get_result());
        }
    }

    void JSON::save(FileSystem::Path const &filePath)
    {
        FileSystem::Save(filePath, getString());
    }

    std::string JSON::getString(void) const
    {
        return visit(
            [](std::string const  &visitedData)
        {
            return String::Format("\"{}\"", visitedData);
        },
            [](JSON::Array const &visitedData)
        {
            std::stringstream stream;

            stream << "[ ";
            bool writtenPrevious = false;
            for (auto &index : visitedData)
            {
                stream << index.getString();
                if (writtenPrevious)
                {
                    stream << ", ";
                }

                writtenPrevious = true;
            }

            stream << "]";
            return stream.str();
        },
            [](JSON::Object const &visitedData)
        {
            std::stringstream stream;

            stream << "{ ";
            bool writtenPrevious = false;
            for (auto &index : visitedData)
            {
                stream << "\"" << index.first << "\": ";
                stream << index.second.getString();
                if (writtenPrevious)
                {
                    stream << ", ";
                }

                writtenPrevious = true;
            }

            stream << "}";
            return stream.str();
        },
            [](auto && visitedData)
        {
            return String::Format("{}", visitedData);
        });
    }

    JSON const &JSON::getIndex(size_t index) const
    {
        if (auto value = std::get_if<Array>(&data))
        {
            return value->at(index);
        }
        else
        {
            return Empty;
        }
    }

    JSON const &JSON::getMember(std::string_view name) const
    {
        if (auto value = std::get_if<Object>(&data))
        {
            auto search = value->find(name.data());
            if (search == std::end(*value))
            {
                return Empty;
            }
            else
            {
                return search->second;
            }
        }
        else
        {
            return Empty;
        }
    }

    JSON &JSON::operator [] (size_t index)
    {
        if (!isType<Array>())
        {
            data = EmptyArray;
        }

        return std::get<Array>(data)[index];
    }

    JSON &JSON::operator [] (std::string_view name)
    {
        if (!isType<Object>())
        {
            data = EmptyObject;
        }

        return std::get<Object>(data)[name.data()];
    }

    Math::Float2 JSON::evaluate(ShuntingYard &shuntingYard, Math::Float2 const &defaultValue) const
    {
        auto data = asType(EmptyArray);
        switch (data.size())
        {
        case 1:
            return Math::Float2(data[0].evaluate(shuntingYard, 0.0f));

        default:
            return Math::Float2(
                data[0].evaluate(shuntingYard, defaultValue.x),
                data[1].evaluate(shuntingYard, defaultValue.y));
        };
    }

    Math::Float3 JSON::evaluate(ShuntingYard &shuntingYard, Math::Float3 const &defaultValue) const
    {
        auto data = asType(EmptyArray);
        switch (data.size())
        {
        case 1:
            return Math::Float3(data[0].evaluate(shuntingYard, 0.0f));

        case 3:
            return Math::Float3(
                data[0].evaluate(shuntingYard, defaultValue.x),
                data[1].evaluate(shuntingYard, defaultValue.y),
                data[2].evaluate(shuntingYard, defaultValue.z));

        default:
            return defaultValue;
        };
    }

    Math::Float4 JSON::evaluate(ShuntingYard &shuntingYard, Math::Float4 const &defaultValue) const
    {
        auto data = asType(EmptyArray);
        switch (data.size())
        {
        case 1:
            return Math::Float4(data[0].evaluate(shuntingYard, 0.0f));

        case 3:
            return Math::Float4(
                data[0].evaluate(shuntingYard, defaultValue.x),
                data[1].evaluate(shuntingYard, defaultValue.y),
                data[2].evaluate(shuntingYard, defaultValue.z),
                defaultValue.w);

        case 4:
            return Math::Float4(
                data[0].evaluate(shuntingYard, defaultValue.x),
                data[1].evaluate(shuntingYard, defaultValue.y),
                data[2].evaluate(shuntingYard, defaultValue.z),
                data[3].evaluate(shuntingYard, defaultValue.w));

        default:
            return defaultValue;
        };
    }

    Math::Quaternion JSON::evaluate(ShuntingYard &shuntingYard, Math::Quaternion const &defaultValue) const
    {
        auto data = asType(EmptyArray);
        switch (data.size())
        {
        case 3:
            if (true)
            {
                float pitch = data[0].evaluate(shuntingYard, Math::Infinity);
                float yaw = data[1].evaluate(shuntingYard, Math::Infinity);
                float roll = data[2].evaluate(shuntingYard, Math::Infinity);
                if (pitch != Math::Infinity && yaw != Math::Infinity && roll != Math::Infinity)
                {
                    return Math::Quaternion::MakeEulerRotation(pitch, yaw, roll);
                }

                return defaultValue;
            }

        case 4:
            return Math::Quaternion(
                data[0].evaluate(shuntingYard, defaultValue.x),
                data[1].evaluate(shuntingYard, defaultValue.y),
                data[2].evaluate(shuntingYard, defaultValue.z),
                data[3].evaluate(shuntingYard, defaultValue.w));

        default:
            return defaultValue;
        };
    }

    std::string JSON::evaluate(ShuntingYard &shuntingYard, std::string const &defaultValue) const
    {
        return visit(
            [](std::string const &visitedData)
        {
            return visitedData;
        },
            [defaultValue](Object const &visitedData)
        {
            return defaultValue;
        },
            [defaultValue](Array const &visitedData)
        {
            return defaultValue;
        },
            [defaultValue](auto const &visitedData)
        {
            return String::Format("{}", GetValueOrDefault(visitedData, defaultValue));
        });
    }
}; // namespace Gek
