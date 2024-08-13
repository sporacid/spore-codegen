#pragma once

#include <algorithm>
#include <map>
#include <string>
#include <string_view>
#include <vector>

#include "fmt/format.h"
#include "spdlog/spdlog.h"
#include "clang-c/CXSourceLocation.h"
#include "clang-c/Index.h"

#include "spore/codegen/ast/parsers/ast_parser.hpp"
#include "spore/codegen/ast/parsers/ast_parser_utils.hpp"
#include "spore/codegen/misc/defer.hpp"
#include "spore/codegen/utils/files.hpp"
#include "spore/codegen/utils/strings.hpp"

namespace spore::codegen
{
    namespace detail
    {
        template <typename func_t>
        void visit_children(CXCursor cursor, func_t&& func)
        {
            using closure_t = std::tuple<func_t&&>;
            const auto visitor = [](CXCursor cursor, CXCursor parent, CXClientData data_ptr) -> CXChildVisitResult {
                auto&& [closure_func] = *static_cast<closure_t*>(data_ptr);
                return closure_func(cursor, parent);
            };

            closure_t closure {std::forward<func_t>(func)};
            clang_visitChildren(cursor, visitor, &closure);
        }

        template <typename value_t, typename func_t>
        void insert_end(std::vector<value_t>& values, func_t&& func)
        {
            std::size_t index = values.size();
            value_t value = func();
            values.insert(values.begin() + index, std::move(value));
        }

        void append_token(std::string& tokens, std::string_view token)
        {
            constexpr unsigned char no_whitespace_before[] {0, '>', ',', ';', '.'};
            constexpr unsigned char no_whitespace_after[] {0, '<', '.'};

            const unsigned char first = !token.empty() ? *token.begin() : 0;
            const unsigned char last = !tokens.empty() ? *tokens.rbegin() : 0;

            const bool no_whitespace =
                std::find(std::begin(no_whitespace_before), std::end(no_whitespace_before), first) != std::end(no_whitespace_before) ||
                std::find(std::begin(no_whitespace_after), std::end(no_whitespace_after), last) != std::end(no_whitespace_after);

            if (!no_whitespace)
            {
                tokens += " ";
            }

            tokens += token;
        }

        void add_access_flags(CXCursor cursor, ast_flags& flags)
        {
            CX_CXXAccessSpecifier access_spec = clang_getCXXAccessSpecifier(cursor);
            switch (access_spec)
            {
                case CX_CXXPublic:
                    flags = flags | ast_flags::public_;
                    break;

                case CX_CXXPrivate:
                    flags = flags | ast_flags::private_;
                    break;

                case CX_CXXProtected:
                    flags = flags | ast_flags::protected_;
                    break;

                default:
                    break;
            }
        }

        std::string get_string(CXString value)
        {
            defer _ = [&] { clang_disposeString(value); };
            const char* c_string = clang_getCString(value);
            return c_string != nullptr ? c_string : std::string();
        }

        std::string get_name(CXCursor cursor)
        {
            CXString string = clang_getCursorDisplayName(cursor);
            return get_string(string);
        }

        std::string get_spelling(CXCursor cursor)
        {
            CXString string = clang_getCursorSpelling(cursor);
            return get_string(string);
        }

        std::string get_spelling(CXType type)
        {
            CXString string = clang_getTypeSpelling(type);
            return get_string(string);
        }

        std::string get_spelling(CXTranslationUnit tu, CXToken token)
        {
            CXString string = clang_getTokenSpelling(tu, token);
            return get_string(string);
        }

        std::string get_type_spelling(CXCursor cursor)
        {
            CXType type = clang_getCursorType(cursor);
            return get_spelling(type);
        }

        std::string get_file(CXCursor cursor)
        {
            CXSourceLocation location = clang_getCursorLocation(cursor);
            CXFile file;
            clang_getFileLocation(location, &file, nullptr, nullptr, nullptr);
            return get_string(clang_getFileName(file));
        }

        std::string get_scope(CXCursor cursor)
        {
            constexpr std::string_view delimiter = "::";

            CXCursor parent = clang_getCursorSemanticParent(cursor);
            std::string scope;

            while (parent.kind != CXCursor_TranslationUnit)
            {
                if (!scope.empty() && !scope.starts_with(delimiter))
                {
                    scope.insert(0, delimiter);
                }

                scope.insert(0, get_name(parent));

                parent = clang_getCursorSemanticParent(parent);
            }

            if (scope.starts_with(delimiter))
            {
                scope.resize(scope.size() - delimiter.size());
            }

            return scope;
        }

        std::vector<CXToken> get_tokens(CXTranslationUnit tu, CXCursor cursor)
        {
            CXSourceRange extent = clang_getCursorExtent(cursor);

            CXToken* token_ptr = nullptr;
            unsigned token_count = 0;

            clang_tokenize(tu, extent, &token_ptr, &token_count);

            defer defer_dispose_tokens = [&] { clang_disposeTokens(tu, token_ptr, token_count); };

            std::vector<CXToken> tokens;
            tokens.reserve(token_count);

            std::copy(token_ptr, token_ptr + token_count, std::back_inserter(tokens));
            return tokens;
        }

        std::optional<std::string> get_default_value(CXTranslationUnit tu, const std::vector<CXToken>& tokens)
        {
            std::optional<std::string> default_value;

            const auto predicate_default_value = [&](CXToken token) {
                return clang_getTokenKind(token) == CXToken_Punctuation && get_spelling(tu, token) == "=";
            };

            const auto it_default_value = std::find_if(tokens.begin(), tokens.end(), predicate_default_value);
            if (it_default_value != tokens.end())
            {
                for (auto it_token = it_default_value + 1; it_token != tokens.end(); ++it_token)
                {
                    std::string spelling = get_spelling(tu, *it_token);

                    if (default_value.has_value())
                    {
                        append_token(default_value.value(), spelling);
                    }
                    else
                    {
                        default_value = std::move(spelling);
                    }
                }
            }

            return default_value;
        }

        std::optional<std::string> get_default_value(CXCursor cursor)
        {
            CXTranslationUnit tu = clang_Cursor_getTranslationUnit(cursor);
            std::vector<CXToken> tokens = get_tokens(tu, cursor);
            return get_default_value(tu, tokens);
        }

        nlohmann::json make_attributes(CXCursor cursor)
        {
            std::string attributes = get_spelling(cursor);

            nlohmann::json json;
            if (ast::parse_pairs(attributes, json))
            {
                return json;
            }

            SPDLOG_WARN("invalid attributes, file={} value={}", get_file(cursor), attributes);
            return nlohmann::json {};
        }

        ast_template_param make_template_param(CXCursor cursor, std::size_t index)
        {
            ast_template_param template_param;
            template_param.name = get_name(cursor);

            if (template_param.name.empty())
            {
                template_param.name = fmt::format("_t{}", index);
            }

            CXTranslationUnit tu = clang_Cursor_getTranslationUnit(cursor);
            std::vector<CXToken> tokens = get_tokens(tu, cursor);

            std::string* token_ptr = &template_param.type;
            for (CXToken token : tokens)
            {
                std::string spelling = get_spelling(tu, token);

                if (spelling == "...")
                {
                    template_param.is_variadic = true;
                }

                if (spelling == template_param.name)
                {
                    token_ptr = nullptr;
                }
                else if (spelling == "=")
                {
                    token_ptr = &template_param.default_value.emplace();
                }
                else if (token_ptr != nullptr)
                {
                    append_token(*token_ptr, spelling);
                }
            }

            strings::trim_end(template_param.type, ",");
            return template_param;
        }

        ast_argument make_argument(CXCursor cursor, std::size_t index)
        {
            ast_argument argument;
            argument.name = get_name(cursor);
            argument.default_value = get_default_value(cursor);
            argument.type.name = get_type_spelling(cursor);

            if (argument.name.empty())
            {
                argument.name = fmt::format("_arg{}", index);
            }

            const auto visitor = [&](CXCursor cursor, CXCursor parent) {
                switch (cursor.kind)
                {
                    case CXCursor_AnnotateAttr: {
                        argument.attributes = make_attributes(cursor);
                        break;
                    }

                    default: {
                        return CXChildVisit_Recurse;
                    }
                }

                return CXChildVisit_Continue;
            };

            visit_children(cursor, visitor);
            return argument;
        }

        ast_function make_function(CXCursor cursor)
        {
            ast_function function;
            function.name = get_spelling(cursor);
            function.scope = get_scope(cursor);

            CXType type = clang_getCursorType(cursor);
            CXType return_type = clang_getResultType(type);
            function.return_type.name = get_string(clang_getTypeSpelling(return_type));

            add_access_flags(cursor, function.flags);

            if (clang_CXXMethod_isConst(cursor))
            {
                function.flags = function.flags | ast_flags::const_;
            }

            if (clang_CXXMethod_isStatic(cursor))
            {
                function.flags = function.flags | ast_flags::static_;
            }

            if (clang_CXXMethod_isVirtual(cursor))
            {
                function.flags = function.flags | ast_flags::virtual_;
            }

            if (clang_CXXMethod_isDefaulted(cursor))
            {
                function.flags = function.flags | ast_flags::default_;
            }

            if (clang_CXXMethod_isDeleted(cursor))
            {
                function.flags = function.flags | ast_flags::delete_;
            }

            const auto visitor = [&](CXCursor cursor, CXCursor parent) {
                switch (cursor.kind)
                {
                    case CXCursor_ParmDecl: {
                        insert_end(function.arguments, [&] { return make_argument(cursor, function.template_params.size()); });
                        break;
                    }

                    case CXCursor_TemplateTypeParameter:
                    case CXCursor_TemplateTemplateParameter:
                    case CXCursor_NonTypeTemplateParameter: {
                        insert_end(function.template_params, [&] { return make_template_param(cursor, function.template_params.size()); });
                        break;
                    }

                    case CXCursor_AnnotateAttr: {
                        function.attributes = make_attributes(cursor);
                        break;
                    }

                    default: {
                        return CXChildVisit_Recurse;
                    }
                }

                return CXChildVisit_Continue;
            };

            visit_children(cursor, visitor);
            return function;
        }

        ast_constructor make_constructor(CXCursor cursor)
        {
            ast_function function = make_function(cursor);

            ast_constructor constructor;
            constructor.flags = function.flags;
            constructor.arguments = std::move(function.arguments);
            constructor.attributes = std::move(function.attributes);
            constructor.template_params = std::move(function.template_params);
            constructor.template_specialization_params = std::move(function.template_specialization_params);

            return constructor;
        }

        ast_enum_value make_enum_value(CXCursor cursor)
        {
            ast_enum_value enum_value;
            enum_value.name = get_name(cursor);
            enum_value.value = clang_getEnumConstantDeclValue(cursor);

            const auto visitor = [&](CXCursor cursor, CXCursor parent) {
                switch (cursor.kind)
                {
                    case CXCursor_AnnotateAttr: {
                        enum_value.attributes = make_attributes(cursor);
                        break;
                    }

                    default: {
                        return CXChildVisit_Recurse;
                    }
                }

                return CXChildVisit_Continue;
            };

            visit_children(cursor, visitor);
            return enum_value;
        }

        ast_enum make_enum(CXCursor cursor)
        {
            ast_enum enum_;
            enum_.name = get_name(cursor);
            enum_.scope = get_scope(cursor);
            enum_.type = clang_EnumDecl_isScoped(cursor) ? ast_enum_type::enum_class : ast_enum_type::enum_;

            CXCursor parent = clang_getCursorSemanticParent(cursor);
            enum_.nested = parent.kind != CXCursor_Namespace;

            CXType base_type = clang_getEnumDeclIntegerType(cursor);
            enum_.base.name = get_string(clang_getTypeSpelling(base_type));

            const auto visitor = [&](CXCursor cursor, CXCursor parent) {
                switch (cursor.kind)
                {
                    case CXCursor_EnumConstantDecl: {
                        insert_end(enum_.values, [&] { return make_enum_value(cursor); });
                        break;
                    }

                    case CXCursor_AnnotateAttr: {
                        enum_.attributes = make_attributes(cursor);
                        break;
                    }

                    default: {
                        return CXChildVisit_Recurse;
                    }
                }

                return CXChildVisit_Continue;
            };

            visit_children(cursor, visitor);
            return enum_;
        }

        ast_field make_field(CXCursor cursor)
        {
            ast_field field;
            field.name = get_name(cursor);
            field.default_value = get_default_value(cursor);
            field.type.name = get_type_spelling(cursor);

            add_access_flags(cursor, field.flags);

            const auto visitor = [&](CXCursor cursor, CXCursor parent) {
                switch (cursor.kind)
                {
                    case CXCursor_AnnotateAttr: {
                        field.attributes = make_attributes(cursor);
                        break;
                    }

                    default: {
                        return CXChildVisit_Recurse;
                    }
                }

                return CXChildVisit_Continue;
            };

            visit_children(cursor, visitor);
            return field;
        }

        ast_class make_class(CXCursor cursor, ast_file& file)
        {
            ast_class class_;
            class_.name = get_name(cursor);
            class_.scope = get_scope(cursor);

            CXCursor parent = clang_getCursorSemanticParent(cursor);
            class_.nested = parent.kind != CXCursor_Namespace;

            std::size_t depth = 0;
            for (std::size_t index_name = class_.name.size() - 1; index_name != std::numeric_limits<std::size_t>::max(); --index_name)
            {
                const char c = class_.name.at(index_name);

                if (c == '>')
                {
                    ++depth;
                }

                if (depth > 0)
                {
                    class_.name.erase(index_name);
                }

                if (c == '<')
                {
                    --depth;
                }

                if (depth == 0)
                {
                    break;
                }
            }

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

            const auto visitor = [&](CXCursor cursor, CXCursor parent) {
                switch (cursor.kind)
                {
                    case CXCursor_TemplateTypeParameter:
                    case CXCursor_TemplateTemplateParameter:
                    case CXCursor_NonTypeTemplateParameter: {
                        insert_end(class_.template_params, [&] { return make_template_param(cursor, class_.template_params.size()); });
                        break;
                    }

                    case CXCursor_AnnotateAttr: {
                        class_.attributes = make_attributes(cursor);
                        break;
                    }

                    case CXCursor_CXXBaseSpecifier: {
                        ast_ref& base = class_.bases.emplace_back();
                        base.name = get_name(cursor);
                        break;
                    }

                    case CXCursor_FieldDecl: {
                        insert_end(class_.fields, [&] { return make_field(cursor); });
                        break;
                    }

                    case CXCursor_Constructor: {
                        insert_end(class_.constructors, [&] { return make_constructor(cursor); });
                        break;
                    }

                    case CXCursor_FunctionDecl:
                    case CXCursor_FunctionTemplate:
                    case CXCursor_CXXMethod: {
                        insert_end(class_.functions, [&] { return make_function(cursor); });
                        break;
                    }

                    case CXCursor_StructDecl:
                    case CXCursor_ClassDecl:
                    case CXCursor_ClassTemplate: {
                        if (clang_isCursorDefinition(cursor))
                        {
                            insert_end(file.classes, [&] { return make_class(cursor, file); });
                        }

                        break;
                    }

                    case CXCursor_EnumDecl: {
                        if (clang_isCursorDefinition(cursor))
                        {
                            insert_end(file.enums, [&] { return make_enum(cursor); });
                        }

                        break;
                    }

                    default: {
                        return CXChildVisit_Recurse;
                    }
                }

                return CXChildVisit_Continue;
            };

            visit_children(cursor, visitor);
            return class_;
        }

        void make_file(CXCursor cursor, CXCursor parent, ast_file& file)
        {
            const auto visitor = [&](CXCursor cursor, CXCursor parent) {
                switch (cursor.kind)
                {
                    case CXCursor_ClassTemplate:
                    case CXCursor_ClassDecl:
                    case CXCursor_StructDecl: {
                        if (clang_isCursorDefinition(cursor))
                        {
                            insert_end(file.classes, [&] { return make_class(cursor, file); });
                        }

                        break;
                    }

                    case CXCursor_EnumDecl: {
                        if (clang_isCursorDefinition(cursor))
                        {
                            insert_end(file.enums, [&] { return make_enum(cursor); });
                        }

                        break;
                    }

                    case CXCursor_FunctionTemplate:
                    case CXCursor_FunctionDecl:
                    case CXCursor_CXXMethod: {
                        insert_end(file.functions, [&] { return make_function(cursor); });
                        break;
                    }

                    default: {
                        return CXChildVisit_Recurse;
                    }
                }

                return CXChildVisit_Continue;
            };

            if (visitor(cursor, parent) == CXChildVisit_Recurse)
            {
                visit_children(cursor, visitor);
            }
        }
    }

    struct ast_parser_clang : ast_parser
    {
        CXIndex clang_index;
        std::vector<std::string> args;
        std::vector<const char*> args_view;

        template <typename args_t>
        explicit ast_parser_clang(const args_t& additional_args)
        {
            clang_index = clang_createIndex(0, 0);

            args.emplace_back("-xc++");
            args.emplace_back("-E");
            args.emplace_back("-P");
            args.emplace_back("-dM");
            args.emplace_back("-w");
            args.emplace_back("-Wno-everything");

            const auto transformer = [](const auto& arg) { return std::string {arg}; };
            std::transform(std::begin(additional_args), std::end(additional_args), std::back_inserter(args), transformer);
            std::sort(args.begin(), args.end());
            args.erase(std::unique(args.begin(), args.end()), args.end());

            args_view.resize(args.size());

            std::transform(args.begin(), args.end(), args_view.begin(),
                [](const std::string& value) { return value.data(); });
        }

        ~ast_parser_clang() noexcept override
        {
            clang_disposeIndex(clang_index);
        }

        bool parse_files(const std::vector<std::string>& paths, std::vector<ast_file>& ast_files) override
        {
            constexpr auto flags = static_cast<CXTranslationUnit_Flags>(
                CXTranslationUnit_Incomplete |
                CXTranslationUnit_KeepGoing |
                CXTranslationUnit_SkipFunctionBodies);

            std::size_t file_index = 0;
            std::map<std::string_view, std::size_t> file_map;

            ast_files.clear();
            ast_files.reserve(paths.size());

            for (const std::string& path : paths)
            {
                ast_file ast_file = {.path = path};

                if (!files::read_file(path, ast_file.source))
                {
                    SPDLOG_ERROR("cannot read source file, path={}", path);
                    return false;
                }

                file_map.emplace(path, file_index++);
                ast_files.emplace_back(std::move(ast_file));
            }

            std::string main_source;
            for (const std::string& path : paths)
            {
                main_source += "#include \"";
                main_source += path;
                main_source += "\"\n";
            }

            std::vector<CXUnsavedFile> unsaved_files;
            unsaved_files.reserve(ast_files.size() + 1);

            for (std::size_t index = 0; index < paths.size(); ++index)
            {
                const std::string& source = ast_files[index].source;
                unsaved_files.emplace_back() = CXUnsavedFile {paths[index].data(), source.data(), static_cast<unsigned long>(source.size())};
            }

            constexpr std::string_view source_file = "__main.cpp";
            unsaved_files.emplace_back() = CXUnsavedFile {source_file.data(), main_source.data(), static_cast<unsigned long>(main_source.size())};

            CXTranslationUnit translation_unit = nullptr;
            CXErrorCode error = clang_parseTranslationUnit2(
                clang_index, source_file.data(),
                args_view.data(), static_cast<int>(args_view.size()),
                unsaved_files.data(), static_cast<unsigned long>(unsaved_files.size()),
                flags, &translation_unit);

            if (error != CXError_Success)
            {
                SPDLOG_ERROR("unable to parse translation unit, code={}", static_cast<int>(error));
                return false;
            }

            defer dispose_translation_unit = [&] { clang_disposeTranslationUnit(translation_unit); };

            std::size_t diagnostics_count = clang_getNumDiagnostics(translation_unit);
            if (diagnostics_count > 0)
            {
                SPDLOG_WARN("clang diagnostics --");

                for (std::size_t diagnostics_index = 0; diagnostics_index < diagnostics_count; ++diagnostics_index)
                {
                    CXDiagnostic diagnostic = clang_getDiagnostic(translation_unit, diagnostics_index);
                    defer dispose_diagnostic = [&] { clang_disposeDiagnostic(diagnostic); };
                    std::string diagnostic_str = "  " + detail::get_string(clang_formatDiagnostic(diagnostic, clang_defaultDiagnosticDisplayOptions()));
                    std::puts(diagnostic_str.data());
                }

                std::fflush(stdout);
            }

            const auto visitor = [&](CXCursor cursor, CXCursor parent) {
                const auto it_file = file_map.find(detail::get_file(cursor));
                if (it_file != file_map.end())
                {
                    std::size_t index = it_file->second;
                    detail::make_file(cursor, parent, ast_files[index]);
                }

                return CXChildVisit_Continue;
            };

            CXCursor cursor = clang_getTranslationUnitCursor(translation_unit);
            detail::visit_children(cursor, visitor);

            return true;
        }
    };
}
