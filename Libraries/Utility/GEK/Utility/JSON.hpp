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
#include <jsoncons/json.hpp>

namespace Gek
{
    namespace JSON
    {
        using Object = jsoncons::json;
        extern const Object EmptyObject;
        extern const Object EmptyArray;

        std::string Parse(ShuntingYard &shuntingYard, Object const &object, std::string const &defaultValue);
        bool Parse(ShuntingYard &shuntingYard, Object const &object, bool defaultValue);
        int32_t Parse(ShuntingYard &shuntingYard, Object const &object, int32_t defaultValue);
        uint32_t Parse(ShuntingYard &shuntingYard, Object const &object, uint32_t defaultValue);
        float Parse(ShuntingYard &shuntingYard, Object const &object, float defaultValue);
        Math::Float2 Parse(ShuntingYard &shuntingYard, Object const &object, Math::Float2 const &defaultValue);
        Math::Float3 Parse(ShuntingYard &shuntingYard, Object const &object, Math::Float3 const &defaultValue);
        Math::Float4 Parse(ShuntingYard &shuntingYard, Object const &object, Math::Float4 const &defaultValue);
        Math::Quaternion Parse(ShuntingYard &shuntingYard, Object const &object, Math::Quaternion const &defaultValue);

        std::string Convert(Object const &object, std::string const &defaultValue);
        bool Convert(Object const &object, bool defaultValue);
        int32_t Convert(Object const &object, int32_t defaultValue);
        uint32_t Convert(Object const &object, uint32_t defaultValue);
        float Convert(Object const &object, float defaultValue);
        Math::Float2 Convert(Object const &object, Math::Float2 const &defaultValue);
        Math::Float3 Convert(Object const &object, Math::Float3 const &defaultValue);
        Math::Float4 Convert(Object const &object, Math::Float4 const &defaultValue);
        Math::Quaternion Convert(Object const &object, Math::Quaternion const &defaultValue);

        template <typename TYPE>
        class Typeless
        {
        private:
            TYPE object;

        public:
            Typeless(TYPE object)
                : object(object)
            {
            }

            Typeless &operator = (TYPE object)
            {
                this->object = object;
                return (*this);
            }

            Object const &getObject(void) const
            {
                return object;
            }

            bool isFloat() const
            {
                return object.is_double();
            }

            void save(FileSystem::Path const &filePath)
            {
                std::ostringstream stream;
                stream << jsoncons::pretty_print(object);
                FileSystem::Save(filePath, stream.str());
            }

            auto getArray(void)
            {
                return (object.is_array() ? object.array_value() : EmptyArray.array_value());
            }

            auto getMembers(void) const
            {
                return (object.is_object() ? ((Object const &)object).object_range() : EmptyObject.object_range());
            }

            Typeless<Object const &> get(std::string const &name) const
            {
                if (object.is_object())
                {
                    auto objectSearch = object.find(name);
                    if (objectSearch != object.end_members())
                    {
                        return objectSearch->value();
                    }
                }

                return EmptyObject;
            }

            Typeless<Object const &> at(size_t index) const
            {
                if (object.is_array())
                {
                    return object.at(index);
                }

                return EmptyObject;
            }

            std::string convert(std::string const &defaultValue)
            {
                return Convert(object, defaultValue);
            }

            bool convert(bool defaultValue)
            {
                return Convert(object, defaultValue);
            }

            int32_t convert(int32_t defaultValue)
            {
                return Convert(object, defaultValue);
            }

            uint32_t convert(uint32_t defaultValue)
            {
                return Convert(object, defaultValue);
            }

            float convert(float defaultValue)
            {
                return Convert(object, defaultValue);
            }

            Math::Float2 convert(Math::Float2 const &defaultValue)
            {
                return Convert(object, defaultValue);
            }

            Math::Float3 convert(Math::Float3 const &defaultValue)
            {
                return Convert(object, defaultValue);
            }

            Math::Float4 convert(Math::Float4 const &defaultValue)
            {
                return Convert(object, defaultValue);
            }

            Math::Quaternion convert(Math::Quaternion const &defaultValue)
            {
                return Convert(object, defaultValue);
            }

            std::string parse(ShuntingYard &shuntingYard, std::string const &defaultValue)
            {
                return Parse(shuntingYard, object, defaultValue);
            }

            bool parse(ShuntingYard &shuntingYard, bool defaultValue)
            {
                return Parse(shuntingYard, object, defaultValue);
            }

            int32_t parse(ShuntingYard &shuntingYard, int32_t defaultValue)
            {
                return Parse(shuntingYard, object, defaultValue);
            }

            uint32_t parse(ShuntingYard &shuntingYard, uint32_t defaultValue)
            {
                return Parse(shuntingYard, object, defaultValue);
            }

            float parse(ShuntingYard &shuntingYard, float defaultValue)
            {
                return Parse(shuntingYard, object, defaultValue);
            }

            Math::Float2 parse(ShuntingYard &shuntingYard, Math::Float2 const &defaultValue)
            {
                return Parse(shuntingYard, object, defaultValue);
            }

            Math::Float3 parse(ShuntingYard &shuntingYard, Math::Float3 const &defaultValue)
            {
                return Parse(shuntingYard, object, defaultValue);
            }

            Math::Float4 parse(ShuntingYard &shuntingYard, Math::Float4 const &defaultValue)
            {
                return Parse(shuntingYard, object, defaultValue);
            }

            Math::Quaternion parse(ShuntingYard &shuntingYard, Math::Quaternion const &defaultValue)
            {
                return Parse(shuntingYard, object, defaultValue);
            }
        };

        using Instance = Typeless<Object>;
        using Reference = Typeless<Object const &>;

        Object Load(FileSystem::Path const &filePath);

        Object Make(std::string const &value);
        Object Make(bool value);
        Object Make(int32_t value);
        Object Make(uint32_t value);
        Object Make(float value);
        Object Make(Math::Float2 const &value);
        Object Make(Math::Float3 const &value);
        Object Make(Math::Float4 const &value);
        Object Make(Math::Quaternion const &value);
    }; // namespace JSON
}; // namespace Gek
