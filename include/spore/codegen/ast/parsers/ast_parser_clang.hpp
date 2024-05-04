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
#include "clang/Lex/Lexer.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Parse/ParseAST.h"

#include "spore/codegen/ast/parsers/ast_parser.hpp"
#include "spore/codegen/codegen_helpers.hpp"
#include "spore/codegen/codegen_options.hpp"
#include "spore/codegen/misc/defer.hpp"

namespace spore::codegen
{
    namespace detail
    {
        constexpr std::string_view whitespaces = " \t\r\n";

        template <typename value_t>
        struct clang_data
        {
            value_t& value;
            std::string& source;
            std::vector<std::string>& scopes;

            [[nodiscard]] std::string full_scope() const
            {
                return join("::", scopes, std::identity());
            }
        };

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

        std::string_view rfind_template(std::string_view preamble)
        {
            constexpr std::string_view chevrons = "<>";

            std::string_view template_;

            std::size_t chevron_count = 0;
            std::size_t chevron_index = preamble.find_last_of(chevrons);

            std::size_t index_template_end = chevron_index;
            std::size_t index_template_begin = std::string_view::npos;

            while (chevron_index != std::string_view::npos && index_template_begin == std::string_view::npos)
            {
                std::size_t chevron_diff = preamble.at(chevron_index) == '>' ? 1 : -1;
                chevron_count += chevron_diff;

                if (chevron_count == 0)
                {
                    index_template_begin = chevron_index;
                }
                else
                {
                    chevron_index = preamble.find_last_of(chevrons, chevron_index - 1);
                }
            }

            if (index_template_begin != std::string_view::npos)
            {
                template_ = preamble.substr(index_template_begin, index_template_end - index_template_begin + 1);
            }

            return template_;
        }

        std::string_view rfind_type(std::string_view preamble)
        {
            constexpr std::string_view delimiters = "[]{}:;,";

            trim_end_chars(preamble, whitespaces);

            std::size_t find_begin_offset = preamble.size();

            if (preamble.at(preamble.size() - 1) == '>')
            {
                std::string_view template_ = rfind_template(preamble);
                find_begin_offset -= template_.size();
            }

            std::size_t index_begin = preamble.find_last_of(delimiters, find_begin_offset);

            while (index_begin > 0 && preamble.at(index_begin) == ':' && preamble.at(index_begin - 1) == ':')
            {
                index_begin = preamble.find_last_of(delimiters, index_begin - 2);
            }

            std::string_view type {preamble.data() + index_begin + 1, preamble.data() + preamble.size()};
            trim_chars(type, whitespaces);
            trim_end_chars(type, whitespaces);

            return type;
        }

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

        std::string_view get_preamble(CXCursor cursor, std::string_view source)
        {
            CXSourceLocation location = clang_getCursorLocation(cursor);

            CXFile file;
            std::uint32_t line;
            std::uint32_t column;
            std::uint32_t offset;

            clang_getFileLocation(location, &file, &line, &column, &offset);

            return std::string_view {source.data(), offset};
        }

        void make_attributes(std::string_view source, std::vector<ast_attribute>& attributes)
        {
            constexpr std::string_view prefix = "[[";
            constexpr std::string_view suffix = "]]";
            constexpr std::string_view colon = ":";
            constexpr std::string_view comma = ",";
            constexpr std::string_view using_stmt = "using";

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

            while (!source.empty())
            {
                constexpr std::string_view name_delimiter = "(,";
                constexpr std::string_view values_delimiter = ")";

                trim_chars(source, whitespaces);

                std::size_t index_name = source.find_first_of(name_delimiter);
                std::string_view name = source.substr(0, index_name);
                source = source.substr(index_name != std::string_view::npos ? index_name : source.size());

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

                while (!values.empty())
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

            std::string_view preamble = get_preamble(cursor, data.source);

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

        ast_field make_field(CXCursor cursor, detail::clang_data<ast_file>& data)
        {
            std::string_view preamble = get_preamble(cursor, data.source);

            ast_field field;
            field.name = get_name(cursor);
            field.type.name = rfind_type(preamble);

            make_attributes(cursor, data, field.attributes);

            CX_CXXAccessSpecifier access_spec = clang_getCXXAccessSpecifier(cursor);
            switch (access_spec)
            {
                case CX_CXXPublic:
                    field.flags = field.flags | ast_flags::public_;
                    break;

                case CX_CXXPrivate:
                    field.flags = field.flags | ast_flags::private_;
                    break;

                case CX_CXXProtected:
                    field.flags = field.flags | ast_flags::protected_;
                    break;

                default:
                    break;
            }

            return field;
        }

        ast_function make_function(CXCursor cursor, detail::clang_data<ast_file>& data)
        {
            ast_function function;
            function.name = get_name(cursor);
            function.scope = data.full_scope();
            return function;
        }

        ast_class make_class_v2(CXCursor cursor, detail::clang_data<ast_file>& data)
        {
            ast_class class_;
            class_.name = get_name(cursor);
            class_.scope = data.full_scope();

            make_attributes(cursor, data, class_.attributes);

            std::string_view name_template = rfind_template(class_.name);
            class_.name.resize(class_.name.size() - name_template.size());

            CXCursorKind cursor_kind = cursor.kind;
            if (cursor_kind == CXCursor_ClassTemplate)
            {
                cursor_kind = clang_getTemplateCursorKind(cursor);
            }

            switch (cursor_kind)
            {
                case CXCursor_StructDecl: {
                    class_.type = ast_class_type::struct_;
                    break;
                }

                case CXCursor_ClassDecl: {
                    class_.type = ast_class_type::class_;
                    break;
                }

                default: {
                    break;
                }
            }

            using closure_t = std::tuple<ast_class&, detail::clang_data<ast_file>&>;
            const auto visitor = [](CXCursor cursor, CXCursor parent, CXClientData data_ptr) {
                auto& [closure_class, closure_data] = *static_cast<closure_t*>(data_ptr);
                switch (cursor.kind)
                {
                    case CXCursor_FieldDecl: {
                        closure_class.fields.emplace_back(make_field(cursor, closure_data));
                        break;
                    }

                    case CXCursor_CXXBaseSpecifier: {
                        ast_ref& base = closure_class.bases.emplace_back();
                        base.name = get_name(cursor);
                        break;
                    }

                    case CXCursor_TemplateTypeParameter: {
                        ast_template_param& template_param = closure_class.template_params.emplace_back();
                        template_param.type = "typename";
                        template_param.name = get_name(cursor);
                        break;
                    }

                    case CXCursor_NonTypeTemplateParameter: {
                        std::string_view preamble = get_preamble(cursor, closure_data.source);
                        ast_template_param& template_param = closure_class.template_params.emplace_back();
                        template_param.type = rfind_type(preamble);
                        template_param.name = get_name(cursor);
                        break;
                    }

                    case CXCursor_FunctionDecl:
                    case CXCursor_FunctionTemplate: {
                        break;
                    }

                    default: {
                        return CXChildVisit_Recurse;
                    }
                }

                return CXChildVisit_Continue;
            };

            closure_t closure {class_, data};
            clang_visitChildren(cursor, visitor, &closure);

            return class_;
        }

        ast_class make_class(CXCursor cursor, detail::clang_data<ast_file>& data)
        {
            ast_class class_;
            class_.name = get_name(cursor);
            class_.scope = data.full_scope();

            make_attributes(cursor, data, class_.attributes);

            switch (cursor.kind)
            {
                case CXCursor_StructDecl: {
                    class_.type = ast_class_type::struct_;
                    break;
                }

                case CXCursor_ClassDecl: {
                    class_.type = ast_class_type::class_;
                    break;
                }

                default: {
                    break;
                }
            }

            using closure_t = std::tuple<ast_class&, detail::clang_data<ast_file>&>;
            const auto visitor = [](CXCursor cursor, CXCursor parent, CXClientData data_ptr) {
                auto& [closure_class, closure_data] = *static_cast<closure_t*>(data_ptr);
                switch (cursor.kind)
                {
                    case CXCursor_FieldDecl: {
                        closure_class.fields.emplace_back(make_field(cursor, closure_data));
                        break;
                    }

                    case CXCursor_CXXBaseSpecifier: {
                        ast_ref& base = closure_class.bases.emplace_back();
                        base.name = get_name(cursor);
                        break;
                    }

                    default: {
                        return CXChildVisit_Recurse;
                    }
                }

                return CXChildVisit_Continue;
            };

            closure_t closure {class_, data};
            clang_visitChildren(cursor, visitor, &closure);

            return class_;
        }

        ast_class make_class_template(CXCursor cursor, detail::clang_data<ast_file>& data)
        {
            ast_class class_;
            std::vector<ast_template_param> params;

            using closure_t = std::tuple<ast_class&, std::vector<ast_template_param>&, detail::clang_data<ast_file>&>;
            const auto visitor = [](CXCursor cursor, CXCursor parent, CXClientData data_ptr) {
                auto& [closure_class, closure_params, closure_data] = *static_cast<closure_t*>(data_ptr);
                switch (cursor.kind)
                {
                    case CXCursor_ClassDecl:
                    case CXCursor_StructDecl: {
                        closure_class = make_class(cursor, closure_data);
                        std::string_view name_template = rfind_template(closure_class.name);
                        closure_class.name.resize(closure_class.name.size() - name_template.size());
                        break;
                    }

                    case CXCursor_TemplateTypeParameter: {
                        ast_template_param& template_param = closure_params.emplace_back();
                        template_param.type = "typename";
                        template_param.name = get_name(cursor);
                        break;
                    }

                    case CXCursor_NonTypeTemplateParameter: {
                        std::string_view preamble = get_preamble(cursor, closure_data.source);
                        ast_template_param& template_param = closure_params.emplace_back();
                        template_param.type = rfind_type(preamble);
                        template_param.name = get_name(cursor);
                        break;
                    }

                    default: {
                        break;
                    }
                }

                return CXChildVisit_Continue;
            };

            closure_t closure {class_, params, data};
            clang_visitChildren(cursor, visitor, &closure);

            class_.template_params = std::move(params);
            return class_;
        }

        CXChildVisitResult visitor_impl(CXCursor cursor, CXCursor, CXClientData data_ptr)
        {
            detail::clang_data<ast_file>& data = *static_cast<detail::clang_data<ast_file>*>(data_ptr);

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

                case CXCursor_ClassTemplate:
                case CXCursor_ClassDecl:
                case CXCursor_StructDecl: {
                    // TODO handle inner classes
                    data.value.classes.emplace_back(make_class_v2(cursor, data));
                    break;
                }

                    //                case CXCursor_ClassTemplate: {
                    //                    data.value.classes.emplace_back(make_class_template(cursor, data));
                    //                    break;
                    //                }
                    //
                    //                case CXCursor_ClassDecl:
                    //                case CXCursor_StructDecl: {
                    //                    data.value.classes.emplace_back(make_class(cursor, data));
                    //                    break;
                    //                }

                case CXCursor_FunctionDecl: {
                    data.value.functions.emplace_back(make_function(cursor, data));
                    return CXChildVisit_Continue;
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
        CXIndex clang_index;

        explicit ast_parser_clang(const codegen_options& options)
        {
            clang = options.clang;

            args.emplace_back("-xc++");
            args.emplace_back("-w");
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

            args_view.resize(args.size());

            std::transform(args.begin(), args.end(), args_view.begin(),
                [](const std::string& value) { return value.data(); });

            clang_index = clang_createIndex(0, 0);
        }

        ~ast_parser_clang() noexcept override
        {
            clang_disposeIndex(clang_index);
        }

        bool parse_file(const std::string& path, ast_file& file) override
        {
            constexpr auto flags = CXTranslationUnit_None;
           // constexpr auto flags =
           //     static_cast<CXTranslationUnit_Flags>(
           //         CXTranslationUnit_Incomplete |
           //         CXTranslationUnit_SkipFunctionBodies |
           //         // CXTranslationUnit_PrecompiledPreamble |
           //         // CXTranslationUnit_CreatePreambleOnFirstParse |
           //         CXTranslationUnit_SingleFileParse |
           //         CXTranslationUnit_KeepGoing |
           //         CXTranslationUnit_IgnoreNonErrorsFromIncludedFiles);

#if 0
            std::string macros;
            if (!preprocess_macros(path, macros))
            {
                return false;
            }

            std::string source;
            if (!read_file(path, source))
            {
                SPDLOG_ERROR("cannot read source file, path={}", path);
                return false;
            }

            std::regex include_regex("#\\s*include .*");
            std::sregex_iterator include_it {source.begin(), source.end(), include_regex};
            std::size_t include_index = include_it != std::sregex_iterator() ? include_it->position() : 0;

            source = std::regex_replace(source, include_regex, "");
            source.insert(include_index, macros);

            if (!preprocess_source(source))
            {
                return false;
            }
#endif

            //  std::string source;
            //  if (!read_file(path, source))
            //  {
            //      SPDLOG_ERROR("cannot read source file, path={}", path);
            //      return false;
            //  }
//
            //  if (!preprocess_source(source))
            //  {
            //      return false;
            //  }
//
            //  CXUnsavedFile clang_file {path.data(), source.data(), static_cast<unsigned long>(source.size())};

            // CXIndex index = clang_createIndex(0, 0);
            // defer dispose_index = [&] { clang_disposeIndex(index); };

            CXTranslationUnit translation_unit = nullptr;
            CXErrorCode error = clang_parseTranslationUnit2(
                clang_index, path.data(), args_view.data(), static_cast<int>(args_view.size()), nullptr, 0, flags, &translation_unit);

            // CXTranslationUnit translation_unit = clang_createTranslationUnitFromSourceFile(
            //     clang_index, path.data(), static_cast<int>(args_view.size()), args_view.data(), 0, nullptr);

            if (translation_unit == nullptr)
            {
                SPDLOG_ERROR("unable to parse translation unit, path={}", path);
                return false;
            }

            defer dispose_translation_unit = [&] { clang_disposeTranslationUnit(translation_unit); };
            CXCursor cursor = clang_getTranslationUnitCursor(translation_unit);

            file.path = path;

            // std::vector<std::string> scopes;
            // detail::clang_data<ast_file> data {file, source, scopes};
            // clang_visitChildren(cursor, &detail::visitor_impl, &data);

            std::size_t indent = 0;
            CXCursorVisitor visitor;

            using closure_t = std::tuple<std::size_t&, CXCursorVisitor>;
            visitor = [](CXCursor cursor, CXCursor parent, CXClientData data_ptr) {
                if (clang_Location_isFromMainFile(clang_getCursorLocation(cursor)))
                {
                    auto& [indent_, visitor_] = *static_cast<closure_t*>(data_ptr);
                    for (std::size_t index = 0; index < indent_; ++index)
                        std::cout << "  ";
                    std::cout << detail::to_string(clang_getCursorKindSpelling(cursor.kind)) << ": " << detail::get_name(cursor) << std::endl;
                    ++indent_;
                    clang_visitChildren(cursor, visitor_, data_ptr);
                    --indent_;
                }

                return CXChildVisit_Continue;
            };

            closure_t closure {indent, visitor};
            std::cout << "FILE " << path << std::endl;
            std::cout << "-------------------" << std::endl;
            clang_visitChildren(cursor, visitor, &closure);
            std::cout << "-------------------" << std::endl;

            //  std::shared_ptr<clang::PreprocessorOptions> PPOpts;
            //
            //  clang::FileManager FileMg;
            //  clang::DiagnosticsEngine diags(
            //      clang::IntrusiveRefCntPtr<clang::DiagnosticIDs>(),
            //      clang::IntrusiveRefCntPtr<clang::DiagnosticOptions>());
            //  clang::LangOptions opts;
            //  clang::SourceManager SM(diags, FileMg);
            //  clang::HeaderSearch Headers;
            //  clang::ModuleLoader TheModuleLoader;
            //
            //  clang::Preprocessor pp;
            //  clang::ASTConsumer consumer;
            //  clang::ASTContext ctx;
            // clang::ParseAST(pp, &consumer, ctx, false, clang::TranslationUnitKind::TU_Prefix);

            return true;
        }

      private:
        bool preprocess_macros(const std::string& path, std::string& macros) const
        {
            constexpr std::size_t command_size = 1024;

            std::initializer_list<std::string_view> additional_args {
                "-E",
                "-dM",
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
                SPDLOG_ERROR("cannot preprocess macros, path={} result={} error={}", path, result, err);
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
                SPDLOG_ERROR("cannot preprocess source, result={} error={}", result, err);
                return false;
            }

            source = std::move(out);
            return true;
        }
    };
}
