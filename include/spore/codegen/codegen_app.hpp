#pragma once

#include <execution>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "glob/glob.h"

#include "spore/codegen/ast/converters/ast_converter.hpp"
#include "spore/codegen/ast/converters/ast_converter_default.hpp"
#include "spore/codegen/ast/parsers/ast_parser.hpp"
#include "spore/codegen/ast/parsers/ast_parser_cppast.hpp"
#include "spore/codegen/codegen_cache.hpp"
#include "spore/codegen/codegen_config.hpp"
#include "spore/codegen/codegen_error.hpp"
#include "spore/codegen/codegen_helpers.hpp"
#include "spore/codegen/codegen_options.hpp"
#include "spore/codegen/codegen_version.hpp"
#include "spore/codegen/renderers/codegen_renderer.hpp"
#include "spore/codegen/renderers/codegen_renderer_composite.hpp"
#include "spore/codegen/renderers/codegen_renderer_inja.hpp"

namespace spore::codegen
{
    void run_codegen(const codegen_options& options)
    {
        codegen_config config;
        codegen_cache cache;

        std::string config_data;
        if (!spore::codegen::read_file(options.config, config_data))
        {
            throw codegen_error(codegen_error_code::io, "unable to read config, file={}", options.config);
        }

        nlohmann::json::parse(config_data).get_to(config);

        std::string cache_data;
        std::string cache_file = (std::filesystem::path(options.output) / options.cache).string();

        if (!options.force && spore::codegen::read_file(cache_file, cache_data))
        {
            nlohmann::json::parse(cache_data).get_to(cache);

            if (cache.version != SPORE_CODEGEN_VERSION)
            {
                SPDLOG_INFO("ignoring old cache, file={} version={}", cache_file, cache.version);
            }
            else
            {
                SPDLOG_DEBUG("using cache, file={}", cache_file);
            }
        }

        cppast::libclang_compile_config cppast_config;
        cppast_config.set_flags(spore::codegen::parse_cpp_standard(options.cpp_standard));
        cppast_config.define_macro("SPORE_CODEGEN", "1");

        for (const std::string& include : options.includes)
        {
            cppast_config.add_include_dir(include);
        }

        // TODO re-enable once it has been figured out why it fails with -fcxx_std_17
        // for (const std::string& feature : options.features)
        // {
        //     cppast_config.enable_feature(feature);
        // }

        for (const std::pair<std::string, std::string>& definition : options.definitions)
        {
            cppast_config.define_macro(definition.first, definition.second);
        }

        std::shared_ptr<ast_parser> parser = std::make_shared<ast_parser_cppast>(std::move(cppast_config));
        std::shared_ptr<ast_converter> converter = std::make_shared<ast_converter_default>();
        std::shared_ptr<codegen_renderer> renderer = std::make_shared<codegen_renderer_composite>(std::make_shared<codegen_renderer_inja>());

        nlohmann::json user_data_json;
        for (const std::pair<std::string,std::string>& pair : options.user_data)
        {
            user_data_json[pair.first] = pair.second;
        }

        for (const codegen_config_step& step : config.steps)
        {
            std::vector<std::filesystem::path> inputs = glob::rglob(step.input);

            const bool templates_up_to_date = std::all_of(
                step.templates.begin(), step.templates.end(),
                [&](const std::string& template_) {
                    return cache.check_and_update(template_);
                });

            std::mutex mutex;
            std::vector<std::exception_ptr> exceptions;

            const auto action = [&](const std::filesystem::path& input) {
                try
                {
                    const bool input_up_to_date = cache.check_and_update(input.string());
                    if (input_up_to_date && templates_up_to_date)
                    {
                        SPDLOG_DEBUG("skipping input because it has not changed, file={}", input.string());
                        return;
                    }

                    ast_file file;
                    if (!parser->parse_file(input.string(), file))
                    {
                        throw codegen_error(codegen_error_code::parsing, "failed to parse input, file={}", input.string());
                    }

                    nlohmann::json json_data;
                    if (!converter->convert_file(file, json_data))
                    {
                        throw codegen_error(codegen_error_code::rendering, "failed to convert input data to json, file={}", input.string());
                    }

                    json_data["user_data"] = user_data_json;

                    for (const std::string& template_ : step.templates)
                    {
                        std::string result;
                        if (!renderer->render_file(template_, json_data, result))
                        {
                            throw codegen_error(codegen_error_code::rendering, "failed to render input, file={} template={}", input.string(), template_);
                        }

                        std::filesystem::path output_ext = std::filesystem::path(template_).stem();
                        std::filesystem::path output_filename = fmt::format("{}.{}", input.stem().string(), output_ext.string());
                        std::filesystem::path output_directory = std::filesystem::path(options.output) / input.parent_path();
                        std::string output = (output_directory / output_filename).string();

                        if (!spore::codegen::write_file(output, result))
                        {
                            throw codegen_error(codegen_error_code::io, "failed to write output, file={}", output);
                        }

                        if (options.reformat && !spore::codegen::format_file(output))
                        {
                            SPDLOG_WARN("failed to reformat output, file={}", output);
                        }
                    }
                }
                catch (...)
                {
                    std::lock_guard lock(mutex);
                    exceptions.emplace_back(std::current_exception());
                }
            };

            if (options.sequential)
            {
                std::for_each(std::execution::seq, inputs.begin(), inputs.end(), action);
            }
            else
            {
                std::for_each(std::execution::par, inputs.begin(), inputs.end(), action);
            }

            std::lock_guard lock(mutex);
            if (!exceptions.empty())
            {
                for (const std::exception_ptr& exception : exceptions)
                {
                    std::rethrow_exception(exception);
                }
            }
        }

        cache_data = nlohmann::json(cache).dump(2);

        if (!spore::codegen::write_file(cache_file, cache_data))
        {
            SPDLOG_WARN("failed to write cache, file={}", cache_file);
        }
    }
}