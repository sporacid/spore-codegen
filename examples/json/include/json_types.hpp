#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#define ATTRIBUTE(...) [[clang::annotate(#__VA_ARGS__)]]

namespace spore::codegen::examples::json
{
    struct ATTRIBUTE(json) generic_pair
    {
        ATTRIBUTE(json = "Key")
        std::string key;

        ATTRIBUTE(json = "Value")
        std::string value;

        ATTRIBUTE(json = false)
        std::uint32_t version = 0;
    };

    template <typename value_t>
    struct ATTRIBUTE(json) template_pair
    {
        ATTRIBUTE(json = "Key")
        std::string key;

        ATTRIBUTE(json = "Value")
        value_t value;

        ATTRIBUTE(json = false)
        std::uint32_t version = 0;
    };
}

#ifndef SPORE_CODEGEN
#include "json_types.json.hpp"
#endif