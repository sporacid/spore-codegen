#include <string>
#include <vector>

#include "argparse/argparse.hpp"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

#include "spore/codegen/codegen_app.hpp"
#include "spore/codegen/codegen_options.hpp"
#include "spore/codegen/codegen_version.hpp"

namespace spore::codegen::detail
{
    std::pair<std::string, std::string> parse_pair(std::string_view pair)
    {
        const auto equal_sign = pair.find_first_of("=:");
        const std::string_view name = equal_sign != std::string_view::npos ? pair.substr(0, equal_sign) : pair;
        const std::string_view value = equal_sign != std::string_view::npos ? pair.substr(equal_sign + 1) : "";
        return {std::string(name), std::string(value)};
    }

    // std::pair<std::string, std::string> parse_additional_arg(std::string_view arg)
    // {
    //     // @sporacid e.g. -cpp:I lib/include -cpp:std=c++20
    //     const auto colon_index = arg.find_first_of(':');
    //     std::string name = equal_sign != std::string::npos ? pair.substr(0, equal_sign) : pair;
    //     std::string value = equal_sign != std::string::npos ? pair.substr(equal_sign + 1) : "";
    //     return {std::move(name), std::move(value)};
    // }

    std::size_t get_rest_index(std::size_t argc, const char* argv[])
    {
        const auto predicate = [](const char* arg) { return std::strcmp(arg, "--") == 0; };
        const char** end = std::find_if(argv, argv + argc, predicate);
        return end != nullptr ? end - argv : argc;
    }

    void parse_additional_args(std::span<const char*> argv, std::map<std::string_view, std::vector<std::string>>& arg_map)
    {
        std::string_view prev_type;
        for (std::string_view arg : argv)
        {
            if (arg.starts_with('-'))
            {
                std::size_t colon_index = arg.find_first_of(':');
                if (colon_index == std::string_view::npos)
                {
                    SPDLOG_WARN("invalid additional argument will be ignored, arg={}", arg);
                    continue;
                }

                std::string_view type = arg.substr(0, colon_index);

                std::size_t dash_index = type.find_last_of('-');
                assert(dash_index != std::string_view::npos);

                std::string_view value_begin = type.substr(0, dash_index + 1);
                std::string_view value_end = arg.substr(colon_index + 1);

                type = type.substr(dash_index + 1);

                std::vector<std::string>& additional_args = arg_map[type];
                additional_args.emplace_back(fmt::format("{}{}", value_begin, value_end));

                prev_type = type;
            }
            else
            {
                std::vector<std::string>& additional_args = arg_map[prev_type];
                additional_args.emplace_back(arg);
            }
        }
    }
}

int main(const int argc, const char* argv[])
{
    using namespace spore::codegen;

    spdlog::set_pattern("%l: %v");

    argparse::ArgumentParser arg_parser {
        SPORE_CODEGEN_NAME,
        SPORE_CODEGEN_VERSION,
    };

    arg_parser
        .add_argument("-o", "--output")
        .help("output directory to generate to")
        .default_value(std::string {".codegen"});

    arg_parser
        .add_argument("-c", "--config")
        .help("codegen configuration file to use")
        .default_value(std::string {"codegen.yml"});

    arg_parser
        .add_argument("-C", "--cache")
        .help("codegen cache file to use")
        .default_value(std::string {"cache.yml"});

    arg_parser
        .add_argument("-t", "--templates")
        .help("paths to search for templates")
        .default_value(std::vector<std::string> {})
        .append();

    arg_parser
        .add_argument("-D", "--user-data")
        .help("additional user data to be passed to the rendering stage, can be accessed through the `user_data` JSON property")
        .default_value(std::vector<std::pair<std::string, std::string>> {})
        .append()
        .action(&detail::parse_pair);

    arg_parser
        .add_argument("-r", "--reformat")
        .help("command to reformat output files, or false to disable reformatting")
        .default_value(std::string {"clang-format -i"})
        .action([](const std::string& reformat) {
            return reformat != "false" ? reformat : "";
        });

    arg_parser
        .add_argument("-f", "--force")
        .help("force generation for all input files even if files haven't changed")
        .default_value(false)
        .implicit_value(true);

    arg_parser
        .add_argument("-d", "--debug")
        .help("enable debug output")
        .default_value(false)
        .implicit_value(true);

    std::size_t rest_index = detail::get_rest_index(argc, argv);

    try
    {
        arg_parser.parse_args(static_cast<int>(rest_index), argv);
    }
    catch (const std::exception& e)
    {
        std::cout << e.what() << std::endl;
        std::cout << arg_parser;
        return static_cast<int>(codegen_error_code::invalid);
    }

    codegen_options options {
        .output = arg_parser.get<std::string>("--output"),
        .config = arg_parser.get<std::string>("--config"),
        .cache = arg_parser.get<std::string>("--cache"),
        .reformat = arg_parser.get<std::string>("--reformat"),
        .templates = arg_parser.get<std::vector<std::string>>("--templates"),
        .user_data = arg_parser.get<std::vector<std::pair<std::string, std::string>>>("--user-data"),
        .force = arg_parser.get<bool>("--force"),
        .debug = arg_parser.get<bool>("--debug"),
    };

    std::span additional_args {argv + rest_index + 1, argv + argc};
    detail::parse_additional_args(additional_args, options.additional_args);

    if (options.reformat == "false")
    {
        options.reformat = std::string();
    }

    if (options.debug)
    {
        spdlog::set_level(spdlog::level::debug);
    }

    try
    {
        codegen_app(options).run();
    }
    catch (const codegen_error& e)
    {
        SPDLOG_ERROR(e.what());
        return static_cast<int>(e.error_code);
    }
    catch (const std::exception& e)
    {
        SPDLOG_ERROR("failed to run codegen: {}", e.what());
        return static_cast<int>(codegen_error_code::unknown);
    }
    catch (...)
    {
        SPDLOG_ERROR("failed to run codegen");
        return static_cast<int>(codegen_error_code::unknown);
    }

    return static_cast<int>(codegen_error_code::none);
}
