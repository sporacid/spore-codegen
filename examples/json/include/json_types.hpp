#pragma once

#include <string>

namespace spore::codegen::examples::json
{
    struct [[json]] generic_pair
    {
        [[json(name = "Key")]] std::string key;
        [[json(name = "Value")]] std::string value;
        [[json(ignore)]] std::uint32_t version = 0;
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