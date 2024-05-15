#pragma once

#include <algorithm>
#include <functional>
#include <string>
#include <string_view>
#include <vector>
#include <variant>

#include "fmt/format.h"
#include "nlohmann/json.hpp"
#include "spdlog/spdlog.h"

#include "spore/codegen/ast/parsers/ast_parser.hpp"
#include "spore/codegen/codegen_helpers.hpp"
#include "spore/codegen/codegen_options.hpp"
#include "spore/codegen/misc/defer.hpp"

namespace spore::codegen
{
    namespace detail
    {
        template <typename value_t>
        void get_to_optional(std::string_view name, const nlohmann::json& json, value_t& value)
        {
            if (json.contains(name))
            {
                json.at(name).get_to(value);
            }
        }

        enum class clang_ast_kind
        {
            none,
            cxx_record_decl,
            cxx_method_decl,
            enum_decl,
            namespace_decl,
            using_decl,
        };

        inline void from_json(const nlohmann::json& json, clang_ast_kind& value)
        {
            static std::map<std::string_view, clang_ast_kind> value_map {
                {"CXXRecordDecl", clang_ast_kind::cxx_record_decl},
                {"CXXMethodDecl", clang_ast_kind::cxx_method_decl},
                {"EnumDecl", clang_ast_kind::enum_decl},
                {"NamespaceDecl", clang_ast_kind::namespace_decl},
                {"UsingDecl", clang_ast_kind::using_decl},
            };
            const auto it_value = value_map.find(json.get<std::string>());
            value = it_value != value_map.end() ? it_value->second : clang_ast_kind::none;
        }

        struct clang_ast_loc
        {
            struct included_from_data
            {
                std::string file;
            };

            std::string file;
            std::size_t line = 0;
            std::size_t col = 0;
            std::size_t offset = 0;
            std::size_t tok_len = 0;
            included_from_data included_from;
        };

        inline void from_json(const nlohmann::json& json, clang_ast_loc& value)
        {
            get_to_optional("file", json, value.file);
            get_to_optional("line", json, value.line);
            get_to_optional("col", json, value.col);
            get_to_optional("offset", json, value.offset);
            get_to_optional("tokLen", json, value.tok_len);
            get_to_optional("includedFrom", json, value.included_from);
        }

        inline void from_json(const nlohmann::json& json, clang_ast_loc::included_from_data& value)
        {
            get_to_optional("file", json, value.file);
        }

        struct clang_ast_loc_template
        {
            clang_ast_loc spelling_loc;
            clang_ast_loc expansion_loc;
        };

        inline void from_json(const nlohmann::json& json, clang_ast_loc_template& value)
        {
            get_to_optional("spellingLoc", json, value.spelling_loc);
            get_to_optional("expansionLoc", json, value.expansion_loc);
        }

        using clang_ast_loc_variant = std::variant<clang_ast_loc, clang_ast_loc_template>;

        inline void from_json(const nlohmann::json& json, clang_ast_loc_variant& value)
        {
            if (json.contains("spellingLoc"))
            {
                json.get_to(value.emplace<clang_ast_loc_template>());
            }
            else
            {
                json.get_to(value.emplace<clang_ast_loc>());
            }
        }

        struct clang_ast_loc_range
        {
            clang_ast_loc begin;
            clang_ast_loc end;
        };

        inline void from_json(const nlohmann::json& json, clang_ast_loc_range& value)
        {
            get_to_optional("begin", json, value.begin);
            get_to_optional("end", json, value.end);
        }

        struct clang_ast_type_definition_data
        {
            struct definition_flags
            {
                bool defaulted_is_constexpr = false;
                bool exists = false;
                bool has_const_param = false;
                bool implicit_has_const_param = false;
                bool irrelevant = false;
                bool is_constexpr = false;
                bool needs_implicit = false;
                bool simple = false;
                bool trivial = false;
            };

            definition_flags dtor;
            definition_flags default_ctor;
            definition_flags copy_ctor;
            definition_flags copy_assign;
            definition_flags move_ctor;
            definition_flags move_assign;
            bool has_constexpr_non_copy_move_constructor = false;
            bool is_aggregate = false;
            bool is_literal = false;
            bool is_pod = false;
            bool is_standard_layout = false;
            bool is_trivial = false;
            bool is_trivially_copyable = false;
        };

        inline void from_json(const nlohmann::json& json, clang_ast_type_definition_data::definition_flags& value)
        {
            get_to_optional("defaultedIsConstexpr", json, value.defaulted_is_constexpr);
            get_to_optional("exists", json, value.exists);
            get_to_optional("hasConstParam", json, value.has_const_param);
            get_to_optional("implicitHasConstParam", json, value.implicit_has_const_param);
            get_to_optional("irrelevant", json, value.irrelevant);
            get_to_optional("isConstexpr", json, value.is_constexpr);
            get_to_optional("needsImplicit", json, value.needs_implicit);
            get_to_optional("simple", json, value.simple);
            get_to_optional("trivial", json, value.trivial);
        }

        inline void from_json(const nlohmann::json& json, clang_ast_type_definition_data& value)
        {
            get_to_optional("dtor", json, value.dtor);
            get_to_optional("defaultCtor", json, value.default_ctor);
            get_to_optional("copyCtor", json, value.copy_ctor);
            get_to_optional("copyAssign", json, value.copy_assign);
            get_to_optional("moveCtor", json, value.move_ctor);
            get_to_optional("moveAssign", json, value.move_assign);
            get_to_optional("hasConstexprNonCopyMoveConstructor", json, value.has_constexpr_non_copy_move_constructor);
            get_to_optional("isAggregate", json, value.is_aggregate);
            get_to_optional("isLiteral", json, value.is_literal);
            get_to_optional("isPOD", json, value.is_pod);
            get_to_optional("isStandardLayout", json, value.is_standard_layout);
            get_to_optional("isTrivial", json, value.is_trivial);
            get_to_optional("isTriviallyCopyable", json, value.is_trivially_copyable);
        }

        struct clang_ast_type_data
        {
            std::string tag_used;
            clang_ast_type_definition_data definition_data;
            bool complete_definition = false;
        };

        inline void from_json(const nlohmann::json& json, clang_ast_type_data& value)
        {
            get_to_optional("tagUsed", json, value.tag_used);
            get_to_optional("definitionData", json, value.definition_data);
            get_to_optional("completeDefinition", json, value.complete_definition);
        }

        using clang_ast_node_data = std::variant<
            std::monostate,
            clang_ast_type_data>;

        struct clang_ast_node
        {
            clang_ast_kind kind = clang_ast_kind::none;
            clang_ast_loc_variant loc;
            clang_ast_node_data data;
            std::vector<clang_ast_node> inner;
        };

        template <typename value_t>
        inline const std::function<void(const nlohmann::json&, clang_ast_node_data&)> from_json_adapter =
            [](const nlohmann::json& json, clang_ast_node_data& data) {
                json.get_to(data.emplace<value_t>());
            };

        inline void from_json(const nlohmann::json& json, clang_ast_node& value)
        {
            static std::map<clang_ast_kind, std::function<void(const nlohmann::json&, clang_ast_node_data&)>> adapter_map {
                {clang_ast_kind::cxx_record_decl, from_json_adapter<clang_ast_type_data>},
            };

            get_to_optional("loc", json, value.loc);
            get_to_optional("kind", json, value.kind);
            get_to_optional("inner", json, value.inner);

            const auto it_adapter = adapter_map.find(value.kind);
            if (it_adapter != adapter_map.end())
            {
                const auto& [_, adapter] = *it_adapter;
                adapter(json, value.data);
            }
        }
    }

    struct ast_parser_clang_process : ast_parser
    {
        std::string clang;
        std::vector<std::string> args;

        explicit ast_parser_clang_process(const codegen_options& options)
        {
            clang = options.clang;

            args.emplace_back("-xc++");
            args.emplace_back("-Wno-everything");
            args.emplace_back(fmt::format("-std={}", options.cpp_standard));

            for (const std::string& include : options.includes)
            {
                args.emplace_back(fmt::format("-I{}", include));
            }

            for (const auto& [key, value] : options.definitions)
            {
                if (value.empty())
                {
                    args.emplace_back(fmt::format("-D{}", key));
                }
                else
                {
                    args.emplace_back(fmt::format("-D{}={}", key, value));
                }
            }

            std::sort(args.begin(), args.end());
            args.erase(std::unique(args.begin(), args.end()), args.end());
        }

        bool parse_file(const std::string& path, ast_file& file) override
        {
            std::string source;
            if (!read_file(path, source))
            {
                SPDLOG_ERROR("cannot read file, path={}", path);
                return false;
            }

            constexpr std::string_view flag_struct =
                "#ifndef __SPORE_CODEGEN_DEFINE__\n"
                "#define __SPORE_CODEGEN_DEFINE__\n"
                "    struct __SPORE_CODEGEN__;\n"
                "#endif\n";

            source.insert(0, flag_struct);

            std::string command;
            command += clang + " ";
            command += join(" ", args, std::identity()) + " ";
            command += "-fsyntax-only -Xclang -ast-dump=json -";

            auto [result, out, err] = run(command, source);
            if (result != 0)
            {
                SPDLOG_ERROR("cannot preprocess macros, path={} result={} error={}", path, result, err);
                return false;
            }

            // write_file("c:/production/out.hpp.json", out);
            //
            // auto callback = [](int depth, nlohmann::json::parse_event_t event, nlohmann::json& parsed) {
            //     return true;
            // };

            std::size_t begin = 0;
            std::size_t depth = 0;
            std::vector<std::tuple<std::size_t, std::size_t>> ranges;
            bool relevant = false;

            for (std::size_t index = 0; index < out.size(); ++index)
            {
                const char c = out[index];

                if (c == '{')
                {
                    if (++depth == 1)
                    {
                        begin = index;
                    }
                }
                else if (c == '}')
                {
                    if (--depth == 0)
                    {
                        if (relevant)
                        {
                            ranges.emplace_back(begin, index + 1);
                        }
                        else
                        {
                            std::string_view json_string {out.data() + begin, (index + 1) - begin};
                            relevant = json_string.find("__SPORE_CODEGEN__") != std::string_view::npos;
                        }
                    }
                }
            }

            std::vector<detail::clang_ast_node> nodes;
            nodes.resize(ranges.size());

            std::size_t index = 0;
            for (const auto& [begin, end] : ranges)
            {
                std::string_view json_string {out.data() + begin, end - begin};
                nlohmann::json json = nlohmann::json::parse(json_string);
                json.get_to(nodes[index]);
                ++index;
            }

            // std::stringstream stream(out);
            //
            // nlohmann::json json = nlohmann::json::parse(out, callback, false);
            // detail::clang_ast_node node;
            // json.get_to(node);

            //  SPDLOG_DEBUG("found node, file={}", node.loc.file);

            return true;
        }
    };
}
