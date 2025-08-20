#include <format>
#include <string>
#include <vector>

#include "argparse/argparse.hpp"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

#include "spore/codegen/codegen_app.hpp"
#include "spore/codegen/codegen_impl.hpp"
#include "spore/codegen/codegen_options.hpp"
#include "spore/codegen/codegen_version.hpp"
#include "spore/codegen/conditions/codegen_condition_factory.hpp"
#include "spore/codegen/conditions/codegen_condition_logical.hpp"
#include "spore/codegen/formatters/codegen_formatter_composite.hpp"
#include "spore/codegen/formatters/codegen_formatter_json.hpp"
#include "spore/codegen/formatters/codegen_formatter_yaml.hpp"
#include "spore/codegen/renderers/codegen_renderer_composite.hpp"
#include "spore/codegen/renderers/codegen_renderer_inja.hpp"

#ifdef SPORE_WITH_CPP
#    include "spore/codegen/conditions/codegen_condition_attribute.hpp"
#    include "spore/codegen/formatters/codegen_formatter_cpp.hpp"
#    include "spore/codegen/parsers/cpp/ast/cpp_file.hpp"
#    include "spore/codegen/parsers/cpp/codegen_converter_cpp.hpp"
#    include "spore/codegen/parsers/cpp/codegen_parser_cpp.hpp"
#endif

#ifdef SPORE_WITH_SPIRV
#    include "spore/codegen/parsers/spirv/ast/spirv_module.hpp"
#    include "spore/codegen/parsers/spirv/codegen_converter_spirv.hpp"
#    include "spore/codegen/parsers/spirv/codegen_parser_spirv.hpp"
#endif

namespace spore::codegen
{
    namespace detail
    {
        namespace metavars
        {
            constexpr auto file = "FILE";
            constexpr auto directory = "DIR";
            constexpr auto pair = "PAIR";
        }

        std::pair<std::string, std::string> parse_pair(std::string_view pair)
        {
            const auto equal_sign = pair.find_first_of("=:");
            const std::string_view name = equal_sign != std::string_view::npos ? pair.substr(0, equal_sign) : pair;
            const std::string_view value = equal_sign != std::string_view::npos ? pair.substr(equal_sign + 1) : "";
            return {std::string(name), std::string(value)};
        }

        template <typename ast_t>
        void register_default_conditions(codegen_condition_factory<ast_t>& factory)
        {
            factory.template register_condition<codegen_condition_any<ast_t>>();
            factory.template register_condition<codegen_condition_all<ast_t>>();
            factory.template register_condition<codegen_condition_none<ast_t>>();
        }
    }

#ifdef SPORE_WITH_CPP
    template <>
    struct codegen_impl_traits<cpp_file>
    {
        static constexpr std::string_view name()
        {
            return "cpp";
        }

        static void register_conditions(codegen_condition_factory<cpp_file>& factory)
        {
            detail::register_default_conditions(factory);
            factory.register_condition<codegen_condition_attribute>();
        }
    };
#endif

#ifdef SPORE_WITH_SPIRV
    template <>
    struct codegen_impl_traits<spirv_module>
    {
        static constexpr std::string_view name()
        {
            return "spirv";
        }

        static void register_conditions(codegen_condition_factory<spirv_module>& factory)
        {
            detail::register_default_conditions(factory);
        }
    };
#endif
}

int main(const int argc, const char* argv[])
{
    using namespace spore::codegen;

    spdlog::set_pattern("%l: %v");

    argparse::ArgumentParser arg_parser {
        SPORE_CODEGEN_NAME,
        SPORE_CODEGEN_VERSION,
    };

    constexpr auto description =
        "Tool to generate files from source code and byte code";

    constexpr auto epilog =
        "Additional implementation specific arguments can be supplied via the syntax --<impl>:<arg>, e.g.\n"
#ifdef SPORE_WITH_CPP
        "  --cpp:-std=c++11\n"
        "  --cpp:-Iinclude/dir"
#endif
        ;

    arg_parser.add_description(description);
    arg_parser.add_epilog(epilog);

    arg_parser
        .add_argument("-c", "--config")
        .help("Codegen configuration file to use")
        .metavar(detail::metavars::file)
        .default_value(std::string {"codegen.yml"});

    arg_parser
        .add_argument("-C", "--cache")
        .help("Codegen cache file to use")
        .metavar(detail::metavars::file)
        .default_value(std::string {".codegen.yml"});

    arg_parser
        .add_argument("-t", "--templates")
        .help("Directories to search for templates")
        .default_value(std::vector<std::string> {})
        .metavar(detail::metavars::directory)
        .append();

    arg_parser
        .add_argument("-D", "--user-data")
        .help("Additional data to be passed to the rendering stage, can be accessed through the \"$.user_data\" JSON property")
        .default_value(std::vector<std::pair<std::string, std::string>> {})
        .metavar(detail::metavars::pair)
        .append()
        .action(&detail::parse_pair);

    arg_parser
        .add_argument("-r", "--reformat")
        .help("Whether to reformat output files or not")
        .default_value(false)
        .implicit_value(true);

    arg_parser
        .add_argument("-f", "--force")
        .help("Bypass cache checks and force generation for all input files")
        .default_value(false)
        .implicit_value(true);

    arg_parser
        .add_argument("-d", "--debug")
        .help("Enable debug output")
        .default_value(false)
        .implicit_value(true);

    std::vector<std::string> additional_args;
    try
    {
        additional_args = arg_parser.parse_known_args(argc, argv);
    }
    catch (const std::exception& e)
    {
        std::cout << e.what() << std::endl;
        std::cout << arg_parser;
        return static_cast<int>(codegen_error_code::invalid);
    }

    codegen_options options {
        .config = arg_parser.get<std::string>("--config"),
        .cache = arg_parser.get<std::string>("--cache"),
        .templates = arg_parser.get<std::vector<std::string>>("--templates"),
        .user_data = arg_parser.get<std::vector<std::pair<std::string, std::string>>>("--user-data"),
        .reformat = arg_parser.get<bool>("--reformat"),
        .force = arg_parser.get<bool>("--force"),
        .debug = arg_parser.get<bool>("--debug"),
    };

    if (options.debug)
    {
        spdlog::set_level(spdlog::level::debug);
    }

    const auto parse_impl_args = [&]<typename impl_t> {
        const std::string prefix = std::format("--{}:", impl_t::name());

        std::vector<std::string> impl_args;
        for (const std::string& additional_arg : additional_args)
        {
            if (additional_arg.starts_with(prefix))
            {
                std::string impl_arg = additional_arg.substr(prefix.size());
                impl_args.emplace_back(std::move(impl_arg));
            }
        }

        return impl_args;
    };

#ifdef SPORE_WITH_CPP
    const auto cpp_args = parse_impl_args.operator()<codegen_impl<cpp_file>>();
    codegen_impl<cpp_file> impl_cpp {
        codegen_parser_cpp {cpp_args},
        codegen_converter_cpp {},
    };
#endif

#ifdef SPORE_WITH_SPIRV
    const auto spirv_args = parse_impl_args.operator()<codegen_impl<spirv_module>>();
    codegen_impl<spirv_module> impl_spirv {
        codegen_parser_spirv {spirv_args},
        codegen_converter_spirv {},
    };
#endif

    codegen_renderer_composite renderer {
        std::make_unique<codegen_renderer_inja>(options.templates),
    };

    codegen_formatter_composite formatter {
        std::make_unique<codegen_formatter_json>(),
        std::make_unique<codegen_formatter_yaml>(),
#ifdef SPORE_WITH_CPP
        std::make_unique<codegen_formatter_cpp>(),
#endif
    };

    try
    {
        codegen_app app {
            std::move(options),
            std::move(renderer),
            std::move(formatter),
#ifdef SPORE_WITH_CPP
            std::move(impl_cpp),
#endif
#ifdef SPORE_WITH_SPIRV
            std::move(impl_spirv),
#endif
        };

        app.run();
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
