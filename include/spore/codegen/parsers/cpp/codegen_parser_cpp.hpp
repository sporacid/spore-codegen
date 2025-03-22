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

#include "spore/codegen/misc/defer.hpp"
#include "spore/codegen/parsers/codegen_parser.hpp"
#include "spore/codegen/parsers/cpp/ast/cpp_file.hpp"
#include "spore/codegen/parsers/cpp/codegen_utils_cpp.hpp"
#include "spore/codegen/utils/files.hpp"
#include "spore/codegen/utils/strings.hpp"

namespace spore::codegen
{
    namespace detail
    {
        constexpr std::string_view ellipsis = "...";
        constexpr std::string_view valid_name_chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_$";

        template <typename func_t>
        void visit_children(const CXCursor& cursor, func_t&& func)
        {
            using closure_t = std::tuple<func_t&&>;
            const CXCursorVisitor visitor = [](CXCursor v_cursor, CXCursor parent, CXClientData data_ptr) -> CXChildVisitResult {
                auto&& [closure_func] = *static_cast<closure_t*>(data_ptr);
                return closure_func(v_cursor, parent);
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

        inline void append_token(std::string& tokens, const std::string_view token)
        {
            constexpr unsigned char no_whitespace_before[] {0, '>', ',', ';', '.'};
            constexpr unsigned char no_whitespace_after[] {0, '<', '.'};

            const unsigned char first = !token.empty() ? *token.begin() : 0;
            const unsigned char last = !tokens.empty() ? *tokens.rbegin() : 0;

            const bool no_whitespace =
                std::ranges::find(no_whitespace_before, first) != std::end(no_whitespace_before) ||
                std::ranges::find(no_whitespace_after, last) != std::end(no_whitespace_after);

            if (!no_whitespace)
            {
                tokens += " ";
            }

            tokens += token;
        }

        inline void erase_template(std::string& name, std::vector<std::string>* template_params = nullptr)
        {
            std::size_t depth = 0;
            std::string template_param;

            const auto add_template_param = [&] {
                if (template_params != nullptr)
                {
                    strings::trim(template_param);
                    template_params->emplace(template_params->begin(), std::move(template_param));
                    template_param.clear();
                }
            };

            const auto add_template_param_char = [&](const char c) {
                if (template_params != nullptr)
                {
                    template_param.insert(0, 1, c);
                }
            };

            for (std::size_t index = name.size() - 1; index != std::numeric_limits<std::size_t>::max(); --index)
            {
                switch (const char c = name.at(index))
                {
                    case '<': {
                        if (--depth > 0)
                        {
                            add_template_param_char(c);
                        }
                        else
                        {
                            add_template_param();
                            name.erase(index);
                        }

                        break;
                    }

                    case '>': {
                        if (++depth > 1)
                        {
                            add_template_param_char(c);
                        }

                        break;
                    }

                    case ',': {
                        if (depth == 1)
                        {
                            add_template_param();
                        }
                        else if (depth > 1)
                        {
                            add_template_param_char(c);
                        }

                        break;
                    }

                    default: {
                        if (depth > 0)
                        {
                            add_template_param_char(c);
                        }

                        break;
                    }
                }

                if (depth == 0)
                {
                    break;
                }
            }
        }

        inline void add_access_flags(const CXCursor& cursor, cpp_flags& flags)
        {
            const CX_CXXAccessSpecifier access_spec = clang_getCXXAccessSpecifier(cursor);
            switch (access_spec)
            {
                case CX_CXXPublic:
                    flags = flags | cpp_flags::public_;
                    break;

                case CX_CXXPrivate:
                    flags = flags | cpp_flags::private_;
                    break;

                case CX_CXXProtected:
                    flags = flags | cpp_flags::protected_;
                    break;

                default:
                    break;
            }
        }

        inline std::string get_string(const CXString& value)
        {
            defer _ = [&] { clang_disposeString(value); };
            const char* c_string = clang_getCString(value);
            return c_string != nullptr ? c_string : std::string();
        }

        inline std::string get_name(const CXCursor& cursor)
        {
            const CXString string = clang_getCursorDisplayName(cursor);
            return get_string(string);
        }

        inline std::string get_spelling(const CXCursor& cursor)
        {
            const CXString string = clang_getCursorSpelling(cursor);
            return get_string(string);
        }

        inline std::string get_spelling(const CXType& type)
        {
            const CXString string = clang_getTypeSpelling(type);
            return get_string(string);
        }

        inline std::string get_spelling(const CXTranslationUnit tu, const CXToken& token)
        {
            const CXString string = clang_getTokenSpelling(tu, token);
            return get_string(string);
        }

        inline std::string get_type_spelling(const CXCursor& cursor)
        {
            const CXType type = clang_getCursorType(cursor);
            return get_spelling(type);
        }

        inline std::string get_file(const CXCursor& cursor)
        {
            const CXSourceLocation location = clang_getCursorLocation(cursor);
            CXFile file;
            clang_getFileLocation(location, &file, nullptr, nullptr, nullptr);
            return get_string(clang_getFileName(file));
        }

        inline std::string get_scope(const CXCursor& cursor)
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

        inline std::string_view get_source(const CXCursor& cursor, const std::string& source)
        {
            const CXSourceRange extent = clang_getCursorExtent(cursor);
            const CXSourceLocation location_begin = clang_getRangeStart(extent);
            const CXSourceLocation location_end = clang_getRangeEnd(extent);

            std::uint32_t offset_begin;
            std::uint32_t offset_end;

            clang_getFileLocation(location_begin, nullptr, nullptr, nullptr, &offset_begin);
            clang_getFileLocation(location_end, nullptr, nullptr, nullptr, &offset_end);

            return std::string_view {
                source.data() + offset_begin,
                source.data() + offset_end,
            };
        }

        inline std::string_view get_epilogue(const CXCursor& cursor, const std::string& source)
        {
            const CXSourceRange extent = clang_getCursorExtent(cursor);
            const CXSourceLocation location_end = clang_getRangeEnd(extent);

            std::uint32_t offset_end;

            clang_getFileLocation(location_end, nullptr, nullptr, nullptr, &offset_end);

            return std::string_view {
                source.data() + offset_end,
                source.size() - offset_end,
            };
        }

        inline std::vector<CXToken> get_tokens(const CXTranslationUnit tu, const CXCursor& cursor)
        {
            const CXSourceRange extent = clang_getCursorExtent(cursor);

            CXToken* token_ptr = nullptr;
            unsigned token_count = 0;

            clang_tokenize(tu, extent, &token_ptr, &token_count);

            defer defer_dispose_tokens = [&] { clang_disposeTokens(tu, token_ptr, token_count); };

            std::vector<CXToken> tokens;
            tokens.reserve(token_count);

            std::copy_n(token_ptr, token_count, std::back_inserter(tokens));
            return tokens;
        }

        inline std::optional<std::string> get_default_value(const CXTranslationUnit tu, const std::vector<CXToken>& tokens)
        {
            std::optional<std::string> default_value;

            const auto predicate_default_value = [&](const CXToken& token) {
                return clang_getTokenKind(token) == CXToken_Punctuation && get_spelling(tu, token) == "=";
            };

            const auto it_default_value = std::ranges::find_if(tokens, predicate_default_value);
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

        inline std::optional<std::string> get_default_value(const CXCursor& cursor)
        {
            const CXTranslationUnit tu = clang_Cursor_getTranslationUnit(cursor);
            const std::vector<CXToken> tokens = get_tokens(tu, cursor);
            return get_default_value(tu, tokens);
        }

        inline cpp_template_param make_template_param(const std::string_view param_source, const std::size_t index)
        {
            cpp_template_param param;

            std::string_view remaining = param_source;

            const std::size_t index_equal = remaining.find_last_of('=');

            if (index_equal != std::string_view::npos)
            {
                std::string_view default_value = remaining.substr(index_equal + 1);
                strings::trim(default_value);
                param.default_value = default_value;

                remaining = remaining.substr(0, index_equal - 1);
                strings::trim(remaining);
            }

            std::size_t index_name_begin = remaining.find_last_of(strings::whitespaces);

            for (std::ptrdiff_t index_previous = index_name_begin; index_previous > 0; --index_previous)
            {
                // Handles case where we don't have a name and we have a space in the type, e.g.
                //   template <std :: size_t>

                if (std::ranges::find(valid_name_chars, remaining[index_previous]) != valid_name_chars.end())
                {
                    break;
                }

                if (remaining[index_previous] == ':')
                {
                    index_name_begin = std::string_view::npos;
                    break;
                }
            }

            if (index_name_begin != std::string_view::npos)
            {
                std::string_view name = remaining.substr(index_name_begin);
                strings::trim(name);

                constexpr std::string_view keywords[] = {
                    "auto",
                    "typename",
                    "template",
                    "class",
                };

                if (std::ranges::find(keywords, name) != std::end(keywords))
                {
                    index_name_begin = std::string_view::npos;
                }
            }

            if (index_name_begin == std::string_view::npos)
            {
                param.name = fmt::format("_t{}", index);
            }
            else
            {
                param.name = param_source.substr(index_name_begin, index_equal - index_name_begin);
                strings::trim(param.name);

                remaining = remaining.substr(0, index_name_begin);
                strings::trim(remaining);
            }

            param.type = remaining;
            strings::trim(param.type);

            if (param.type.ends_with(ellipsis))
            {
                param.is_variadic = true;
            }

            return param;
        }

        inline std::vector<cpp_template_param> make_template_params(const CXCursor& cursor, const std::string& source)
        {
            const std::string_view cursor_source = get_source(cursor, source);

            std::vector<cpp_template_param> template_params;

            if (!cursor_source.starts_with("template"))
            {
                return template_params;
            }

            const std::size_t index_bracket_begin = cursor_source.find_first_of("<");

            if (index_bracket_begin == std::string_view::npos)
            {
                return template_params;
            }

            const std::size_t index_template_begin = index_bracket_begin + 1;
            std::size_t index_template_end = cursor_source.find_first_of("<>", index_template_begin);
            std::size_t bracket_count = 1;

            while (index_template_end != std::string_view::npos && index_template_end < cursor_source.size() && bracket_count > 0)
            {
                switch (cursor_source[index_template_end])
                {
                    case '<':
                        ++bracket_count;
                        break;
                    case '>':
                        --bracket_count;
                        break;
                    default:
                        break;
                }

                if (bracket_count > 0)
                {
                    index_template_end = cursor_source.find_first_of("<>", index_template_end + 1);
                }
            }

            std::string_view template_source = cursor_source.substr(index_template_begin, index_template_end - index_template_begin);
            std::size_t index_param_end = 0;
            std::size_t index_param = 0;

            bracket_count = 0;

            while (!template_source.empty())
            {
                index_param_end = template_source.find_first_of("<>,", index_param_end + 1);

                if (index_param_end == std::string_view::npos)
                {
                    const std::string_view param_source = template_source;
                    template_params.emplace_back(make_template_param(param_source, index_param++));
                    template_source = std::string_view {};
                }
                else
                {
                    switch (template_source[index_param_end])
                    {
                        case '<':
                            ++bracket_count;
                            break;
                        case '>':
                            --bracket_count;
                            break;
                        case ',':
                            if (bracket_count == 0)
                            {
                                const std::string_view param_source = template_source.substr(0, index_param_end);
                                template_params.emplace_back(make_template_param(param_source, index_param++));

                                template_source = template_source.substr(index_param_end + 1);
                                strings::trim_start(template_source);

                                index_param_end = 0;
                            }

                            break;
                        default:
                            break;
                    }
                }
            }

            return template_params;
        }

        inline nlohmann::json make_attributes(const CXCursor& cursor)
        {
            std::string attributes = get_spelling(cursor);

            nlohmann::json json;
            if (cpp::parse_pairs(attributes, json))
            {
                return json;
            }

            SPDLOG_WARN("invalid attributes, file={} value={}", get_file(cursor), attributes);
            return nlohmann::json {};
        }

        inline cpp_ref make_ref(const CXCursor& cursor, const cpp_file& file)
        {
            cpp_ref ref;
            ref.name = get_name(cursor);

            std::string_view cursor_epilogue = get_epilogue(cursor, file.source);
            strings::trim_start(cursor_epilogue);

            if (cursor_epilogue.starts_with(ellipsis))
            {
                ref.is_variadic = true;
                ref.name += ellipsis;
            }

            return ref;
        }

        inline cpp_argument make_argument(const CXCursor& cursor, const std::size_t index)
        {
            cpp_argument argument;
            argument.name = get_name(cursor);
            argument.default_value = get_default_value(cursor);
            argument.type.name = get_type_spelling(cursor);

            if (argument.type.name.ends_with(ellipsis))
            {
                argument.is_variadic = true;
                argument.type.name.resize(argument.type.name.size() - ellipsis.size());
            }

            if (argument.name.empty())
            {
                argument.name = fmt::format("_arg{}", index);
            }

            const auto visitor = [&](const CXCursor& v_cursor, const CXCursor&) {
                switch (v_cursor.kind)
                {
                    case CXCursor_AnnotateAttr: {
                        argument.attributes = make_attributes(v_cursor);
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

        inline cpp_function make_function(const CXCursor& cursor, const cpp_file& file)
        {
            cpp_function function;
            function.name = get_spelling(cursor);
            function.scope = get_scope(cursor);
            function.template_params = make_template_params(cursor, file.source);

            const CXType type = clang_getCursorType(cursor);
            const CXType return_type = clang_getResultType(type);
            function.return_type.name = get_string(clang_getTypeSpelling(return_type));

            add_access_flags(cursor, function.flags);

            if (clang_CXXMethod_isConst(cursor))
            {
                function.flags = function.flags | cpp_flags::const_;
            }

            if (clang_CXXMethod_isStatic(cursor))
            {
                function.flags = function.flags | cpp_flags::static_;
            }

            if (clang_CXXMethod_isVirtual(cursor))
            {
                function.flags = function.flags | cpp_flags::virtual_;
            }

            if (clang_CXXMethod_isDefaulted(cursor))
            {
                function.flags = function.flags | cpp_flags::default_;
            }

            if (clang_CXXMethod_isDeleted(cursor))
            {
                function.flags = function.flags | cpp_flags::delete_;
            }

            const auto visitor = [&](const CXCursor& v_cursor, const CXCursor&) {
                switch (v_cursor.kind)
                {
                    case CXCursor_ParmDecl: {
                        insert_end(function.arguments, [&] { return make_argument(v_cursor, function.arguments.size()); });
                        break;
                    }

                    case CXCursor_TemplateTypeParameter:
                    case CXCursor_TemplateTemplateParameter:
                    case CXCursor_NonTypeTemplateParameter: {
                        break;
                    }

                    case CXCursor_AnnotateAttr: {
                        function.attributes = make_attributes(v_cursor);
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

        inline cpp_constructor make_constructor(const CXCursor& cursor, const cpp_file& file)
        {
            cpp_function function = make_function(cursor, file);

            cpp_constructor constructor;
            constructor.flags = function.flags;
            constructor.arguments = std::move(function.arguments);
            constructor.attributes = std::move(function.attributes);
            constructor.template_params = std::move(function.template_params);
            constructor.template_specialization_params = std::move(function.template_specialization_params);

            return constructor;
        }

        inline cpp_enum_value make_enum_value(const CXCursor& cursor)
        {
            cpp_enum_value enum_value;
            enum_value.name = get_name(cursor);
            enum_value.value = clang_getEnumConstantDeclValue(cursor);

            const auto visitor = [&](const CXCursor& v_cursor, const CXCursor&) {
                switch (v_cursor.kind)
                {
                    case CXCursor_AnnotateAttr: {
                        enum_value.attributes = make_attributes(v_cursor);
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

        inline cpp_enum make_enum(const CXCursor& cursor)
        {
            cpp_enum enum_;
            enum_.name = get_name(cursor);
            enum_.scope = get_scope(cursor);
            enum_.type = clang_EnumDecl_isScoped(cursor) ? cpp_enum_type::enum_class : cpp_enum_type::enum_;

            const CXCursor parent = clang_getCursorSemanticParent(cursor);
            enum_.nested = parent.kind != CXCursor_Namespace;

            const CXType base_type = clang_getEnumDeclIntegerType(cursor);
            enum_.base.name = get_string(clang_getTypeSpelling(base_type));

            const auto visitor = [&](const CXCursor& v_cursor, const CXCursor&) {
                switch (v_cursor.kind)
                {
                    case CXCursor_EnumConstantDecl: {
                        insert_end(enum_.values, [&] { return make_enum_value(v_cursor); });
                        break;
                    }

                    case CXCursor_AnnotateAttr: {
                        enum_.attributes = make_attributes(v_cursor);
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

        inline cpp_field make_field(const CXCursor& cursor)
        {
            cpp_field field;
            field.name = get_name(cursor);
            field.default_value = get_default_value(cursor);
            field.type.name = get_type_spelling(cursor);

            add_access_flags(cursor, field.flags);

            const auto visitor = [&](const CXCursor& v_cursor, const CXCursor&) {
                switch (v_cursor.kind)
                {
                    case CXCursor_AnnotateAttr: {
                        field.attributes = make_attributes(v_cursor);
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

        inline cpp_class make_class(const CXCursor& cursor, cpp_file& file)
        {
            cpp_class class_;
            class_.name = get_name(cursor);
            class_.scope = get_scope(cursor);
            class_.template_params = make_template_params(cursor, file.source);

            const CXCursor specialization = clang_getSpecializedCursorTemplate(cursor);
            erase_template(class_.name, clang_Cursor_isNull(specialization) ? nullptr : &class_.template_specialization_params);

            const CXCursor parent = clang_getCursorSemanticParent(cursor);
            class_.nested = parent.kind != CXCursor_Namespace;

            CXCursorKind cursor_kind = cursor.kind;
            if (cursor_kind == CXCursor_ClassTemplate || cursor_kind == CXCursor_ClassTemplatePartialSpecialization)
            {
                cursor_kind = clang_getTemplateCursorKind(cursor);
            }

            switch (cursor_kind)
            {
                case CXCursor_StructDecl: {
                    class_.type = cpp_class_type::struct_;
                    break;
                }

                case CXCursor_ClassDecl: {
                    class_.type = cpp_class_type::class_;
                    break;
                }

                default: {
                    break;
                }
            }

            const auto visitor = [&](const CXCursor& v_cursor, const CXCursor&) {
                switch (v_cursor.kind)
                {
                    case CXCursor_TemplateTypeParameter:
                    case CXCursor_TemplateTemplateParameter:
                    case CXCursor_NonTypeTemplateParameter: {
                        break;
                    }

                    case CXCursor_AnnotateAttr: {
                        class_.attributes = make_attributes(v_cursor);
                        break;
                    }

                    case CXCursor_CXXBaseSpecifier: {
                        insert_end(class_.bases, [&] { return make_ref(v_cursor, file); });
                        break;
                    }

                    case CXCursor_FieldDecl: {
                        insert_end(class_.fields, [&] { return make_field(v_cursor); });
                        break;
                    }

                    case CXCursor_Constructor: {
                        insert_end(class_.constructors, [&] { return make_constructor(v_cursor, file); });
                        break;
                    }

                    case CXCursor_FunctionDecl:
                    case CXCursor_FunctionTemplate:
                    case CXCursor_CXXMethod: {
                        insert_end(class_.functions, [&] { return make_function(v_cursor, file); });
                        break;
                    }

                    case CXCursor_StructDecl:
                    case CXCursor_ClassDecl:
                    case CXCursor_ClassTemplate:
                    case CXCursor_ClassTemplatePartialSpecialization: {
                        if (clang_isCursorDefinition(v_cursor))
                        {
                            insert_end(file.classes, [&] { return make_class(v_cursor, file); });
                        }

                        break;
                    }

                    case CXCursor_EnumDecl: {
                        if (clang_isCursorDefinition(v_cursor))
                        {
                            insert_end(file.enums, [&] { return make_enum(v_cursor); });
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

        inline void make_file(const CXCursor& cursor, const CXCursor& parent, cpp_file& file)
        {
            const auto visitor = [&](const CXCursor& v_cursor, const CXCursor&) {
                switch (v_cursor.kind)
                {
                    case CXCursor_ClassTemplate:
                    case CXCursor_ClassDecl:
                    case CXCursor_StructDecl:
                    case CXCursor_ClassTemplatePartialSpecialization: {
                        if (clang_isCursorDefinition(v_cursor))
                        {
                            insert_end(file.classes, [&] { return make_class(v_cursor, file); });
                        }

                        break;
                    }

                    case CXCursor_EnumDecl: {
                        if (clang_isCursorDefinition(v_cursor))
                        {
                            insert_end(file.enums, [&] { return make_enum(v_cursor); });
                        }

                        break;
                    }

                    case CXCursor_FunctionTemplate:
                    case CXCursor_FunctionDecl:
                    case CXCursor_CXXMethod: {
                        insert_end(file.functions, [&] { return make_function(v_cursor, file); });
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

    struct codegen_parser_cpp final : codegen_parser<cpp_file>
    {
        struct clang_index_deleter
        {
            using pointer = CXIndex;

            void operator()(CXIndex index) const
            {
                clang_disposeIndex(index);
            }
        };

        std::unique_ptr<CXIndex, clang_index_deleter> index;
        std::vector<std::string> args;
        std::vector<const char*> args_view;

        template <typename args_t>
        explicit codegen_parser_cpp(const args_t& additional_args)
        {
            index = std::unique_ptr<CXIndex, clang_index_deleter>(
                clang_createIndex(0, 0));

            args.emplace_back("-xc++");
            args.emplace_back("-E");
            args.emplace_back("-P");
            args.emplace_back("-dM");
            args.emplace_back("-w");
            args.emplace_back("-Wno-everything");

            const auto transformer = [](const auto& arg) { return std::string {arg}; };
            std::transform(std::begin(additional_args), std::end(additional_args), std::back_inserter(args), transformer);
            std::ranges::sort(args);
            args.erase(std::ranges::unique(args).begin(), args.end());

            args_view.resize(args.size());

            std::ranges::transform(args, args_view.begin(),
                [](const std::string& value) { return value.data(); });
        }

        bool parse_asts(const std::vector<std::string>& paths, std::vector<cpp_file>& cpp_files) override
        {
            constexpr auto flags = static_cast<CXTranslationUnit_Flags>(
                CXTranslationUnit_Incomplete |
                CXTranslationUnit_KeepGoing |
                CXTranslationUnit_SkipFunctionBodies);

            std::size_t file_index = 0;
            std::map<std::string_view, std::size_t> file_map;

            cpp_files.clear();
            cpp_files.reserve(paths.size());

            for (const std::string& path : paths)
            {
                cpp_file cpp_file = {.path = path};

                if (!files::read_file(path, cpp_file.source))
                {
                    SPDLOG_ERROR("cannot read source file, path={}", path);
                    return false;
                }

                file_map.emplace(path, file_index++);
                cpp_files.emplace_back(std::move(cpp_file));
            }

            std::string main_source;
            for (const std::string& path : paths)
            {
                main_source += "#include \"";
                main_source += path;
                main_source += "\"\n";
            }

            std::vector<CXUnsavedFile> unsaved_files;
            unsaved_files.reserve(cpp_files.size() + 1);

            for (std::size_t index = 0; index < paths.size(); ++index)
            {
                const std::string& source = cpp_files[index].source;
                unsaved_files.emplace_back() = CXUnsavedFile {paths[index].data(), source.data(), static_cast<unsigned long>(source.size())};
            }

            constexpr std::string_view source_file = "__main.cpp";
            unsaved_files.emplace_back() = CXUnsavedFile {source_file.data(), main_source.data(), static_cast<unsigned long>(main_source.size())};

            CXTranslationUnit translation_unit = nullptr;
            const CXErrorCode error = clang_parseTranslationUnit2(
                index.get(), source_file.data(),
                args_view.data(), static_cast<int>(args_view.size()),
                unsaved_files.data(), static_cast<unsigned long>(unsaved_files.size()),
                flags, &translation_unit);

            if (error != CXError_Success)
            {
                SPDLOG_ERROR("unable to parse translation unit, code={}", static_cast<int>(error));
                return false;
            }

            defer dispose_translation_unit = [&] { clang_disposeTranslationUnit(translation_unit); };

            const std::size_t diagnostics_count = clang_getNumDiagnostics(translation_unit);
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

            const auto visitor = [&](const CXCursor& cursor, const CXCursor& parent) {
                const auto it_file = file_map.find(detail::get_file(cursor));
                if (it_file != file_map.end())
                {
                    std::size_t index = it_file->second;
                    detail::make_file(cursor, parent, cpp_files[index]);
                }

                return CXChildVisit_Continue;
            };

            const CXCursor cursor = clang_getTranslationUnitCursor(translation_unit);
            detail::visit_children(cursor, visitor);

            return true;
        }
    };
}