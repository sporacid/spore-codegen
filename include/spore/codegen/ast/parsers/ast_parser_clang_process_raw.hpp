#pragma once

#include <algorithm>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

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
        // enum class clang_ast_kind
        // {
        //     none,
        //     cxx_record_decl,
        //     cxx_method_decl,
        //     enum_decl,
        //     namespace_decl,
        //     using_decl,
        // };
        //
        // inline void from_json(const nlohmann::json& json, clang_ast_kind& value)
        // {
        //     static std::map<std::string_view, clang_ast_kind> value_map {
        //         {"CXXRecordDecl", clang_ast_kind::cxx_record_decl},
        //         {"CXXMethodDecl", clang_ast_kind::cxx_method_decl},
        //         {"EnumDecl", clang_ast_kind::enum_decl},
        //         {"NamespaceDecl", clang_ast_kind::namespace_decl},
        //         {"UsingDecl", clang_ast_kind::using_decl},
        //     };
        //     const auto it_value = value_map.find(json.get<std::string>());
        //     value = it_value != value_map.end() ? it_value->second : clang_ast_kind::none;
        // }
    }

    struct ast_parser_clang_process_raw : ast_parser
    {
        std::string clang;
        std::vector<std::string> args;

        explicit ast_parser_clang_process_raw(const codegen_options& options)
        {
            clang = options.clang;

            args.emplace_back("-xc++");
            args.emplace_back("-w");
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
            command += "-fsyntax-only -Xclang -ast-dump -";

            auto [result, out, err] = run(command, source);
            if (result != 0)
            {
                SPDLOG_ERROR("cannot preprocess macros, path={} result={} error={}", path, result, err);
                return false;
            }

            std::size_t relevant_index = out.find("__SPORE_CODEGEN__");
            out.erase(out.begin(), out.begin() + static_cast<std::ptrdiff_t>(relevant_index));

            write_file("c:/production/out.hpp.ast", out);

            return true;
        }
    };
}
