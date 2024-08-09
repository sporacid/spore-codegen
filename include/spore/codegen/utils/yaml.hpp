#pragma once

#include <cmath>
#include <cstdint>
#include <string>

#include "nlohmann/json.hpp"
#include "yaml-cpp/yaml.h"

namespace spore::codegen::yaml
{
    namespace detail
    {
        inline nlohmann::json parse_scalar(const YAML::Node& node)
        {
            bool b;
            if (YAML::convert<bool>::decode(node, b))
                return b;

            std::int64_t i;
            if (YAML::convert<std::int64_t>::decode(node, i))
                return i;

            std::double_t d;
            if (YAML::convert<std::double_t>::decode(node, d))
                return d;

            std::string s;
            if (YAML::convert<std::string>::decode(node, s))
                return s;

            return nullptr;
        }

        inline void to_yaml(const nlohmann::json& json, YAML::Emitter& yaml)
        {
            switch (json.type())
            {
                case nlohmann::detail::value_t::null: {
                    yaml << YAML::Null;
                    break;
                }

                case nlohmann::detail::value_t::object: {
                    yaml << YAML::BeginMap;

                    for (auto it_json = json.begin(); it_json != json.end(); ++it_json)
                    {
                        yaml << YAML::Key << it_json.key() << YAML::Value;
                        to_yaml(it_json.value(), yaml);
                    }

                    yaml << YAML::EndMap;
                    break;
                }

                case nlohmann::detail::value_t::array: {
                    yaml << YAML::BeginSeq;

                    for (auto&& json_value : json)
                    {
                        yaml << YAML::Value;
                        to_yaml(json_value, yaml);
                    }

                    yaml << YAML::EndSeq;
                    break;
                }

                case nlohmann::detail::value_t::string: {
                    yaml << YAML::Value << json.get<std::string>();
                    break;
                }

                case nlohmann::detail::value_t::boolean: {
                    yaml << YAML::Value << json.get<bool>();
                    break;
                }

                case nlohmann::detail::value_t::number_integer: {
                    yaml << YAML::Value << json.get<std::int64_t>();
                    break;
                }

                case nlohmann::detail::value_t::number_unsigned: {
                    yaml << YAML::Value << json.get<std::uint64_t>();
                    break;
                }

                case nlohmann::detail::value_t::number_float: {
                    yaml << YAML::Value << json.get<std::double_t>();
                    break;
                }

                default: {
                    break;
                }
            }
        }

        inline nlohmann::json from_yaml(const YAML::Node& yaml)
        {
            switch (yaml.Type())
            {
                case YAML::NodeType::Null: {
                    return nlohmann::json(nlohmann::json::value_t::null);
                }

                case YAML::NodeType::Scalar: {
                    return parse_scalar(yaml);
                }

                case YAML::NodeType::Sequence: {
                    nlohmann::json json;
                    for (auto&& node : yaml)
                    {
                        json.emplace_back(from_yaml(node));
                    }

                    return json;
                }

                case YAML::NodeType::Map: {
                    nlohmann::json json;
                    for (auto&& pair : yaml)
                    {
                        json[pair.first.as<std::string>()] = from_yaml(pair.second);
                    }

                    return json;
                }

                default: {
                    return nlohmann::json(nlohmann::json::value_t::discarded);
                }
            }
        }
    }

    inline std::string to_yaml(const nlohmann::json& json, std::size_t indent = 2)
    {
        YAML::Emitter yaml;
        yaml.SetIndent(indent);
        detail::to_yaml(json, yaml);
        return yaml.c_str();
    }

    inline nlohmann::json from_yaml(const std::string& value)
    {
        YAML::Node yaml = YAML::Load(value);
        return detail::from_yaml(yaml);
    }
}
