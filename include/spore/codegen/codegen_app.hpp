#pragma once

#include <chrono>
#include <execution>
#include <filesystem>
#include <optional>
#include <ranges>
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
#include "spore/codegen/ast/parsers/ast_parser_clang.hpp"
#include "spore/codegen/ast/parsers/ast_parser_clang_cpp.hpp"
#include "spore/codegen/codegen_cache.hpp"
#include "spore/codegen/codegen_config.hpp"
#include "spore/codegen/codegen_error.hpp"
#include "spore/codegen/codegen_helpers.hpp"
#include "spore/codegen/codegen_options.hpp"
#include "spore/codegen/codegen_version.hpp"
#include "spore/codegen/misc/defer.hpp"
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

            explicit current_path_scope(const std::filesystem::path& new_path)
            {
                old_path = std::filesystem::current_path();
                std::filesystem::current_path(new_path);
            }

            ~current_path_scope()
            {
                std::filesystem::current_path(old_path);
            }

            current_path_scope(const current_path_scope&) = delete;
            current_path_scope(current_path_scope&&) = delete;

            current_path_scope& operator=(const current_path_scope&) = delete;
            current_path_scope& operator=(current_path_scope&&) = delete;
        };
    }

    struct codegen_file_task
    {
        std::string template_;
        std::string output;
    };

    struct codegen_file_step
    {
        std::shared_ptr<ast_condition> condition;
        std::vector<codegen_file_task> file_tasks;
    };

    struct codegen_file_stage
    {
        std::string file;
        std::vector<codegen_file_step> file_steps;
    };

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

        explicit codegen_app(codegen_options in_options)
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

            options.definitions.emplace_back(SPORE_CODEGEN_MACRO, "1");
            // options.definitions.emplace_back("SPORE_ATTRIBUTE_V2_IMPL(...)", "__attribute__((#__VA_ARGS__))");
            // options.definitions.emplace_back("SPORE_ATTRIBUTE_V2(...)", "SPORE_ATTRIBUTE_V2_IMPL(__VA_ARGS__)");

            for (const std::pair<std::string, std::string>& pair : options.user_data)
            {
                json_user_data[pair.first] = pair.second;
            }

            parser = std::make_shared<ast_parser_clang>(options);
            converter = std::make_shared<ast_converter_default>();
            renderer = std::make_shared<codegen_renderer_composite>(
                std::make_shared<codegen_renderer_inja>(options.template_paths));
        }

        void run()
        {
            const auto then = std::chrono::steady_clock::now();

            const auto finally = [&]() {
                nlohmann::json cache_data = nlohmann::json(cache).dump(2);
                if (!write_file(options.cache, cache_data))
                {
                    SPDLOG_WARN("failed to write cache, file={}", options.cache);
                }

                std::lock_guard lock(mutex);
                if (!processes.empty())
                {
                    SPDLOG_DEBUG("waiting on pending processes");
                    for (const std::unique_ptr<TinyProcessLib::Process>& process : processes)
                    {
                        process->get_exit_status();
                    }

                    processes.clear();
                }

                const auto now = std::chrono::steady_clock::now();
                const std::chrono::duration<std::float_t> duration = now - then;
                SPDLOG_INFO("codegen completed, duration={}s", duration.count());
            };

            defer defer_finally {finally};

            const auto action = [&](const codegen_config_stage& stage) {
                run_stage(stage);
            };

            std::for_each(config.stages.begin(), config.stages.end(), action);
        }

        void run_stage(const codegen_config_stage& stage)
        {
            std::filesystem::path stage_directory = std::filesystem::current_path() / stage.directory;
            if (!std::filesystem::exists(stage_directory))
            {
                SPDLOG_DEBUG("skipping stage because directory does not exist, directory={}", stage_directory.string());
                return;
            }

            details::current_path_scope directory_scope(stage_directory);
            std::vector<std::exception_ptr> exceptions;

#if 0
            const auto then = std::chrono::steady_clock::now();
            std::vector<ast_file> asts;
            std::vector<std::filesystem::path> files = glob::rglob("**/*.hpp");
            ast_parser_clang_cpp ast_parser {options};
            ast_parser.parse_files(files, asts);

            const auto now = std::chrono::steady_clock::now();
            const std::chrono::duration<std::float_t> duration = now - then;
            SPDLOG_INFO("__ codegen completed for all files, duration={}s", duration.count());
#endif

            std::vector<codegen_file_stage> file_stages;
            make_file_stages(stage, file_stages);

            const auto action = [&](const codegen_file_stage& file_stage) {
                std::atomic_thread_fence(std::memory_order_acquire);
                try
                {
                    const auto then = std::chrono::steady_clock::now();

                    run_file_stage(file_stage);

                    const auto now = std::chrono::steady_clock::now();
                    const std::chrono::duration<std::float_t> duration = now - then;

                    SPDLOG_INFO("codegen completed for file, file={} duration={}s", file_stage.file, duration.count());
                }
                catch (...)
                {
                    std::lock_guard lock(mutex);
                    exceptions.emplace_back(std::current_exception());
                }
            };

            std::size_t parallelism = options.parallelism > 1 ? options.parallelism : 1;
            if (file_stages.size() < parallelism)
            {
                std::for_each(std::execution::par, file_stages.begin(), file_stages.end(), action);
            }
            else
            {
                std::vector<std::size_t> indices;
                indices.resize(parallelism);

                std::iota(indices.begin(), indices.end(), 0);

                std::size_t partition_size = file_stages.size() / parallelism;
                std::size_t partition_remainder = file_stages.size() % parallelism;

                const auto parallel_action = [&](std::size_t index) {
                    std::size_t index_begin_offset = index < partition_remainder ? index : partition_remainder;
                    std::size_t index_begin = (index * partition_size) + index_begin_offset;

                    std::size_t index_end_offset = index < partition_remainder ? 1 : 0;
                    std::size_t index_end = index_begin + partition_size + index_end_offset;

                    const auto it_begin = file_stages.begin() + static_cast<std::ptrdiff_t>(index_begin);
                    const auto it_end = file_stages.begin() + static_cast<std::ptrdiff_t>(index_end);

                    std::for_each(std::execution::seq, it_begin, it_end, action);
                };

                std::for_each(std::execution::par, indices.begin(), indices.end(), parallel_action);
            }

            std::lock_guard lock(mutex);
            if (!exceptions.empty())
            {
                std::rethrow_exception(*exceptions.begin());
            }

            for (const codegen_file_stage& file_stage : file_stages)
            {
                for (const codegen_file_step& file_step : file_stage.file_steps)
                {
                    for (const codegen_file_task& file_task : file_step.file_tasks)
                    {
                        cache.check_and_update(file_task.output);
                    }
                }
            }
        }

        void run_file_stage(const codegen_file_stage& file_stage)
        {
            ast_file file;
            if (!parser->parse_file(file_stage.file, file))
            {
                throw codegen_error(codegen_error_code::parsing, "failed to parse input, file={}", file_stage.file);
            }

            nlohmann::json json_data;
            if (!converter->convert_file(file, json_data))
            {
                throw codegen_error(codegen_error_code::rendering, "failed to convert input data to json, file={}", file.path);
            }

            std::filesystem::path file_path(file_stage.file);
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

            // TODO find out a better way to achieve the same thing
            std::vector<std::string> outputs;
            for (const codegen_file_step& file_step : file_stage.file_steps)
            {
                if (file_step.condition && !file_step.condition->matches_condition(file))
                {
                    continue;
                }

                for (const codegen_file_task& file_task : file_step.file_tasks)
                {
                    outputs.emplace_back(file_task.output);
                }
            }

            nlohmann::json json_context;
            json_context["outputs"] = outputs;
            json_context["user_data"] = json_user_data;
            json_data["$"] = json_context;

            for (const codegen_file_step& file_step : file_stage.file_steps)
            {
                if (file_step.condition && !file_step.condition->matches_condition(file))
                {
                    SPDLOG_DEBUG("skipping input because it does not match condition, file={}", file.path);
                    continue;
                }

                for (const codegen_file_task& file_task : file_step.file_tasks)
                {
                    SPDLOG_DEBUG("rendering output, file={} template={}", file.path, file_task.template_);

                    std::string result;
                    if (!renderer->render_file(file_task.template_, json_data, result))
                    {
                        throw codegen_error(codegen_error_code::rendering, "failed to render input, file={} template={}", file.path, file_task.template_);
                    }

                    SPDLOG_DEBUG("writing output, file={}", file_task.output);

                    if (!spore::codegen::write_file(file_task.output, result))
                    {
                        throw codegen_error(codegen_error_code::io, "failed to write output, file={}", file_task.output);
                    }

                    if (!options.reformat.empty())
                    {
                        std::lock_guard lock(mutex);
                        std::string command = fmt::format("{} {}", options.reformat, file_task.output);
                        processes.emplace_back(std::make_unique<TinyProcessLib::Process>(command));
                    }
                }
            }
        }

        void make_file_stages(const codegen_config_stage& stage, std::vector<codegen_file_stage>& file_stages)
        {
            std::vector<std::shared_ptr<ast_condition>> conditions;
            conditions.reserve(stage.steps.size());

            for (const codegen_config_step& step : stage.steps)
            {
                std::shared_ptr<ast_condition>& condition = conditions.emplace_back();
                if (step.condition.has_value())
                {
                    condition = ast_condition_factory::instance().make_condition(step.condition.value());
                }
            }

            std::vector<std::vector<std::string>> templates;
            templates.reserve(stage.steps.size());

            std::vector<std::vector<bool>> templates_up_to_date;
            templates_up_to_date.reserve(stage.steps.size());

            for (const codegen_config_step& step : stage.steps)
            {
                std::vector<std::string>& step_templates = templates.emplace_back(step.templates);
                resolve_templates(step_templates);

                const auto transform = [&](const std::string& template_) {
                    return cache.check_and_update(template_);
                };

                std::transform(step_templates.begin(), step_templates.end(), std::back_inserter(templates_up_to_date.emplace_back()), transform);
            }

            std::vector<std::filesystem::path> files = glob::rglob(stage.files);
            file_stages.reserve(files.size());

            for (const std::filesystem::path& file : files)
            {
                codegen_file_stage file_stage;
                file_stage.file = file.string();
                replace_all(file_stage.file, "\\", "/");

                const bool file_up_to_date = cache.check_and_update(file.string());
                for (std::size_t index_step = 0; index_step < stage.steps.size(); ++index_step)
                {
                    const codegen_config_step& step = stage.steps.at(index_step);
                    const std::vector<std::string>& step_templates = templates.at(index_step);
                    const std::vector<bool>& step_templates_up_to_date = templates_up_to_date.at(index_step);
                    const std::shared_ptr<ast_condition>& step_condition = conditions.at(index_step);

                    codegen_file_step file_step;
                    file_step.condition = step_condition;

                    for (std::size_t index_template = 0; index_template < step_templates.size(); ++index_template)
                    {
                        const std::string& template_ = step_templates.at(index_template);
                        const bool template_up_to_date = step_templates_up_to_date.at(index_template);

                        // TODO can't check whether output is up to date because of conditions
                        // const bool output_up_to_date = cache.check_and_update(output);

                        if (!file_up_to_date || !template_up_to_date)
                        {
                            std::filesystem::path output_ext = std::filesystem::path(template_).stem();
                            std::filesystem::path output_filename = fmt::format("{}.{}", file.stem().string(), output_ext.string());
                            std::filesystem::path output_directory = std::filesystem::path(options.output) / step.directory / file.parent_path();
                            std::string output = std::filesystem::absolute(output_directory / output_filename).string();

                            codegen_file_task& file_task = file_step.file_tasks.emplace_back();
                            file_task.template_ = template_;
                            file_task.output = std::move(output);
                        }
                    }

                    if (!file_step.file_tasks.empty())
                    {
                        file_stage.file_steps.emplace_back(std::move(file_step));
                    }
                }

                if (!file_stage.file_steps.empty())
                {
                    file_stages.emplace_back(std::move(file_stage));
                }
                else
                {
                    SPDLOG_DEBUG("skipping file because everything is up-to-date, file={}", file.string());
                }
            }
        }

        void resolve_templates(std::vector<std::string>& templates)
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

                    return false;
                };

                if (!std::any_of(options.template_paths.begin(), options.template_paths.end(), action_predicate))
                {
                    throw codegen_error(codegen_error_code::configuring, "could not find template file, file={}", template_);
                }
            }
        }
    };
}
