#pragma once

#include <chrono>
#include <filesystem>
#include <string>
#include <string_view>
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
#include "spore/codegen/utils/strings.hpp"

#ifndef SPORE_CODEGEN_MACRO
#define SPORE_CODEGEN_MACRO "SPORE_CODEGEN"
#endif

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
        std::map<std::string, nlohmann::json> debug_map;

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

            std::string config_data;
            if (!files::read_file(options.config, config_data))
            {
                throw codegen_error(codegen_error_code::io, "unable to read config, file={}", options.config);
            }

            try
            {
                nlohmann::json::parse(config_data).get_to(config);
            }
            catch (...)
            {
                throw codegen_error(codegen_error_code::configuring, "invalid config, file={}", options.config);
            }

            std::string cache_data;
            if (!options.force && files::read_file(options.cache, cache_data))
            {
                try
                {
                    nlohmann::json::parse(cache_data).get_to(cache);
                }
                catch (...)
                {
                    SPDLOG_INFO("ignoring invalid cache, file={}", options.cache);
                    cache.reset();
                }

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

            options.definitions.emplace_back(SPORE_CODEGEN_MACRO, "1");
            options.template_paths.emplace_back(std::filesystem::current_path().string());

            for (const std::pair<std::string, std::string>& pair : options.user_data)
            {
                user_data[pair.first] = pair.second;
            }

            parser = std::make_shared<ast_parser_clang>(options);
            converter = std::make_shared<ast_converter_default>();
            renderer = std::make_shared<codegen_renderer_composite>(
                std::make_shared<codegen_renderer_inja>(options.template_paths));
        }

        inline void run()
        {
            const auto then = std::chrono::steady_clock::now();

            const auto finally = [&] {
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

            const auto action = [&](const codegen_config_stage& stage) {
                run_stage(stage);
            };

            defer defer_finally {finally};

            std::for_each(config.stages.begin(), config.stages.end(), action);

            std::string cache_data = nlohmann::json(cache).dump(2);

            if (!files::write_file(options.cache, cache_data))
            {
                SPDLOG_WARN("failed to write cache, file={}", options.cache);
            }

            if (options.debug)
            {
                std::string debug_file = (std::filesystem::path(options.output) / "debug.json").string();
                std::string debug_data = nlohmann::json(debug_map).dump(2);

                if (!files::write_file(debug_file, debug_data))
                {
                    SPDLOG_WARN("failed to write debug data, file={}", debug_file);
                }
            }
        }

        inline void run_stage(const codegen_config_stage& stage)
        {
            std::filesystem::path stage_directory = std::filesystem::current_path() / stage.directory;
            if (!std::filesystem::exists(stage_directory))
            {
                SPDLOG_DEBUG("skipping stage because directory does not exist, stage={} directory={}", stage.name, stage_directory.string());
                return;
            }

            current_path_scope directory_scope(stage_directory);

            codegen_stage_data data;
            make_stage_data(stage, data);

            std::vector<std::size_t> dirty_indices;
            dirty_indices.reserve(data.files.size());

            std::vector<std::string> dirty_files;
            dirty_files.reserve(data.files.size());

            const auto predicate = [](const codegen_step_data& step) {
                const auto predicate = [](const codegen_template_data& template_) {
                    return template_.status != codegen_cache_status::up_to_date;
                };
                return std::any_of(step.templates.begin(), step.templates.end(), predicate);
            };

            const bool has_dirty_templates = std::any_of(data.steps.begin(), data.steps.end(), predicate);

            for (std::size_t index = 0; index < data.files.size(); ++index)
            {
                const codegen_file_data& file = data.files.at(index);

                if (has_dirty_templates || file.status != codegen_cache_status::up_to_date)
                {
                    dirty_indices.emplace_back(index);
                    dirty_files.emplace_back(file.path);
                }
            }

            if (dirty_indices.empty())
            {
                SPDLOG_INFO("skipping stage, all files are up-to-date, stage={}", stage.name);
                return;
            }

            std::vector<ast_file> asts;
            parse_files(stage, dirty_files, asts);

            for (std::size_t index_step = 0; index_step < stage.steps.size(); ++index_step)
            {
                const codegen_step_data& step = data.steps.at(index_step);

                for (std::size_t index_file = 0; index_file < dirty_files.size(); ++index_file)
                {
                    const codegen_file_data& file = data.files.at(dirty_indices.at(index_file));
                    const ast_file& ast = asts.at(index_file);

                    if (step.condition == nullptr || step.condition->matches_condition(ast))
                    {
                        render_file(data, step, file, ast);
                    }
                }
            }
        }

        inline void render_file(const codegen_stage_data& stage, const codegen_step_data& step, const codegen_file_data& file, const ast_file& ast)
        {
            nlohmann::json json_data;
            if (!converter->convert_file(ast, json_data))
            {
                throw codegen_error(codegen_error_code::rendering, "failed to convert input data to json, file={}", file.path);
            }

            json_data["$"] = {
                {"stage", stage},
                {"step", step},
                {"file", file},
                {"user_data", user_data},
            };

            for (std::size_t index = 0; index < step.templates.size(); ++index)
            {
                const codegen_template_data& template_ = step.templates.at(index);
                json_data["$"]["template"] = template_;

                std::string output = fmt::format("{}.{}", file.output_prefix, template_.output_suffix);

                SPDLOG_DEBUG("rendering output, file={}", output);

                std::string result;
                if (!renderer->render_file(template_.path, json_data, result))
                {
                    throw codegen_error(codegen_error_code::rendering, "failed to render input, file={} template={}", file.path, template_.path);
                }

                SPDLOG_DEBUG("writing output, file={}", output);

                if (!files::write_file(output, result))
                {
                    throw codegen_error(codegen_error_code::io, "failed to write output, file={}", output);
                }

                if (!options.reformat.empty())
                {
                    std::string command = fmt::format("{} {}", options.reformat, output);
                    processes.emplace_back(std::make_unique<TinyProcessLib::Process>(command));
                }

                if (options.debug)
                {
                    debug_map.emplace(output, json_data);
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

            SPDLOG_INFO("parsing completed, stage={} duration={}s", stage.name, duration.count());
        }

        inline void make_stage_data(const codegen_config_stage& stage, codegen_stage_data& data)
        {
            for (const codegen_config_step& step : stage.steps)
            {
                std::vector<std::filesystem::path> step_files = glob::rglob(stage.files);
                std::vector<std::string> step_templates = step.templates;

                resolve_templates(step_templates);

                const auto transform_template = [&](std::string& template_) {
                    codegen_cache_status status = cache.check_and_update(template_);
                    std::filesystem::path output_suffix = std::filesystem::path(template_).stem();
                    return codegen_template_data {
                        .path = std::move(template_),
                        .output_suffix = output_suffix.string(),
                        .status = status,
                    };
                };

                const auto transform_file = [&](std::filesystem::path& file) {
                    std::string file_string = file.string();
                    std::filesystem::path output_stem = std::filesystem::path(file).stem();
                    std::filesystem::path output_directory = std::filesystem::path(options.output) / step.directory / std::filesystem::path(file).parent_path();
                    std::filesystem::path output_prefix = std::filesystem::absolute(output_directory / output_stem);
                    codegen_cache_status status = cache.check_and_update(file_string);
                    return codegen_file_data {
                        .path = std::move(file_string),
                        .output_prefix = output_prefix.string(),
                        .status = status,
                    };
                };

                codegen_step_data& step_data = data.steps.emplace_back();

                if (step.condition.has_value())
                {
                    step_data.condition = ast_condition_factory::instance().make_condition(step.condition.value());
                }

                std::transform(step_templates.begin(), step_templates.end(), std::back_inserter(step_data.templates), transform_template);
                std::transform(step_files.begin(), step_files.end(), std::back_inserter(data.files), transform_file);
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

                const auto predicate = [&](const std::string& template_path) {
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

                if (!std::any_of(options.template_paths.begin(), options.template_paths.end(), predicate))
                {
                    throw codegen_error(codegen_error_code::configuring, "could not find template file, file={}", template_);
                }
            }
        }
    };
}
