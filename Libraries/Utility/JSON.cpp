#include "GEK/Utility/JSON.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include "GEK/Utility/Context.hpp"
#include <jsoncons/json.hpp>

namespace Gek
{
    const JSON::Array JSON::EmptyArray = JSON::Array();
    const JSON::Object JSON::EmptyObject = JSON::Object();
    const JSON JSON::Empty = JSON();

    void LoadJSON(JSON &value, jsoncons::json const &object)
    {
        if (object.is_empty() || object.is_null())
        {
            return;
        }

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
            if (true)
            {
                auto &data = value = JSON::Array(object.size());
                for (size_t index = 0; index < object.size(); ++index)
                {
                    LoadJSON(data[index], object[index]);
                }
            }

            break;

        case jsoncons::value_type::object_t:
            if (true)
            {
                auto &data = value = JSON::Object();
                for (auto &pair : object.members())
                {
                    LoadJSON(data[pair.name()], pair.value());
                }
            }

            break;
        };
    }

    void SaveJSON(JSON const &value, std::stringstream &stream)
    {
        value.visit([&](auto && data)
        {
            using TYPE = std::decay_t<decltype(data)>;
            if constexpr (std::is_same_v<TYPE, std::string>)
            {
                stream << "\"" << data << "\"";
            }
            else if constexpr (std::is_same_v<TYPE, JSON::Array>)
            {
                stream << "[ ";
                bool writtenPrevious = false;
                for (auto &index : data)
                {
                    SaveJSON(index, stream);
                    if (writtenPrevious)
                    {
                        stream << ", ";
                    }

                    writtenPrevious = true;
                }

                stream << "]";
            }
            else if constexpr (std::is_same_v<TYPE, JSON::Object>)
            {
                stream << "{ ";
                bool writtenPrevious = false;
                for (auto &index : data)
                {
                    stream << "\"" << index.first << "\": ";
                    SaveJSON(index.second, stream);
                    if (writtenPrevious)
                    {
                        stream << ", ";
                    }

                    writtenPrevious = true;
                }

                stream << "}";
            }
            else
            {
                stream << data;
            }
        });
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
            LoadJSON(*this, decoder.get_result());
        }
    }

    void JSON::save(FileSystem::Path const &filePath)
    {
        std::stringstream stream;
        SaveJSON(*this, stream);
        FileSystem::Save(filePath, stream.str());
    }

    JSON const &JSON::get(size_t index) const
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

    JSON const &JSON::get(std::string_view name) const
    {
        if (auto value = std::get_if<Object>(&data))
        {
            return value->at(name.data());
        }
        else
        {
            return Empty;
        }
    }

    JSON &JSON::operator [] (size_t index)
    {
        if (!is<Array>())
        {
            data = EmptyArray;
        }

        return as<Array>(EmptyArray)[index];
    }

    JSON &JSON::operator [] (std::string_view name)
    {
        if (!is<Object>())
        {
            data = EmptyObject;
        }

        return as<Object>(EmptyObject)[name.data()];
    }

    int32_t JSON::evaluate(ShuntingYard &shuntingYard, int32_t defaultValue) const
    {
        return visit([&](auto && data) -> int32_t
        {
            using TYPE = std::decay_t<decltype(data)>;
            if constexpr (std::is_same_v<TYPE, std::string>)
            {
                return shuntingYard.evaluate(data).value_or(defaultValue);
            }
            else if constexpr (std::is_same_v<TYPE, Object> ||
                std::is_same_v<TYPE, Array>)
            {
                return defaultValue;
            }
            else
            {
                return data;
            }
        });
    }

    uint32_t JSON::evaluate(ShuntingYard &shuntingYard, uint32_t defaultValue) const
    {
        return visit([&](auto && data) -> uint32_t
        {
            using TYPE = std::decay_t<decltype(data)>;
            if constexpr (std::is_same_v<TYPE, std::string>)
            {
                return shuntingYard.evaluate(data).value_or(defaultValue);
            }
            else if constexpr (std::is_same_v<TYPE, Object> ||
                std::is_same_v<TYPE, Array>)
            {
                return defaultValue;
            }
            else
            {
                return data;
            }
        });
    }

    float JSON::evaluate(ShuntingYard &shuntingYard, float defaultValue) const
    {
        return visit([&](auto && data) -> float
        {
            using TYPE = std::decay_t<decltype(data)>;
            if constexpr (std::is_same_v<TYPE, std::string>)
            {
                return shuntingYard.evaluate(data).value_or(defaultValue);
            }
            else if constexpr (std::is_same_v<TYPE, Object> ||
                std::is_same_v<TYPE, Array>)
            {
                return defaultValue;
            }
            else
            {
                return data;
            }
        });
    }

    Math::Float2 JSON::evaluate(ShuntingYard &shuntingYard, Math::Float2 const &defaultValue) const
    {
        auto data = as(EmptyArray);
        switch (data.size())
        {
        case 1:
            return Math::Float2(data[0].evaluate(shuntingYard, defaultValue.x));

        default:
            return Math::Float2(
                data[0].evaluate(shuntingYard, defaultValue.x),
                data[1].evaluate(shuntingYard, defaultValue.y));
        };
    }

    Math::Float3 JSON::evaluate(ShuntingYard &shuntingYard, Math::Float3 const &defaultValue) const
    {
        auto data = as(EmptyArray);
        switch (data.size())
        {
        case 1:
            return Math::Float3(data[0].evaluate(shuntingYard, defaultValue.x));

        default:
            return Math::Float3(
                data[0].evaluate(shuntingYard, defaultValue.x),
                data[1].evaluate(shuntingYard, defaultValue.y),
                data[2].evaluate(shuntingYard, defaultValue.z));
        };
    }

    Math::Float4 JSON::evaluate(ShuntingYard &shuntingYard, Math::Float4 const &defaultValue) const
    {
        auto data = as(EmptyArray);
        switch (data.size())
        {
        case 1:
            return Math::Float4(data[0].evaluate(shuntingYard, defaultValue.x));

        case 3:
            return Math::Float4(
                data[0].evaluate(shuntingYard, defaultValue.x),
                data[1].evaluate(shuntingYard, defaultValue.y),
                data[2].evaluate(shuntingYard, defaultValue.z),
                defaultValue.w);

        default:
            return Math::Float4(
                data[0].evaluate(shuntingYard, defaultValue.x),
                data[1].evaluate(shuntingYard, defaultValue.y),
                data[2].evaluate(shuntingYard, defaultValue.z),
                data[3].evaluate(shuntingYard, defaultValue.w));
        };
    }

    Math::Quaternion JSON::evaluate(ShuntingYard &shuntingYard, Math::Quaternion const &defaultValue) const
    {
        auto data = as(EmptyArray);
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
        return visit([&](auto && data) -> std::string
        {
            using TYPE = std::decay_t<decltype(data)>;
            if constexpr (std::is_same_v<TYPE, std::string>)
            {
                return data;
            }
            else if constexpr (std::is_same_v<TYPE, JSON::Array> || 
                               std::is_same_v<TYPE, JSON::Object>)
            {
                return defaultValue;
            }
            else
            {
                return String::Format("{}", data);
            }
        });
    }
}; // namespace Gek
