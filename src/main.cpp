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
    std::pair<std::string, std::string> parse_pair(const std::string& pair)
    {
        const auto equal_sign = pair.find_first_of("=:");
        std::string name = equal_sign != std::string::npos ? pair.substr(0, equal_sign) : pair;
        std::string value = equal_sign != std::string::npos ? pair.substr(equal_sign + 1) : "";
        return {std::move(name), std::move(value)};
    }

    std::size_t get_rest_index(std::size_t argc, const char* argv[])
    {
        const auto predicate = [](const char* arg) { return std::strcmp(arg, "--") == 0; };
        const char** end = std::find_if(argv, argv + argc, predicate);
        return end != nullptr ? end - argv : argc;
    }
}

int main(int argc, const char* argv[])
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

    for (std::size_t index = rest_index + 1; index < argc; ++index)
    {
        options.additional_args.emplace_back(argv[index]);
    }

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
