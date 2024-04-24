#pragma once

#include <algorithm>
#include <functional>
#include <regex>
#include <string>
#include <string_view>
#include <vector>

#include "fmt/format.h"
#include "nlohmann/json.hpp"
#include "process.hpp"
#include "spdlog/spdlog.h"
#include "clang-c/CXSourceLocation.h"
#include "clang-c/Index.h"
#include "clang/Lex/Preprocessor.h"

#include "spore/codegen/ast/parsers/ast_parser.hpp"
#include "spore/codegen/codegen_helpers.hpp"
#include "spore/codegen/codegen_options.hpp"
#include "spore/codegen/misc/defer.hpp"

namespace spore::codegen
{
    namespace detail
    {
        template <typename value_t>
        struct clang_data
        {
            value_t& value;
            std::string& source;
            std::vector<std::string>& scopes;

            std::string full_scope() const
            {
                return join("::", scopes, std::identity());
            }
        };

        struct clang_visit_data
        {
            ast_file& file;
            std::vector<std::string> scopes;

            std::string join_scopes() const
            {
                return join("::", scopes, std::identity());
            }
        };

#if 0
        CXChildVisitResult visit_function(CXCursor cursor, CXCursor parent, CXClientData data)
        {
            // ast_file& file = *static_cast<ast_file*>(data);
            return CXChildVisit_Break;
        }

        CXChildVisitResult visit_field(CXCursor cursor, CXCursor parent, CXClientData data)
        {
            // ast_file& file = *static_cast<ast_file*>(data);
            return CXChildVisit_Break;
        }

        CXChildVisitResult visit_type(CXCursor cursor, CXCursor parent, CXClientData data)
        {
            // ast_file& file = *static_cast<ast_file*>(data);
            // clang_Type_visitFields()
            return CXChildVisit_Break;
        }

        CXChildVisitResult visit_namespace(CXCursor cursor, CXCursor parent, CXClientData data_ptr)
        {
            detail::clang_data<ast_file>& data = *static_cast<detail::clang_data<ast_file>*>(data_ptr);

            CXString scope = clang_getCursorDisplayName(cursor);
            defer dispose_scope = [&] { clang_disposeString(scope); };

            data.scopes.emplace_back(clang_getCString(scope));
            defer pop_back_scope = [&] { data.scopes.pop_back(); };

            switch (cursor.kind)
            {
                case CXCursor_Namespace: {
                    clang_visitChildren(cursor, &detail::visit_namespace, &data);
                    break;
                }

                case CXCursor_ClassDecl:
                case CXCursor_StructDecl: {
                    clang_visitChildren(cursor, &detail::visit_type, &data);
                    break;
                }

                case CXCursor_FunctionDecl: {
                    clang_visitChildren(cursor, &detail::visit_function, &data);
                    break;
                }

                default:
                    break;
            }

            return CXChildVisit_Break;
        }
#endif
        std::string to_string(CXString value)
        {
            defer _ = [&] { clang_disposeString(value); };
            return clang_getCString(value);
        }

        std::string get_name(CXCursor cursor)
        {
            CXString string = clang_getCursorDisplayName(cursor);
            return to_string(string);
        }

        void trim_chars(std::string_view& value, std::string_view chars)
        {
            std::size_t index = value.find_first_not_of(chars);
            value.remove_prefix(std::min(index, value.size()));
        }

        void trim_end_chars(std::string_view& value, std::string_view chars)
        {
            std::size_t index = value.find_last_not_of(chars);
            value = value.substr(0, std::max(index + 1, std::size_t {0}));
        }

        void make_attributes(std::string_view source, std::vector<ast_attribute>& attributes)
        {
            constexpr std::string_view prefix = "[[";
            constexpr std::string_view suffix = "]]";
            constexpr std::string_view colon = ":";
            constexpr std::string_view comma = ",";
            constexpr std::string_view using_stmt = "using";
            constexpr std::string_view whitespaces = " \t\r\n";

            trim_chars(source, whitespaces);

            if (source.starts_with(prefix))
            {
                source = std::string_view {source.begin() + prefix.size(), source.end()};
            }

            if (source.ends_with(suffix))
            {
                source = std::string_view {source.begin(), source.end() - suffix.size()};
            }

            trim_chars(source, whitespaces);

            std::string_view base_scope;
            if (source.starts_with(using_stmt))
            {
                source = std::string_view {source.begin() + using_stmt.size(), source.end()};
                trim_chars(source, whitespaces);

                std::size_t scope_index = source.find_first_of(colon);
                base_scope = source.substr(0, scope_index);

                std::size_t whitespace_index = base_scope.find_first_of(whitespaces);
                if (whitespace_index != std::string_view ::npos)
                {
                    base_scope = base_scope.substr(0, whitespace_index);
                }

                source = source.substr(scope_index + 1);
            }

            while (source.size() > 0)
            {
                constexpr std::string_view name_delimiter = "(,";
                constexpr std::string_view values_delimiter = ")";

                trim_chars(source, whitespaces);

                std::size_t index_name = source.find_first_of(name_delimiter);
                std::string_view name = source.substr(0, index_name);
                source = source.substr(index_name);

                constexpr std::string_view scope_delimiter = "::";

                std::string scope {base_scope};
                std::size_t index_scope = name.rfind(scope_delimiter);

                if (index_scope != std::string_view::npos)
                {
                    if (!scope.empty())
                    {
                        scope += scope_delimiter;
                    }

                    scope += name.substr(0, index_scope);
                    name = name.substr(index_scope + scope_delimiter.size());
                }

                std::map<std::string, std::string> value_map;
                std::string_view values;

                if (!source.empty() && source.at(0) == '(')
                {
                    std::size_t index_values = source.find(values_delimiter);
                    values = source.substr(1, index_values - 1);
                    source = source.substr(index_values + 1);
                }

                while (values.size() > 0)
                {
                    constexpr std::string_view key_delimiter = "=,";
                    constexpr std::string_view value_delimiter = ",";

                    trim_chars(values, whitespaces);

                    std::size_t index_key = values.find_first_of(key_delimiter);
                    std::string_view key = values.substr(0, index_key);
                    trim_end_chars(key, whitespaces);

                    std::string_view value = "true";
                    if (index_key == std::string_view::npos)
                    {
                        values = std::string_view {};
                    }
                    else if (values.at(index_key) == '=')
                    {
                        std::size_t index_value = values.find_first_of(value_delimiter);
                        if (index_value == std::string_view::npos)
                        {
                            value = values.substr(index_key + 1);
                            values = std::string_view {};
                        }
                        else
                        {
                            value = values.substr(index_key + 1, index_value - index_key - 1);
                            values = values.substr(index_value);
                        }

                        trim_chars(value, whitespaces);
                    }
                    else
                    {
                        values = values.substr(index_key);
                    }

                    value_map.emplace(key, value);
                    trim_chars(values, key_delimiter);
                }

                ast_attribute& attribute = attributes.emplace_back();
                attribute.scope = std::move(scope);
                attribute.name = std::string(name);
                attribute.values = std::move(value_map);

                trim_chars(source, comma);
            }
        }

        void make_attributes(CXCursor cursor, detail::clang_data<ast_file>& data, std::vector<ast_attribute>& attributes)
        {
            constexpr std::string_view attribute_begin = "[[";
            constexpr std::string_view attribute_end = "]]";
            constexpr std::string_view whitespaces = " \t\r\n";

            CXSourceLocation location = clang_getCursorLocation(cursor);

            CXFile file;
            std::uint32_t line;
            std::uint32_t column;
            std::uint32_t offset;

            clang_getFileLocation(location, &file, &line, &column, &offset);

            std::string_view preamble {data.source.data(), offset};

            trim_end_chars(preamble, whitespaces);

            while (preamble.ends_with(attribute_end))
            {
                std::size_t index_begin = preamble.rfind(attribute_begin);
                std::string_view attribute = preamble.substr(index_begin);

                make_attributes(attribute, attributes);

                preamble = preamble.substr(0, index_begin);
                trim_end_chars(preamble, whitespaces);
            }
        }

        ast_attribute make_attribute(CXCursor cursor, detail::clang_data<ast_file>& data)
        {
            ast_attribute attribute;
            attribute.name = get_name(cursor);
            attribute.scope = data.full_scope();
            SPDLOG_WARN("found attribute, name={}", attribute.full_name());
            return attribute;
        }

        ast_class make_class(CXCursor cursor, detail::clang_data<ast_file>& data)
        {
            ast_class class_;
            class_.name = get_name(cursor);
            class_.scope = data.full_scope();

            make_attributes(cursor, data, class_.attributes);

            if (clang_Cursor_hasAttrs(cursor))
            {
                SPDLOG_WARN("found attribute in class");
            }

            SPDLOG_WARN("found class, name={}", class_.full_name());

            const auto visitor = [](CXCursor cursor, CXCursor parent, CXClientData data_ptr) {
                detail::clang_data<ast_file>& data = *static_cast<detail::clang_data<ast_file>*>(data_ptr);

                if (clang_isAttribute(cursor.kind))
                {
                    SPDLOG_WARN("found attribute");
                }

                SPDLOG_WARN("found token in class, kind={}", to_string(clang_getCursorKindSpelling(cursor.kind)));
                switch (cursor.kind)
                {
                    case CXCursor_WarnUnusedAttr:
                    case CXCursor_UnexposedAttr: {
                        ast_attribute attribute = make_attribute(cursor, data);
                        break;
                    }

                    default: {
                        break;
                    }
                }

                return CXChildVisit_Continue;
            };

            clang_visitChildren(cursor, visitor, &data);

            return class_;
        }

        ast_field make_field(CXCursor cursor, detail::clang_data<ast_file>& data)
        {
            ast_field field;
            field.name = get_name(cursor);
            return field;
        }

        ast_function make_function(CXCursor cursor, detail::clang_data<ast_file>& data)
        {
            ast_function function;
            function.name = get_name(cursor);
            function.scope = data.full_scope();
            return function;
        }

        CXChildVisitResult visitor_impl(CXCursor cursor, CXCursor parent, CXClientData data_ptr)
        {
            detail::clang_data<ast_file>& data = *static_cast<detail::clang_data<ast_file>*>(data_ptr);

            if (clang_isAttribute(cursor.kind))
            {
                SPDLOG_WARN("found attribute");
            }

            SPDLOG_WARN("found token, kind={}", to_string(clang_getCursorKindSpelling(cursor.kind)));
            switch (cursor.kind)
            {
                case CXCursor_Namespace: {
                    CXString scope = clang_getCursorDisplayName(cursor);
                    defer dispose_scope = [&] { clang_disposeString(scope); };

                    data.scopes.emplace_back(clang_getCString(scope));
                    defer pop_back_scope = [&] { data.scopes.pop_back(); };

                    clang_visitChildren(cursor, &detail::visitor_impl, &data);
                    break;
                }

                case CXCursor_ClassTemplate: {
                    break;
                }

                case CXCursor_ClassDecl:
                case CXCursor_StructDecl: {
                    data.value.classes.emplace_back(make_class(cursor, data));
                    break;
                }

                case CXCursor_FunctionDecl: {
                    data.value.functions.emplace_back(make_function(cursor, data));
                    break;
                }

                case CXCursor_WarnUnusedAttr:
                case CXCursor_UnexposedAttr: {
                    ast_attribute attribute = make_attribute(cursor, data);
                    break;
                }

                default:
                    return CXChildVisit_Recurse;
            }

            return CXChildVisit_Continue;
        }

        std::tuple<std::int32_t, std::string, std::string> run_command(const std::string& command, const std::string& input = std::string {})
        {
            const bool has_stdin = not input.empty();
            const auto reader_factory = [](std::stringstream& stream) {
                return [&](const char* bytes, std::size_t n) {
                    stream.write(bytes, static_cast<std::streamsize>(n));
                };
            };

            std::stringstream _stdout;
            std::stringstream _stderr;

            TinyProcessLib::Process process {
                command,
                std::string(),
                reader_factory(_stdout),
                reader_factory(_stderr),
                has_stdin,
            };

            if (has_stdin)
            {
                process.write(input);
                process.close_stdin();
            }

            std::int32_t result = process.get_exit_status();
            return std::tuple {result, _stdout.str(), _stderr.str()};
        }
    }

    struct ast_parser_clang : ast_parser
    {
        std::map<CXCursorKind, std::function<void()>> handler_map;
        std::vector<std::string> args;
        std::vector<const char*> args_view;
        std::string clang;

        explicit ast_parser_clang(const codegen_options& options)
        {
            clang = options.clang;

            args.emplace_back("-xc++");
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

            args_view.resize(args.size());

            std::transform(args.begin(), args.end(), args_view.begin(),
                [](const std::string& value) { return value.data(); });
        }

        bool parse_file(const std::string& path, ast_file& file) override
        {
            constexpr auto flags =
                static_cast<CXTranslationUnit_Flags>(
                    CXTranslationUnit_Incomplete |
                    CXTranslationUnit_SkipFunctionBodies |
                    CXTranslationUnit_SingleFileParse |
                    CXTranslationUnit_KeepGoing |
                    CXTranslationUnit_VisitImplicitAttributes |
                    CXTranslationUnit_IncludeAttributedTypes);

            std::string macros;
            if (!preprocess_macros(path, macros))
            {
                SPDLOG_ERROR("unable to preprocess macros file, path={}", path);
                return false;
            }

            std::string source;
            if (!read_file(path, source))
            {
                SPDLOG_ERROR("unable to read source file, path={}", path);
                return false;
            }

            std::regex include_regex("#\\s*include .*");
            std::size_t include_index = std::sregex_iterator(source.begin(), source.end(), include_regex)->position();
            source = std::regex_replace(source, include_regex, "");
            source.insert(include_index, macros);

            if (!preprocess_source(source))
            {
                SPDLOG_ERROR("unable to read source file, path={}", path);
                return false;
            }

            CXUnsavedFile clang_file {path.data(), source.data(), static_cast<unsigned long>(source.size())};

            CXIndex index = clang_createIndex(0, 0);
            defer dispose_index = [&] { clang_disposeIndex(index); };

            CXTranslationUnit translation_unit = clang_parseTranslationUnit(
                index, path.data(), args_view.data(), static_cast<int>(args_view.size()), &clang_file, 1, flags);

            if (translation_unit == nullptr)
            {
                SPDLOG_ERROR("unable to parse translation unit, path={}", path);
                return false;
            }

            file.path = path;

            defer dispose_translation_unit = [&] { clang_disposeTranslationUnit(translation_unit); };
            CXCursor cursor = clang_getTranslationUnitCursor(translation_unit);

            const auto visitor = [](CXCursor cursor, CXCursor parent, CXClientData data_ptr) {
                detail::clang_data<ast_file>& data = *static_cast<detail::clang_data<ast_file>*>(data_ptr);
                if (clang_isAttribute(cursor.kind))
                {
                    SPDLOG_WARN("found attribute");
                }

                if (clang_Cursor_hasAttrs(cursor))
                {
                    SPDLOG_WARN("found attribute container");
                }

                switch (cursor.kind)
                {
                    case CXCursor_ClassTemplate: {
                        break;
                    }

                    case CXCursor_ClassDecl:
                    case CXCursor_StructDecl: {
                        SPDLOG_WARN("found class, name={}", detail::get_name(cursor));
                        CXSourceLocation location = clang_getCursorLocation(cursor);

                        CXFile file;
                        std::uint32_t line;
                        std::uint32_t column;
                        std::uint32_t offset;

                        clang_getFileLocation(location, &file, &line, &column, &offset);

                        std::string preamble {data.source.data(), offset};
                        //                        std::regex attribute_regex {R"((\[\[.*\]\])+\s*)"};

                        std::regex attribute_regex {R"(\s*(\]\].*\[\[)+)"};

                        std::match_results<std::string::const_reverse_iterator> matches;
                        if (std::regex_search(preamble.crbegin(), preamble.crend(), matches, attribute_regex))
                        {
                            for (std::size_t index = 1; index < matches.size(); ++index)
                            {
                                std::string match = matches[index].str();
                                std::reverse(match.begin(), match.end());
                                SPDLOG_WARN("match: {}", match);
                            }
                        }

                        //                        using svregex_iterator = std::regex_iterator<std::string_view>;
                        //                        for (auto it = std::sregex_iterator(preamble.rbegin(), preamble.rend(), attribute_regex); it != std::sregex_iterator(); ++it)
                        //                        {
                        //                            SPDLOG_WARN("match: {}", it->str());
                        //                        }

                        CXString* maybe_str1 = (CXString*) location.ptr_data[0];
                        CXString* maybe_str2 = (CXString*) location.ptr_data[1];
                        CXCursor lexical_parent = clang_getCursorLexicalParent(cursor);
                        CXCursor semantic_parent = clang_getCursorSemanticParent(cursor);
                        SPDLOG_WARN("");
                        break;
                    }

                    default:
                        break;
                }

                return CXChildVisit_Recurse;
            };

            std::vector<std::string> scopes;
            detail::clang_data<ast_file> data {file, source, scopes};
            // clang_visitChildren(cursor, visitor, &data);
            clang_visitChildren(cursor, &detail::visitor_impl, &data);

            return true;
        }

      private:
        bool preprocess_macros(const std::string& path, std::string& macros) const
        {
            constexpr std::size_t command_size = 1024;

            std::initializer_list<std::string_view> additional_args {
                "-E",
                "-dM",
                "-Wno-everything",
            };

            std::string command;
            command.reserve(command_size);

            command += clang + " ";
            command += join(" ", args, std::identity()) + " ";
            command += join(" ", additional_args, std::identity()) + " ";
            command += path;

            auto [result, out, err] = detail::run_command(command);
            if (result != 0)
            {
                SPDLOG_ERROR("cannot preprocess macros, path={} result={}", path, result);
                return false;
            }

            macros = std::move(out);
            return true;
        }

        bool preprocess_source(std::string& source) const
        {
            constexpr std::size_t command_size = 1024;

            std::initializer_list<std::string_view> additional_args {
                "-E",
                "-P",
                "-Wno-everything",
                "-",
            };

            std::string command;
            command.reserve(command_size);
            command += clang + " ";
            command += join(" ", args, std::identity()) + " ";
            command += join(" ", additional_args, std::identity());

            auto [result, out, err] = detail::run_command(command, source);
            if (result != 0)
            {
                SPDLOG_ERROR("cannot preprocess source, result={}", result);
                return false;
            }

            source = std::move(out);
            return true;
        }

#if 0
        bool preprocess_file(const std::string& path, const std::vector<std::string>& args, std::string& preprocessed)
        {
            std::initializer_list<std::string_view> preprocessor_args {
                "-E",
                "-P",
                "-Wno-everything",
                "-fkeep-system-includes",
                // "-nostdinc",
                // "-nostdinc++",
            };

            constexpr std::size_t command_size = 1024;

            std::string command;
            command.reserve(command_size);
            command += clang + " "; // TODO @sporacid move to command line interface
            command += join(" ", preprocessor_args, std::identity()) + " ";
            command += join(" ", args, std::identity()) + " ";
            command += path;

            SPDLOG_WARN("{}", command);

            auto [result, out, err] = detail::run_command(command);
            if (result != 0)
            {
                SPDLOG_ERROR("cannot preprocess file, path={} result={}", path, result);
                return false;
            }

            preprocessed = std::move(out);
            return true;
        }
#endif
    };
}

#if 0
namespace spore::codegen
{
    namespace details
    {
        struct clang_namespace
        {
            // TODO
        };

        struct clang_type
        {
            std::string_view qualified_type;
        };

        void from_json(const nlohmann::json& json, clang_type& value)
        {
            json["qualType"].get_to(value.qualified_type);
        }

        struct clang_typedef_decl
        {
            clang_type type;
        };

        void from_json(const nlohmann::json& json, clang_typedef_decl& value)
        {
            json["type"].get_to(value.type);
        }

        using clang_data = std::variant<
            std::monostate,
            clang_namespace,
            clang_typedef_decl>;

        void from_json(const nlohmann::json& json, clang_data& value)
        {
            static const std::map<std::string_view, std::function<void(const nlohmann::json&, clang_data&)>> from_json_map {
                {"TypedefDecl", [](const nlohmann::json& json, clang_data& value) { from_json(json, value.emplace<clang_typedef_decl>()); }},
            };

            const auto from_json_it = from_json_map.find(json["kind"]);
            if (from_json_it != from_json_map.end())
            {
                from_json_it->second(json, value);
            }
            else
            {
                value = std::monostate();
            }
        }

        struct clang_location
        {
            std::optional<std::string_view> file;
            std::uint64_t offset = 0;
            std::uint32_t line = 0;
            std::uint32_t column = 0;
            std::uint32_t token_length = 0;
        };

        void from_json(const nlohmann::json& json, clang_location& value)
        {
            if (json.contains("file"))
            {
                json["file"].get_to(value.file.emplace());
            }

            if (json.contains("offset"))
            {
                json["offset"].get_to(value.offset);
            }

            if (json.contains("line"))
            {
                json["line"].get_to(value.line);
            }

            if (json.contains("col"))
            {
                json["col"].get_to(value.column);
            }

            if (json.contains("tokLen"))
            {
                json["tokLen"].get_to(value.token_length);
            }
        }

        struct clang_range
        {
            clang_location begin;
            clang_location end;
        };

        void from_json(const nlohmann::json& json, clang_range& value)
        {
            if (json.contains("begin"))
            {
                json["begin"].get_to(value.begin);
            }

            if (json.contains("end"))
            {
                json["end"].get_to(value.end);
            }
        }

        struct clang_token
        {
            std::uint64_t id = 0;
            std::string_view kind;
            std::optional<clang_location> location;
            std::optional<clang_range> range;
            std::vector<clang_token> inners;
            clang_data data;
        };

        void from_json(const nlohmann::json& json, clang_token& value)
        {
            std::string_view id = json["id"];
            if (id.starts_with("0x"))
            {
                id = std::string_view {id.data() + 2, id.size() - 2};
            }

            value.id = std::stoull(id.data(), nullptr, 16);
            json["kind"].get_to(value.kind);

            if (json.contains("loc"))
            {
                json["loc"].get_to(value.location.emplace());
            }

            if (json.contains("range"))
            {
                json["range"].get_to(value.range.emplace());
            }

            if (json.contains("inner"))
            {
                json["inner"].get_to(value.inners);
            }

            json.get_to(value.data);
        }
    }

    struct ast_parser_clang : ast_parser
    {
        std::string clang_args;

        explicit ast_parser_clang(const codegen_options& options)
        {
            //            for (const std::string& include : options.includes)
            //            {
            //                clang_args += fmt::format("-I{} ", include);
            //            }

            for (const std::pair<std::string, std::string>& definition : options.definitions)
            {
                clang_args += fmt::format("-D{}={} ", definition.first, definition.second);
            }

            if (clang_args.ends_with(" "))
            {
                clang_args.resize(clang_args.size() - 1);
            }
        }

        bool parse_file(const std::string& path, ast_file& file) override
        {
            std::string command = fmt::format("clang++ {} -Xclang -ast-dump=json -fsyntax-only {}", clang_args, path);

            std::stringstream _stdout;
            std::stringstream _stderr;

            const auto reader_factory = [](std::stringstream& stream) {
                return [&](const char* bytes, std::size_t n) {
                    stream.write(bytes, static_cast<std::streamsize>(n));
                };
            };

            TinyProcessLib::Process process {
                command,
                std::string(),
                reader_factory(_stdout),
                reader_factory(_stderr),
            };

            std::int32_t result = process.get_exit_status();
            if (result != 0)
            {
                SPDLOG_DEBUG("clang++ has returned with non-zero status, value={} file={} stderr={}", result, path, _stderr.str());
                // return false;
            }

            details::clang_token clang_root;
            nlohmann::json json_clang_root = nlohmann::json::parse(_stdout.str());
            json_clang_root.get_to(clang_root);

            return false;
        }
#if 0
        CXChildVisitResult visit_clang_children(CXCursor current_cursor, CXCursor parent, CXClientData client_data)
        {
            CXString current_display_name = clang_getCursorDisplayName(current_cursor);

            SPDLOG_DEBUG("Visiting element {}", clang_getCString(current_display_name));

            clang_disposeString(current_display_name);

            return CXChildVisit_Recurse;
        }

        bool parse_file(const std::string& path, ast_file& file) override
        {
            CXIndex index = clang_createIndex(0, 0);
            CXTranslationUnit unit = clang_parseTranslationUnit(
                index,
                path.c_str(),
                nullptr,
                0,
                nullptr,
                0,
                CXTranslationUnit_None);

            if (unit == nullptr)
            {
                SPDLOG_ERROR("Unable to parse translation unit");
                return false;
            }

            CXCursor cursor = clang_getTranslationUnitCursor(unit);
            clang_visitChildren(
                cursor,
                [](CXCursor current_cursor, CXCursor parent, CXClientData client_data) {
                    CXString current_display_name = clang_getCursorDisplayName(current_cursor);
                    // Allocate a CXString representing the name of the current cursor

                    std::cout << "Visiting element " << clang_getCString(current_display_name) << "\n";
                    // Print the char* value of current_display_name

                    clang_disposeString(current_display_name);
                    // Since clang_getCursorDisplayName allocates a new CXString, it must be freed. This applies
                    // to all functions returning a CXString

                    return CXChildVisit_Recurse;
                },
                nullptr // client_data
            );
            return false;
        }
#endif
    };
}
#endif