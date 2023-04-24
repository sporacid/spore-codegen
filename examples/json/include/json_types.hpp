#pragma once

#include <string>
#include <optional>
namespace spore::codegen::examples::json
{
    struct [[json]] generic_pair
    {
        [[json(name = "Key")]] std::string key;
        [[json(name = "Value")]] std::string value;
        [[json(ignore)]] std::uint32_t version = 0;
        std::optional<int> optional_field;
    };

    template <typename value_t>
    struct [[json]] template_pair
    {
        [[json(name = "Key")]] std::string key;
        [[json(name = "Value")]] value_t value;
        [[json(ignore)]] std::uint32_t version = 0;
    };
}

#ifndef SPORE_CODEGEN
#include "json_types.json.hpp"
#endif
