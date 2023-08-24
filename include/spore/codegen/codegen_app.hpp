#pragma once

#include <chrono>
#include <execution>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "glob/glob.h"
#include "process.hpp"

#include "spore/codegen/ast/conditions/ast_condition_all.hpp"
#include "spore/codegen/ast/conditions/ast_condition_any.hpp"
#include "spore/codegen/ast/conditions/ast_condition_attribute.hpp"
#include "spore/codegen/ast/conditions/ast_condition_factory.hpp"
#include "spore/codegen/ast/conditions/ast_condition_none.hpp"
#include "spore/codegen/ast/converters/ast_converter.hpp"
#include "spore/codegen/ast/converters/ast_converter_default.hpp"
#include "spore/codegen/ast/parsers/ast_parser.hpp"
#include "spore/codegen/ast/parsers/ast_parser_cppast.hpp"
#include "spore/codegen/codegen_cache.hpp"
#include "spore/codegen/codegen_config.hpp"
#include "spore/codegen/codegen_error.hpp"
#include "spore/codegen/codegen_helpers.hpp"
#include "spore/codegen/codegen_options.hpp"
#include "spore/codegen/codegen_thread_pool.hpp"
#include "spore/codegen/codegen_version.hpp"
#include "spore/codegen/renderers/codegen_renderer.hpp"
#include "spore/codegen/renderers/codegen_renderer_composite.hpp"
#include "spore/codegen/renderers/codegen_renderer_inja.hpp"

#ifndef SPORE_CODEGEN_MACRO
#define SPORE_CODEGEN_MACRO "SPORE_CODEGEN"
#endif

namespace spore::codegen
{
    namespace details
    {
        std::once_flag setup_once_flag;

        void setup_once()
        {
            ast_condition_factory::instance().register_condition<ast_condition_any>();
            ast_condition_factory::instance().register_condition<ast_condition_all>();
            ast_condition_factory::instance().register_condition<ast_condition_none>();
            ast_condition_factory::instance().register_condition<ast_condition_attribute>();
        }

        struct current_path_scope
        {
            std::filesystem::path old_path;

            inline explicit current_path_scope(const std::filesystem::path& new_path)
            {
                old_path = std::filesystem::current_path();
                std::filesystem::current_path(new_path);
            }

            inline ~current_path_scope()
            {
                std::filesystem::current_path(old_path);
            }
        };
    }

    struct codegen_app
    {
        codegen_config config;
        codegen_cache cache;
        codegen_options options;
        nlohmann::json json_user_data;
        std::shared_ptr<ast_parser> parser;
        std::shared_ptr<ast_converter> converter;
        std::shared_ptr<codegen_renderer> renderer;

        std::mutex mutex;
        std::vector<std::unique_ptr<TinyProcessLib::Process>> processes;

        inline explicit codegen_app(codegen_options in_options)
            : options(std::move(in_options))
        {
            std::call_once(details::setup_once_flag, &details::setup_once);

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
                    cache.reset();
                }
                else if (!cache.check_and_update(options.config))
                {
                    SPDLOG_INFO("ignoring old cache because of new config, file={} config={}", cache_file, options.config);
                    cache.reset();
                    cache.check_and_update(options.config);
                }
                else
                {
                    SPDLOG_DEBUG("using cache, file={}", cache_file);
                }
            }

            for (const std::pair<std::string, std::string>& pair : options.user_data)
            {
                json_user_data[pair.first] = pair.second;
            }

            cppast::libclang_compile_config cppast_config;
            cppast_config.set_flags(spore::codegen::parse_cpp_standard(options.cpp_standard));
            cppast_config.define_macro(SPORE_CODEGEN_MACRO, "1");
            cppast_config.fast_preprocessing(true);

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

            parser = std::make_shared<ast_parser_cppast>(std::move(cppast_config));
            converter = std::make_shared<ast_converter_default>();
            renderer = std::make_shared<codegen_renderer_composite>(
                std::make_shared<codegen_renderer_inja>(options.template_paths));
        }

        inline void run()
        {
            const auto action = [&](const codegen_config_stage& stage) {
                run_stage(stage);
            };

            std::for_each(config.stages.begin(), config.stages.end(), action);

            nlohmann::json cache_data = nlohmann::json(cache).dump(2);
            if (!spore::codegen::write_file(options.cache, cache_data))
            {
                SPDLOG_WARN("failed to write cache, file={}", options.cache);
            }

            std::lock_guard lock(mutex);

            const auto predicate = [](const std::unique_ptr<TinyProcessLib::Process>& process) {
                int exit_status = 0;
                return process->try_get_exit_status(exit_status);
            };

            while (!std::all_of(processes.begin(), processes.end(), predicate))
            {
                // no-op
            }

            processes.clear();
        }

        inline void run_stage(const codegen_config_stage& stage)
        {
            details::current_path_scope directory_scope(stage.directory);

            std::vector<std::filesystem::path> file_paths = glob::rglob(stage.files);
            std::vector<std::exception_ptr> exceptions;

            std::vector<ast_file> files;
            files.resize(file_paths.size());

//            const auto try_rethrow = [&]() {
//                std::lock_guard lock(mutex);
//                if (!exceptions.empty())
//                {
//                    std::rethrow_exception(*exceptions.begin());
//                }
//            };

            const auto generate = [&](const std::filesystem::path& file_path) {
                ast_file file;
                try
                {
                    const auto then = std::chrono::steady_clock::now();

                    if (!parser->parse_file(file_path.string(), file))
                    {
                        throw codegen_error(codegen_error_code::parsing, "failed to parse input, file={}", file_path.string());
                    }

                    for (const codegen_config_step& step : stage.steps)
                    {
                        run_step(step, file);
                    }

                    const auto now = std::chrono::steady_clock::now();
                    const std::chrono::duration<std::float_t> duration = now - then;
                    SPDLOG_DEBUG("generate file completed, file={} duration={}s", file.path, duration.count());
                }
                catch (...)
                {
                    std::lock_guard lock(mutex);
                    exceptions.emplace_back(std::current_exception());
                }

                return file;
            };

            codegen_thread_pool thread_pool(options.parallelism);
            const auto add_task = [&](const std::filesystem::path& file_path) mutable {
                thread_pool.add_task([&]() { generate(file_path); });
            };

            std::for_each(file_paths.begin(), file_paths.end(), add_task);
            thread_pool.end_execution();

            std::lock_guard lock(mutex);
            if (!exceptions.empty())
            {
                std::rethrow_exception(*exceptions.begin());
            }

//            const auto action = [&](const ast_file& file) {
//                try
//                {
//                    const auto then = std::chrono::steady_clock::now();
//
//                    for (const codegen_config_step& step : stage.steps)
//                    {
//                        run_step(stage, step, file);
//                    }
//
//                    const auto now = std::chrono::steady_clock::now();
//                    const std::chrono::duration<std::float_t> duration = now - then;
//                    SPDLOG_DEBUG("render file completed, file={} duration={}s", file.path, duration.count());
//                }
//                catch (...)
//                {
//                    std::lock_guard lock(mutex);
//                    exceptions.emplace_back(std::current_exception());
//                }
//            };
//
//            if (options.sequential)
//            {
//                std::for_each(std::execution::seq, files.begin(), files.end(), action);
//            }
//            else
//            {
//                std::for_each(std::execution::par, files.begin(), files.end(), action);
//            }
//
//            try_rethrow();
        }

        inline void run_step(const codegen_config_step& step, const ast_file& file)
        {
            std::filesystem::path file_path(file.path);
            std::vector<std::string> templates = step.templates;
            resolve_templates(templates);

            bool templates_up_to_date = true;
            for (const std::string& template_ : templates)
            {
                const bool template_up_to_date = cache.check_and_update(template_);
                templates_up_to_date = templates_up_to_date && template_up_to_date;
            }

            std::shared_ptr<ast_condition> condition;
            if (step.condition.has_value())
            {
                condition = ast_condition_factory::instance().make_condition(step.condition.value());
            }

            const bool input_up_to_date = cache.check_and_update(file.path);
            if (input_up_to_date && templates_up_to_date)
            {
                SPDLOG_DEBUG("skipping input because it has not changed, file={}", file.path);
                return;
            }

            if (condition && !condition->matches_condition(file))
            {
                SPDLOG_DEBUG("skipping input because it does not match condition, file={}", file.path);
                return;
            }

            nlohmann::json json_data;
            if (!converter->convert_file(file, json_data))
            {
                throw codegen_error(codegen_error_code::rendering, "failed to convert input data to json, file={}", file.path);
            }

            if (options.dump_ast)
            {
                std::filesystem::path output_filename = fmt::format("{}.json", file_path.stem().string());
                std::filesystem::path output_directory = std::filesystem::path(*options.dump_ast) / file_path.parent_path();
                std::string output = (output_directory / output_filename).string();

                if (!spore::codegen::write_file(output, json_data.dump(2)))
                {
                    SPDLOG_WARN("failed to write dump file, file={}", output);
                }
            }

            std::vector<std::string> outputs;
            outputs.reserve(templates.size());

            const auto transform = [&](const std::string& template_) {
                std::filesystem::path output_ext = std::filesystem::path(template_).stem();
                std::filesystem::path output_filename = fmt::format("{}.{}", file_path.stem().string(), output_ext.string());
                std::filesystem::path output_directory = std::filesystem::path(options.output) / step.directory / file_path.parent_path();
                std::string output = std::filesystem::absolute(output_directory / output_filename).string();
                return output;
            };

            std::transform(templates.begin(), templates.end(), std::back_inserter(outputs), transform);

            nlohmann::json json_context;
            json_context["outputs"] = outputs;
            json_context["user_data"] = json_user_data;
            json_data["$"] = json_context;

            std::size_t index_template = 0;
            for (const std::string& template_ : templates)
            {
                std::string result;
                if (!renderer->render_file(template_, json_data, result))
                {
                    throw codegen_error(codegen_error_code::rendering, "failed to render input, file={} template={}", file.path, template_);
                }

                const std::string& output = outputs[index_template];
                SPDLOG_DEBUG("generating output, file={}", output);

                if (!spore::codegen::write_file(output, result))
                {
                    throw codegen_error(codegen_error_code::io, "failed to write output, file={}", output);
                }

                if (!options.reformat.empty())
                {
                    std::lock_guard lock(mutex);
                    std::string command = fmt::format("{} {}", options.reformat, output);
                    processes.emplace_back(std::make_unique<TinyProcessLib::Process>(command));
                }

                ++index_template;
            }
        }

        inline void resolve_templates(std::vector<std::string>& templates)
        {
            SPDLOG_DEBUG("searching for templates");
            for (std::string& template_ : templates)
            {
                SPDLOG_DEBUG("  search template, file={}", template_);
                if (std::filesystem::exists(template_))
                {
                    template_ = std::filesystem::absolute(template_).string();
                    SPDLOG_DEBUG("  found template, file={}", template_);
                    continue;
                }

                const auto action_predicate = [&](const std::string& template_path) {
                    std::filesystem::path possible_template = std::filesystem::path(template_path) / template_;

                    SPDLOG_DEBUG("  search template, file={}", possible_template.string());
                    if (std::filesystem::exists(possible_template))
                    {
                        template_ = std::filesystem::absolute(possible_template).string();
                        SPDLOG_DEBUG("  found template, file={}", possible_template.string());
                        return true;
                    }
                };

                if (!std::any_of(options.template_paths.begin(), options.template_paths.end(), action_predicate))
                {
                    throw codegen_error(codegen_error_code::configuring, "could not find template file, file={}", template_);
                }
            }
        }
    };
#if 0
    void run_codegen(const codegen_options& options)
    {
        std::call_once(details::setup_once_flag, &details::setup_once);

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
                cache.reset();
            }
            else if (!cache.check_and_update(options.config))
            {
                SPDLOG_INFO("ignoring old cache because of new config, file={} config={}", cache_file, options.config);
                cache.reset();
                cache.check_and_update(options.config);
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
        std::shared_ptr<codegen_renderer> renderer = std::make_shared<codegen_renderer_composite>(std::make_shared<codegen_renderer_inja>(options.template_paths));

        nlohmann::json user_data_json;
        for (const std::pair<std::string, std::string>& pair : options.user_data)
        {
            user_data_json[pair.first] = pair.second;
        }

        for (const codegen_config_step& step : config.steps)
        {
            bool templates_up_to_date = true;
            for (const std::string& template_ : templates)
            {
                const bool template_up_to_date = cache.check_and_update(template_);
                templates_up_to_date = templates_up_to_date && template_up_to_date;
            }

            std::mutex mutex;
            std::vector<std::exception_ptr> exceptions;

            std::shared_ptr<ast_condition> condition;
            if (step.condition.has_value())
            {
                condition = ast_condition_factory::instance().make_condition(step.condition.value());
            }

            SPDLOG_DEBUG("processing step, name={}", step.name);

            const auto action = [&](const std::filesystem::path& input) {
                SPDLOG_DEBUG("processing input, file={}", input.string());
                try
                {
                    const auto then = std::chrono::steady_clock::now();
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

                    if (condition && !condition->matches_condition(file))
                    {
                        SPDLOG_DEBUG("skipping input because it does not match condition, file={}", input.string());
                        return;
                    }

                    nlohmann::json json_data;
                    if (!converter->convert_file(file, json_data))
                    {
                        throw codegen_error(codegen_error_code::rendering, "failed to convert input data to json, file={}", input.string());
                    }

                    if (options.dump_ast)
                    {
                        std::filesystem::path output_filename = fmt::format("{}.json", input.stem().string());
                        std::filesystem::path output_directory = std::filesystem::path(*options.dump_ast) / input.parent_path();
                        std::string output = (output_directory / output_filename).string();

                        if (!spore::codegen::write_file(output, json_data.dump(2)))
                        {
                            SPDLOG_WARN("failed to write dump file, file={}", output);
                        }
                    }

                    std::vector<std::string> outputs;
                    outputs.reserve(templates.size());

                    std::transform(
                        templates.begin(), templates.end(),
                        std::back_inserter(outputs),
                        [&](const std::string& template_) {
                            std::filesystem::path output_ext = std::filesystem::path(template_).stem();
                            std::filesystem::path output_filename = fmt::format("{}.{}", input.stem().string(), output_ext.string());
                            std::filesystem::path output_directory = std::filesystem::path(options.output) / input.parent_path();
                            std::string output = std::filesystem::absolute(output_directory / output_filename).string();
                            return output;
                        });

                    json_data["outputs"] = outputs;
                    json_data["user_data"] = user_data_json;

                    int index_template = 0;
                    for (const std::string& template_ : templates)
                    {
                        std::string result;
                        if (!renderer->render_file(template_, json_data, result))
                        {
                            throw codegen_error(codegen_error_code::rendering, "failed to render input, file={} template={}", input.string(), template_);
                        }

                        const std::string& output = outputs[index_template];
                        SPDLOG_DEBUG("generating output, file={}", output);

                        if (!spore::codegen::write_file(output, result))
                        {
                            throw codegen_error(codegen_error_code::io, "failed to write output, file={}", output);
                        }

                        if (!options.reformat.empty())
                        {
                            std::string command = fmt::format("{} {}", options.reformat, output);

                            TinyProcessLib::Process process {command};

                            if (process.get_exit_status() != 0)
                            {
                                SPDLOG_WARN("failed to reformat output, file={}", output);
                            }
                        }

                        ++index_template;
                    }

                    const auto now = std::chrono::steady_clock::now();
                    const std::chrono::duration<std::float_t> duration = now - then;
                    SPDLOG_INFO("codegen for input file completed, file={} time={:.2f}s", input.string(), duration.count());
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
#endif
}
