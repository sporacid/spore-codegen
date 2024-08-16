#pragma once

#include <chrono>
#include <filesystem>
#include <map>
#include <string>
#include <string_view>
#include <vector>

#include "glob/glob.h"
#include "nlohmann/json.hpp"
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
#include "spore/codegen/codegen_cache.hpp"
#include "spore/codegen/codegen_config.hpp"
#include "spore/codegen/codegen_data.hpp"
#include "spore/codegen/codegen_error.hpp"
#include "spore/codegen/codegen_options.hpp"
#include "spore/codegen/codegen_version.hpp"
#include "spore/codegen/misc/current_path_scope.hpp"
#include "spore/codegen/misc/defer.hpp"
#include "spore/codegen/renderers/codegen_renderer.hpp"
#include "spore/codegen/renderers/codegen_renderer_composite.hpp"
#include "spore/codegen/renderers/codegen_renderer_inja.hpp"
#include "spore/codegen/utils/files.hpp"

namespace spore::codegen
{
    namespace detail
    {
        std::once_flag setup_once_flag;

        void setup_once()
        {
            ast_condition_factory::instance().register_condition<ast_condition_any>();
            ast_condition_factory::instance().register_condition<ast_condition_all>();
            ast_condition_factory::instance().register_condition<ast_condition_none>();
            ast_condition_factory::instance().register_condition<ast_condition_attribute>();
        }

        template <typename func_t, typename end_func_t>
        void run_timed(func_t&& func, end_func_t&& end_func)
        {
            const auto then = std::chrono::steady_clock::now();
            const auto finally = [&] {
                const auto now = std::chrono::steady_clock::now();
                const std::chrono::duration<std::float_t> duration = now - then;
                end_func(duration.count());
            };

            defer defer = finally;
            func();
        }
    }

    struct codegen_app
    {
        using process_ptr = std::unique_ptr<TinyProcessLib::Process>;

        codegen_config config;
        codegen_cache cache;
        codegen_options options;
        nlohmann::json user_data;
        std::shared_ptr<ast_parser> parser;
        std::shared_ptr<ast_converter> converter;
        std::shared_ptr<codegen_renderer> renderer;
        std::vector<process_ptr> processes;

        explicit codegen_app(codegen_options in_options)
            : options(std::move(in_options))
        {
            std::call_once(detail::setup_once_flag, &detail::setup_once);

            const auto normalize_path = [](std::string& value, const std::string* base = nullptr) {
                std::filesystem::path path(value);

                if (base != nullptr)
                {
                    path = std::filesystem::path(*base) / path;
                }

                if (!path.is_absolute())
                {
                    path = std::filesystem::absolute(path);
                }

                path = path.make_preferred();
                value = path.string();
            };

            normalize_path(options.config);
            normalize_path(options.output);
            normalize_path(options.cache, &options.output);

            nlohmann::json config_json;
            nlohmann::json cache_json;

            if (!files::read_file(options.config, config_json))
            {
                throw codegen_error(codegen_error_code::io, "invalid config, file={}", options.config);
            }

            config = config_json;

            if (!options.force && files::read_file(options.cache, cache_json))
            {
                cache = cache_json;

                if (cache.version != SPORE_CODEGEN_VERSION)
                {
                    SPDLOG_INFO("ignoring old cache, file={} version={}", options.cache, cache.version);
                    cache.reset();
                }
            }

            if (cache.check_and_update(options.config) == codegen_cache_status::dirty)
            {
                SPDLOG_INFO("ignoring old cache because of new config, file={} config={}", options.cache, options.config);
                cache.reset();
                cache.check_and_update(options.config);
            }

            for (const auto& [key, value] : options.user_data)
            {
                user_data[key] = value;
            }

            options.templates.emplace_back(std::filesystem::current_path().string());

            parser = std::make_shared<ast_parser_clang>(options.additional_args);
            converter = std::make_shared<ast_converter_default>();
            renderer = std::make_shared<codegen_renderer_composite>(
                std::make_shared<codegen_renderer_inja>(options.templates));
        }

        inline void run()
        {
            const auto action = [&] {
                defer defer_wait_processes = [&] { wait_pending_processes(); };
                codegen_data data = make_data();

                for (std::size_t index = 0; index < config.stages.size(); ++index)
                {
                    const codegen_config_stage& stage = config.stages.at(index);
                    run_stage(data, data.stages.at(index), stage);
                }

                if (!files::write_file(options.cache, cache))
                {
                    SPDLOG_WARN("failed to write cache, file={}", options.cache);
                }
            };

            const auto finally = [](std::float_t duration) {
                SPDLOG_INFO("codegen completed, duration={:.3f}s", duration);
            };

            detail::run_timed(action, finally);
        }

        inline void run_stage(const codegen_data& data, codegen_stage_data& stage_data, const codegen_config_stage& stage)
        {
            std::filesystem::path directory = std::filesystem::current_path() / stage.directory;
            if (!std::filesystem::exists(directory))
            {
                SPDLOG_DEBUG("skipping stage because directory does not exist, stage={} directory={}", stage.name, directory.string());
                return;
            }

            current_path_scope directory_scope(directory);

            std::vector<std::size_t> dirty_file_indices;
            dirty_file_indices.reserve(stage_data.files.size());

            std::vector<std::string> dirty_files;
            dirty_files.reserve(stage_data.files.size());

            const auto template_predicate = [](const codegen_template_data& template_data) {
                return template_data.status != codegen_cache_status::up_to_date;
            };

            const bool has_dirty_templates = std::any_of(data.templates.begin(), data.templates.end(), template_predicate);

            for (std::size_t file_index = 0; file_index < stage_data.files.size(); ++file_index)
            {
                const codegen_file_data& file_data = stage_data.files.at(file_index);

                if (has_dirty_templates || file_data.status != codegen_cache_status::up_to_date)
                {
                    dirty_file_indices.emplace_back(file_index);
                    dirty_files.emplace_back(file_data.path);
                }
            }

            if (dirty_file_indices.empty())
            {
                SPDLOG_INFO("skipping stage, all files are up-to-date, stage={}", stage.name);
                return;
            }

            std::vector<ast_file> ast_files;
            parse_files(stage, dirty_files, ast_files);
            erase_unmatched_outputs(stage_data, ast_files, dirty_file_indices);

            const auto action = [&] {
                SPDLOG_DEBUG("rendering stage files, stage={} files={}", stage.name, stage.files);
                for (std::size_t file_index = 0; file_index < dirty_files.size(); ++file_index)
                {
                    const ast_file& ast_file = ast_files.at(file_index);
                    const codegen_file_data& file_data = stage_data.files.at(dirty_file_indices.at(file_index));

                    if (!file_data.outputs.empty())
                    {
                        render_file(data, stage_data, file_data, ast_file);
                    }
                }
            };

            const auto finally = [&](std::float_t duration) {
                SPDLOG_INFO("rendering completed, stage={} duration={:.3f}s", stage.name, duration);
            };

            detail::run_timed(action, finally);
        }

        inline void render_file(const codegen_data& data, const codegen_stage_data& stage_data, const codegen_file_data& file_data, const ast_file& ast)
        {
            nlohmann::json json_data;
            if (!converter->convert_file(ast, json_data))
            {
                throw codegen_error(codegen_error_code::rendering, "failed to convert input data to json, file={}", file_data.path);
            }

            json_data["$"] = {
                {"stage", stage_data},
                {"file", file_data},
                {"user_data", user_data},
            };

            for (const codegen_step_data& step_data : stage_data.steps)
            {
                json_data["$"]["step"] = step_data;

                for (std::size_t template_index : step_data.template_indices)
                {
                    const codegen_template_data& template_data = data.templates.at(template_index);

                    const auto output_predicate = [&](const codegen_output_data& output_data) {
                        return output_data.template_index == template_index;
                    };

                    const auto it_output = std::find_if(file_data.outputs.begin(), file_data.outputs.end(), output_predicate);
                    if (it_output == file_data.outputs.end())
                    {
                        SPDLOG_DEBUG("template skipped, no output, file={} template={}", file_data.path, template_data.path);
                        continue;
                    }

                    const codegen_output_data& output_data = *it_output;

                    json_data["$"]["template"] = template_data;
                    json_data["$"]["output"] = output_data;

                    SPDLOG_DEBUG("rendering output, file={}", output_data.path);

                    std::string result;
                    if (!renderer->render_file(template_data.path, json_data, result))
                    {
                        throw codegen_error(codegen_error_code::rendering, "failed to render input, file={} template={}", file_data.path, template_data.path);
                    }

                    SPDLOG_DEBUG("writing output, file={}", output_data.path);

                    if (!files::write_file(output_data.path, result))
                    {
                        throw codegen_error(codegen_error_code::io, "failed to write output, file={}", output_data.path);
                    }

                    if (!options.reformat.empty())
                    {
                        std::string command = fmt::format("{} {}", options.reformat, output_data.path);
                        processes.emplace_back(std::make_unique<TinyProcessLib::Process>(command));
                    }
                }
            }
        }

        static inline void erase_unmatched_outputs(codegen_stage_data& stage_data, const std::vector<ast_file>& ast_files, const std::vector<std::size_t>& file_indices)
        {
            for (std::size_t step_index = 0; step_index < stage_data.steps.size(); ++step_index)
            {
                const codegen_step_data& step_data = stage_data.steps.at(step_index);

                for (std::size_t file_index = 0; file_index < file_indices.size(); ++file_index)
                {
                    const ast_file& ast_file = ast_files.at(file_index);
                    const bool ast_file_matches = step_data.condition == nullptr || step_data.condition->matches_condition(ast_file);

                    if (!ast_file_matches)
                    {
                        codegen_file_data& file_data = stage_data.files.at(file_indices.at(file_index));
                        const auto output_predicate = [&](const codegen_output_data& output_data) {
                            return output_data.step_index == step_index;
                        };

                        std::erase_if(file_data.outputs, output_predicate);
                    }
                }
            }
        }

        inline void parse_files(const codegen_config_stage& stage, const std::vector<std::string>& files, std::vector<ast_file>& ast_files) const
        {
            SPDLOG_INFO("parsing stage files, stage={} files={} count={}", stage.name, stage.files, files.size());

            const auto then = std::chrono::steady_clock::now();

            if (!parser->parse_files(files, ast_files))
            {
                throw codegen_error(codegen_error_code::parsing, "failed to parse stage input files, stage={} files={}", stage.name, stage.files);
            }

            const auto now = std::chrono::steady_clock::now();
            const std::chrono::duration<std::float_t> duration = now - then;

            SPDLOG_INFO("parsing completed, stage={} duration={:.3f}s", stage.name, duration.count());
        }

        inline codegen_data make_data()
        {
            codegen_data data;

            for (std::string& template_ : options.templates)
            {
                std::filesystem::path pattern = std::filesystem::path(template_) / "**" / "*";
                std::vector<std::filesystem::path> template_files = glob::rglob(pattern.string());

                for (const std::filesystem::path& template_file : template_files)
                {
                    std::string template_string = template_file.string();
                    if (renderer->can_render_file(template_string))
                    {
                        codegen_cache_status status = cache.check_and_update(template_string);

                        codegen_template_data template_data {
                            .path = std::move(template_string),
                            .status = status,
                        };

                        data.templates.emplace_back(std::move(template_data));
                    }
                }
            }

            for (const codegen_config_stage& stage : config.stages)
            {
                codegen_stage_data stage_data = make_stage_data(stage, data);
                data.stages.emplace_back(std::move(stage_data));
            }

            return data;
        }

        inline codegen_stage_data make_stage_data(const codegen_config_stage& stage, const codegen_data& data)
        {
            codegen_stage_data stage_data;

            // TODO @sporacid Figure out if we should change working directory or find everything in-place
            current_path_scope directory_scope {std::filesystem::current_path() / stage.directory};

            std::vector<std::filesystem::path> stage_files = glob::rglob(stage.files);
            for (const std::filesystem::path& stage_file : stage_files)
            {
                std::string file_string = stage_file.string();
                codegen_cache_status status = cache.check_and_update(file_string);

                codegen_file_data file_data {
                    .path = std::move(file_string),
                    .status = status,
                };

                stage_data.files.emplace_back(std::move(file_data));
            }

            for (std::size_t step_index = 0; step_index < stage.steps.size(); ++step_index)
            {
                const codegen_config_step& step = stage.steps.at(step_index);

                codegen_step_data step_data;

                if (step.condition.has_value())
                {
                    step_data.condition = ast_condition_factory::instance().make_condition(step.condition.value());
                }

                for (const std::string& template_ : step.templates)
                {
                    const auto template_predicate = [&](const codegen_template_data& template_data) {
                        // TODO @sporacid Find a better way to achieve the same thing without suffix matching
                        return template_data.path.ends_with(std::filesystem::path(template_).make_preferred().string());
                    };

                    const auto it_template = std::find_if(data.templates.begin(), data.templates.end(), template_predicate);
                    if (it_template != data.templates.end())
                    {
                        std::size_t template_index = static_cast<std::size_t>(it_template - data.templates.begin());
                        step_data.template_indices.emplace_back(template_index);

                        for (codegen_file_data& file_data : stage_data.files)
                        {
                            std::filesystem::path output_stem = std::filesystem::path(file_data.path).stem();
                            std::filesystem::path output_directory = std::filesystem::path(options.output) / step.directory / std::filesystem::path(file_data.path).parent_path();
                            std::filesystem::path output_prefix = std::filesystem::absolute(output_directory / output_stem);
                            std::filesystem::path output_suffix = std::filesystem::path(it_template->path).stem();

                            codegen_output_data output_data {
                                .step_index = step_index,
                                .template_index = template_index,
                                .path = fmt::format("{}.{}", output_prefix.string(), output_suffix.string()),
                            };

                            file_data.outputs.emplace_back(std::move(output_data));
                        }
                    }
                }

                stage_data.steps.emplace_back(std::move(step_data));
            }

            return stage_data;
        }

        inline void wait_pending_processes()
        {
            if (processes.empty())
                return;

            SPDLOG_DEBUG("waiting on pending processes, count={}", processes.size());

            for (const std::unique_ptr<TinyProcessLib::Process>& process : processes)
            {
                process->get_exit_status();
            }

            processes.clear();

            SPDLOG_DEBUG("all pending processes completed");
        }
    };
}
