#pragma once

#include <atomic>
#include <cstddef>

namespace spore::codegen
{
    template <typename>
    std::size_t make_unique_id()
    {
        static std::atomic<std::size_t> seed;
        return ++seed;
    }
}