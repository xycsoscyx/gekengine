#include "GEK/Utility/JSON.hpp"
#include "GEK/Utility/FileSystem.hpp"
#include "GEK/Utility/Context.hpp"

namespace Gek
{
    namespace JSON
    {
        const Object EmptyObject = Object();
        const Array EmptyArray = Array();

		Object Load(FileSystem::Path const &filePath, const Object &defaultValue)
		{
			std::string data(FileSystem::Load(filePath, String::Empty));
			std::istringstream dataStream(data);
			jsoncons::json_decoder<jsoncons::json> decoder;
			jsoncons::json_reader reader(dataStream, decoder);

			std::error_code errorCode;
			reader.read(errorCode);

			if (errorCode)
			{
				return defaultValue;
                std::cerr << errorCode.message() << " at line " << reader.line_number() << " and column " << reader.column_number() << std::endl;
			}
			else
			{
				return decoder.get_result();
			}
		}

        void Save(FileSystem::Path const &filePath, Object const &object)
        {
            std::ostringstream stream;
            stream << jsoncons::pretty_print(object);
            FileSystem::Save(filePath, stream.str());
        }

        Members GetMembers(Object const &object)
        {
            return (object.is_object() ? object.object_range() : EmptyObject.object_range());
        }

        Object const &Get(Object const &object, std::string const &name, Object const &defaultValue)
        {
            if (object.type_id() == jsoncons::value_type::object_t)
            {
                auto objectSearch = object.find(name);
                if (objectSearch != object.object_range().end())
                {
                    return objectSearch->value();
                }
                else
                {
                    return defaultValue;
                }
            }

            return defaultValue;
        }

        Elements GetElements(Object const &object)
        {
            return (object.is_array() ? object.array_range() : Elements(std::begin(EmptyArray), std::end(EmptyArray)));
        }

        Object const &At(Object const &object, size_t index, Object const &defaultValue)
        {
            return (object.is_array() ? object.at(index) : defaultValue);
        }

        float From(JSON::Object const &data, ShuntingYard &parser, float defaultValue)
        {
            switch (data.type_id())
            {
            case jsoncons::value_type::small_string_t:
            case jsoncons::value_type::string_t:
                return parser.evaluate(data.as_string(), defaultValue);

            case jsoncons::value_type::double_t:
                return static_cast<float>(data.var_.double_data_cast()->value());

            case jsoncons::value_type::integer_t:
                return static_cast<float>(data.var_.integer_data_cast()->value());

            case jsoncons::value_type::uinteger_t:
                return static_cast<float>(data.var_.uinteger_data_cast()->value());

            case jsoncons::value_type::bool_t:
                return data.var_.bool_data_cast()->value() ? 1.0f : 0.0f;

            default:
                return defaultValue;
            };
        }

        Math::Float2 From(JSON::Object const &data, ShuntingYard &parser, Math::Float2 const &defaultValue)
        {
            if (data.is_array() && data.size() == 2)
            {
                return Math::Float2(
                    From(data.at(0), parser, defaultValue.x),
                    From(data.at(1), parser, defaultValue.y));
            }

            return defaultValue;
        }

        Math::Float3 From(JSON::Object const &data, ShuntingYard &parser, Math::Float3 const &defaultValue)
        {
            if (data.is_array() && data.size() == 3)
            {
                return Math::Float3(
                    From(data.at(0), parser, defaultValue.x),
                    From(data.at(1), parser, defaultValue.y),
                    From(data.at(2), parser, defaultValue.z));
            }

            return defaultValue;
        }

        Math::Float4 From(JSON::Object const &data, ShuntingYard &parser, Math::Float4 const &defaultValue)
        {
            if (data.is_array())
            {
                if (data.size() == 3)
                {
                    return Math::Float4(
                        From(data.at(0), parser, defaultValue.x),
                        From(data.at(1), parser, defaultValue.y),
                        From(data.at(2), parser, defaultValue.z), 1.0f);
                }
                else if (data.size() == 4)
                {
                    return Math::Float4(
                        From(data.at(0), parser, defaultValue.x),
                        From(data.at(1), parser, defaultValue.y),
                        From(data.at(2), parser, defaultValue.z),
                        From(data.at(3), parser, defaultValue.w));
                }
            }

            return defaultValue;
        }

        Math::Quaternion From(JSON::Object const &data, ShuntingYard &parser, Math::Quaternion const &defaultValue)
        {
            if (data.is_array())
            {
                if (data.size() == 3)
                {
                    return Math::Quaternion::FromEuler(
                        From(data.at(0), parser, defaultValue.x),
                        From(data.at(1), parser, defaultValue.y),
                        From(data.at(2), parser, defaultValue.z));
                }
                else if (data.size() == 4)
                {
                    return Math::Quaternion(
                        From(data.at(0), parser, defaultValue.x),
                        From(data.at(1), parser, defaultValue.y),
                        From(data.at(2), parser, defaultValue.z),
                        From(data.at(3), parser, defaultValue.w));
                }
            }

            return defaultValue;
        }

        int32_t From(JSON::Object const &data, ShuntingYard &parser, int32_t defaultValue)
        {
            switch (data.type_id())
            {
            case jsoncons::value_type::small_string_t:
            case jsoncons::value_type::string_t:
                return static_cast<int32_t>(parser.evaluate(data.as_string(), defaultValue));

            case jsoncons::value_type::double_t:
                return static_cast<int64_t>(data.var_.double_data_cast()->value());

            case jsoncons::value_type::integer_t:
                return static_cast<int64_t>(data.var_.integer_data_cast()->value());

            case jsoncons::value_type::uinteger_t:
                return static_cast<int64_t>(data.var_.uinteger_data_cast()->value());

            case jsoncons::value_type::bool_t:
                return data.var_.bool_data_cast()->value() ? 1 : 0;

            default:
                return defaultValue;
            };
        }

        uint32_t From(JSON::Object const &data, ShuntingYard &parser, uint32_t defaultValue)
        {
            switch (data.type_id())
            {
            case jsoncons::value_type::small_string_t:
            case jsoncons::value_type::string_t:
                return static_cast<uint32_t>(parser.evaluate(data.as_string(), defaultValue));

            case jsoncons::value_type::double_t:
                return static_cast<uint32_t>(data.var_.double_data_cast()->value());

            case jsoncons::value_type::integer_t:
                return static_cast<uint32_t>(data.var_.integer_data_cast()->value());

            case jsoncons::value_type::uinteger_t:
                return static_cast<uint32_t>(data.var_.uinteger_data_cast()->value());

            case jsoncons::value_type::bool_t:
                return data.var_.bool_data_cast()->value() ? 1 : 0;

            default:
                return defaultValue;
            };
        }

        bool From(JSON::Object const &data, ShuntingYard &parser, bool defaultValue)
        {
            switch (data.type_id())
            {
            case jsoncons::value_type::small_string_t:
            case jsoncons::value_type::string_t:
                return parser.evaluate(data.as_string(), defaultValue) != 0.0f;

            case jsoncons::value_type::bool_t:
                return data.var_.bool_data_cast()->value();

            case jsoncons::value_type::double_t:
                return data.var_.double_data_cast()->value() != 0.0;

            case jsoncons::value_type::integer_t:
                return data.var_.integer_data_cast()->value() != 0;

            case jsoncons::value_type::uinteger_t:
                return data.var_.uinteger_data_cast()->value() != 0;

            default:
                return defaultValue;
            };
        }

        std::string From(JSON::Object const &data, ShuntingYard &parser, std::string const &defaultValue)
        {
            return data.as_string();
        }

        JSON::Object To(float value)
        {
            return value;
        }

        JSON::Object To(Math::Float2 const &value)
        {
            return Array {
                value.x,
                value.y,
            };
        }

        JSON::Object To(Math::Float3 const &value)
        {
            return Array{
                value.x,
                value.y,
                value.z,
            };
        }

        JSON::Object To(Math::Float4 const &value)
        {
            return Array{
                value.x,
                value.y,
                value.z,
                value.w
            };
        }

        JSON::Object To(Math::Quaternion const &value)
        {
            return Array{
                value.x,
                value.y,
                value.z,
                value.w
            };
        }

        JSON::Object To(int32_t value)
        {
            return value;
        }

        JSON::Object To(uint32_t value)
        {
            return value;
        }

        JSON::Object To(bool value)
        {
            return value;
        }

        JSON::Object To(std::string const &value)
        {
            return value;
        }
    }; // namespace JSON
}; // namespace Gek
