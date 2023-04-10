#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "argparse/argparse.hpp"
#include "spdlog/async.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

#include "spore/codegen/codegen_app.hpp"
#include "spore/codegen/codegen_options.hpp"
#include "spore/codegen/codegen_version.hpp"

int main(int argc, const char* argv[])
{
    std::shared_ptr<spdlog::logger> logger = spdlog::stdout_color_st<spdlog::async_factory>("default");
    spdlog::set_default_logger(logger);
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
        .default_value(std::string {"codegen.json"});

    arg_parser
        .add_argument("-C", "--cache")
        .help("codegen cache file to use")
        .default_value(std::string {"cache.json"});

    arg_parser
        .add_argument("-I", "--includes")
        .help("include paths to add to clang compilation")
        .default_value(std::vector<std::string> {})
        .append();

    arg_parser
        .add_argument("-f", "--features")
        .help("feature to enable for clang compilation")
        .default_value(std::vector<std::string> {})
        .append();

    arg_parser
        .add_argument("-D", "--definitions")
        .help("compiler definitions to add to clang compilation")
        .default_value(std::vector<std::pair<std::string, std::string>> {})
        .append()
        .action([](const std::string& definition) {
            const auto equal_sign = definition.find_first_of("=:");
            std::string name = equal_sign != std::string::npos ? definition.substr(0, equal_sign) : definition;
            std::string value = equal_sign != std::string::npos ? definition.substr(equal_sign + 1) : "";
            return std::pair(std::move(name), std::move(value));
        });

    arg_parser
        .add_argument("-s", "--cpp-standard")
        .help("c++ standard for clang compilation")
        .default_value(std::string {"c++14"})
        .action([](const std::string& cpp_standard) {
            const bool starts_with_cpp = cpp_standard.find("c++") == 0;
            return starts_with_cpp ? cpp_standard : "c++" + cpp_standard;
        });

    arg_parser
        .add_argument("-F", "--force")
        .help("force generation for all input files even if files haven't changed")
        .default_value(false)
        .implicit_value(true);

    arg_parser
        .add_argument("-r", "--reformat")
        .help("whether to reformat generated files with clang-format")
        .default_value(false)
        .implicit_value(true);

    arg_parser
        .add_argument("-g", "--debug")
        .help("sets output verbosity to debug")
        .default_value(false)
        .implicit_value(true);

    arg_parser
        .add_argument("-S", "--sequential")
        .help("process input files sequentially instead of in parallel")
        .default_value(false)
        .implicit_value(true);

    try
    {
        arg_parser.parse_args(argc, argv);
    }
    catch (const std::exception& e)
    {
        std::cout << e.what() << std::endl;
        std::cout << arg_parser;
        return static_cast<int>(spore::codegen::codegen_error_code::invalid);
    }

    if (arg_parser.get<bool>("--debug"))
    {
        spdlog::set_level(spdlog::level::debug);
    }

    spore::codegen::codegen_options options;
    options.output = arg_parser.get<std::string>("--output");
    options.config = arg_parser.get<std::string>("--config");
    options.cache = arg_parser.get<std::string>("--cache");
    options.cpp_standard = arg_parser.get<std::string>("--cpp-standard");
    options.includes = arg_parser.get<std::vector<std::string>>("--includes");
    options.features = arg_parser.get<std::vector<std::string>>("--features");
    options.definitions = arg_parser.get<std::vector<std::pair<std::string, std::string>>>("--definitions");
    options.force = arg_parser.get<bool>("--force");
    options.reformat = arg_parser.get<bool>("--reformat");
    options.sequential = arg_parser.get<bool>("--sequential");

    try
    {
        spore::codegen::run_codegen(options);
    }
    catch (const spore::codegen::codegen_error& e)
    {
        SPDLOG_ERROR(e.what());
        return static_cast<int>(e.error_code);
    }
    catch (const std::exception& e)
    {
        SPDLOG_ERROR("failed to run codegen: {}", e.what());
        return static_cast<int>(spore::codegen::codegen_error_code::unknown);
    }
    catch (...)
    {
        SPDLOG_ERROR("failed to run codegen");
        return static_cast<int>(spore::codegen::codegen_error_code::unknown);
    }

    return static_cast<int>(spore::codegen::codegen_error_code::none);
}
