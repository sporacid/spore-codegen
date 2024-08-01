#pragma once

#include <iomanip>
#include <sstream>
#include <string>
#include <tuple>

#include "process.hpp"

namespace spore::codegen::processes
{
    inline std::tuple<std::int32_t, std::string, std::string> run(const std::string& command, const std::string& input = std::string {})
    {
        const bool has_stdin = not input.empty();
        const auto reader_factory = [](std::stringstream& stream) {
            return [&](const char* bytes, std::size_t n) {
                stream.write(bytes, static_cast<std::streamsize>(n));
            };
        };

        std::stringstream _stdout;
        std::stringstream _stderr;

        TinyProcessLib::Process process {
            command,
            std::string(),
            reader_factory(_stdout),
            reader_factory(_stderr),
            has_stdin,
        };

        if (has_stdin)
        {
            process.write(input);
            process.close_stdin();
        }

        std::int32_t result = process.get_exit_status();
        return std::tuple {result, _stdout.str(), _stderr.str()};
    }
}
