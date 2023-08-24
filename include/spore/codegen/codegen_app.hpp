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
        inline std::once_flag setup_once_flag;

        inline void setup_once()
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

            current_path_scope(const current_path_scope&) = delete;
            current_path_scope(current_path_scope&&) = delete;

            current_path_scope& operator=(const current_path_scope&) = delete;
            current_path_scope& operator=(current_path_scope&&) = delete;
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

        struct resolved_template
        {
            std::string path;
            bool up_to_date = false;
        };

        inline explicit codegen_app(codegen_options in_options)
            : options(std::move(in_options))
        {
            std::call_once(details::setup_once_flag, &details::setup_once);

            std::string config_data;
            if (!spore::codegen::read_file(options.config, config_data))
            {
                throw codegen_error(codegen_error_code::io, "unable to read config, file={}", options.config);
            }

            std::filesystem::path output_path(options.output);
            if (!std::filesystem::path(options.output).is_absolute())
            {
                output_path = std::filesystem::absolute(output_path);
                options.output = output_path.string();
            }

            std::filesystem::path cache_path(options.cache);
            if (!std::filesystem::path(options.cache).is_absolute())
            {
                cache_path = std::filesystem::absolute(output_path / cache_path);
                options.cache = cache_path.string();
            }

            nlohmann::json::parse(config_data).get_to(config);

            std::string cache_data;
            if (!options.force && spore::codegen::read_file(options.cache, cache_data))
            {
                nlohmann::json::parse(cache_data).get_to(cache);

                if (cache.version != SPORE_CODEGEN_VERSION)
                {
                    SPDLOG_INFO("ignoring old cache, file={} version={}", options.cache, cache.version);
                    cache.reset();
                }
                else if (!cache.check_and_update(options.config))
                {
                    SPDLOG_INFO("ignoring old cache because of new config, file={} config={}", options.cache, options.config);
                    cache.reset();
                    cache.check_and_update(options.config);
                }
                else
                {
                    SPDLOG_DEBUG("using cache, file={}", options.cache);
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
            const auto then = std::chrono::steady_clock::now();
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
                SPDLOG_DEBUG("waiting on pending processes");
            }

            processes.clear();

            const auto now = std::chrono::steady_clock::now();
            const std::chrono::duration<std::float_t> duration = now - then;
            SPDLOG_INFO("codegen completed, duration={}s", duration.count());
        }

        inline void run_stage(const codegen_config_stage& stage)
        {
            std::filesystem::path stage_directory = std::filesystem::current_path() / stage.directory;
            if (!std::filesystem::exists(stage_directory))
            {
                SPDLOG_DEBUG("skipping stage because directory does not exist, directory={}", stage_directory.string());
                return;
            }

            std::vector<std::vector<resolved_template>> stage_templates;
            stage_templates.reserve(stage.steps.size());

            for (const codegen_config_step& step : stage.steps)
            {
                stage_templates.emplace_back(resolve_templates(step.templates));
            }

            details::current_path_scope directory_scope(stage_directory);

            std::vector<std::filesystem::path> file_paths = glob::rglob(stage.files);
            std::vector<std::exception_ptr> exceptions;

            std::vector<ast_file> files;
            files.resize(file_paths.size());

            const auto generate = [&](const std::filesystem::path& file_path) {
                ast_file file;
                try
                {
                    const auto then = std::chrono::steady_clock::now();

                    if (!parser->parse_file(file_path.string(), file))
                    {
                        throw codegen_error(codegen_error_code::parsing, "failed to parse input, file={}", file_path.string());
                    }

                    std::size_t index = 0;
                    for (const codegen_config_step& step : stage.steps)
                    {
                        run_step(step, file, stage_templates.at(index));
                    }

                    const auto now = std::chrono::steady_clock::now();
                    const std::chrono::duration<std::float_t> duration = now - then;
                    SPDLOG_INFO("codegen completed for file, file={} duration={}s", file.path, duration.count());
                }
                catch (...)
                {
                    std::lock_guard lock(mutex);
                    exceptions.emplace_back(std::current_exception());
                }

                return file;
            };

            if (options.sequential)
            {
                std::for_each(std::execution::seq, file_paths.begin(), file_paths.end(), generate);
            }
            else
            {
                std::for_each(std::execution::par, file_paths.begin(), file_paths.end(), generate);
            }

            std::lock_guard lock(mutex);
            if (!exceptions.empty())
            {
                std::rethrow_exception(*exceptions.begin());
            }
        }

        inline void run_step(const codegen_config_step& step, const ast_file& file, const std::vector<resolved_template>& templates)
        {
            std::filesystem::path file_path(file.path);

            std::vector<resolved_template> dirty_templates;
            dirty_templates.reserve(templates.size());

            const bool input_up_to_date = cache.check_and_update(file.path);
            const auto predicate = [&](const resolved_template& template_) {
                return !template_.up_to_date || !input_up_to_date;
            };

            std::copy_if(templates.begin(), templates.end(), std::back_inserter(dirty_templates), predicate);

            if (dirty_templates.empty())
            {
                SPDLOG_DEBUG("skipping input because everything is up-to-date, file={}", file.path);
                return;
            }

            std::shared_ptr<ast_condition> condition;
            if (step.condition.has_value())
            {
                condition = ast_condition_factory::instance().make_condition(step.condition.value());
            }

            if (condition && !condition->matches_condition(file))
            {
                SPDLOG_DEBUG("skipping input because it does not match condition, file={}", file.path);
                return;
            }

            std::vector<std::string> outputs;
            outputs.reserve(dirty_templates.size());

            const auto transform = [&](const resolved_template& template_) {
                std::filesystem::path output_ext = std::filesystem::path(template_.path).stem();
                std::filesystem::path output_filename = fmt::format("{}.{}", file_path.stem().string(), output_ext.string());
                std::filesystem::path output_directory = std::filesystem::path(options.output) / step.directory / file_path.parent_path();
                std::string output = std::filesystem::absolute(output_directory / output_filename).string();
                return output;
            };

            std::transform(dirty_templates.begin(), dirty_templates.end(), std::back_inserter(outputs), transform);

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

            nlohmann::json json_context;
            json_context["outputs"] = outputs;
            json_context["user_data"] = json_user_data;
            json_data["$"] = json_context;

            std::size_t index_template = 0;
            for (const resolved_template& template_ : dirty_templates)
            {
                std::string result;
                if (!renderer->render_file(template_.path, json_data, result))
                {
                    throw codegen_error(codegen_error_code::rendering, "failed to render input, file={} template={}", file.path, template_.path);
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

        inline std::vector<resolved_template> resolve_templates(const std::vector<std::string>& templates)
        {
            const auto transform = [&](const std::string& template_) {
                resolved_template resolved_template;

                SPDLOG_DEBUG("  search template, file={}", template_);
                if (std::filesystem::exists(template_))
                {
                    resolved_template.path = std::filesystem::absolute(template_).string();
                    SPDLOG_DEBUG("  found template, file={}", template_);
                }
                else
                {
                    const auto action_predicate = [&](const std::string& template_path) {
                        std::filesystem::path possible_template = std::filesystem::path(template_path) / template_;

                        SPDLOG_DEBUG("  search template, file={}", possible_template.string());
                        if (std::filesystem::exists(possible_template))
                        {
                            resolved_template.path = std::filesystem::absolute(possible_template).string();
                            SPDLOG_DEBUG("  found template, file={}", possible_template.string());
                            return true;
                        }

                        return false;
                    };

                    if (!std::any_of(options.template_paths.begin(), options.template_paths.end(), action_predicate))
                    {
                        throw codegen_error(codegen_error_code::configuring, "could not find template file, file={}", template_);
                    }
                }

                resolved_template.up_to_date = cache.check_and_update(resolved_template.path);
                return resolved_template;
            };

            std::vector<resolved_template> resolved_templates;
            resolved_templates.reserve(templates.size());

            SPDLOG_DEBUG("searching for templates");
            std::transform(templates.begin(), templates.end(), std::back_inserter(resolved_templates), transform);

            return resolved_templates;
        }
    };
}
