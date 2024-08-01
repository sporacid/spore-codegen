#pragma once

#include <algorithm>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

#include "fmt/format.h"
#include "spdlog/spdlog.h"
#include "clang-c/CXSourceLocation.h"
#include "clang-c/Index.h"

#include "spore/codegen/ast/ast_utils.hpp"
#include "spore/codegen/ast/parsers/ast_parser.hpp"
#include "spore/codegen/codegen_helpers.hpp"
#include "spore/codegen/codegen_options.hpp"
#include "spore/codegen/misc/defer.hpp"

namespace spore::codegen
{
    namespace detail
    {
#if 0
        constexpr std::string_view whitespaces = " \t\r\n";

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
#endif
#if 0
        template <bool backward_v = false>
        std::string_view find_template(std::string_view source)
        {
            constexpr std::string_view chevrons = "<>";
            constexpr char chevron_begin = backward_v ? '>' : '<';

            std::string_view template_;

            std::size_t chevron_count = 0;
            std::size_t chevron_index;

            if constexpr (backward_v)
            {
                chevron_index = source.find_last_of(chevrons);
            }
            else
            {
                chevron_index = source.find_first_of(chevrons);
            }

            std::size_t index_template_begin = chevron_index;
            std::size_t index_template_end = std::string_view::npos;

            while (chevron_index != std::string_view::npos && index_template_end == std::string_view::npos)
            {
                std::size_t chevron_diff = source.at(chevron_index) == chevron_begin ? 1 : -1;
                chevron_count += chevron_diff;

                if (chevron_count == 0)
                {
                    index_template_end = chevron_index;
                }
                else
                {
                    if constexpr (backward_v)
                    {
                        chevron_index = source.find_last_of(chevrons, chevron_index - 1);
                    }
                    else
                    {
                        chevron_index = source.find_first_of(chevrons, chevron_index + 1);
                    }
                }
            }

            if (index_template_begin != std::string_view::npos)
            {
                template_ = source.substr(index_template_begin, index_template_end - index_template_begin + 1);
            }

            return template_;
        }

        template <bool backward_v = false>
        std::string_view find_expression(std::string_view source)
        {
            constexpr std::string_view subexpression_begin = backward_v ? ">]}" : "<[{";
            constexpr std::string_view subexpression_end = backward_v ? "<[{" : ">]}";
            constexpr std::string_view expression_end = ":;,?";

            std::array<std::size_t, subexpression_begin.size()> sub_counters {};
            std::size_t index = 0;

            while (index < source.size())
            {
                std::size_t index_sub = subexpression_begin.find(source.at(index));

                if (index_sub != std::string_view::npos)
                {
                    // subexpression begin
                    ++sub_counters[index_sub];
                }
                else
                {
                    index_sub = subexpression_end.find(source.at(index));

                    if (index_sub != std::string_view::npos)
                    {
                        if (std::all_of(sub_counters.begin(), sub_counters.end(), [](std::size_t value) { return value == 0; }))
                        {
                            // expression end
                            break;
                        }

                        // subexpression end
                        --sub_counters[index_sub];
                    }
                    else
                    {
                        std::size_t index_end = expression_end.find(source.at(index));

                        if (index_end != std::string_view::npos)
                        {
                            if constexpr (backward_v)
                            {
                            }
                            else
                            {
                            }

                            if (expression_end.at(index_end) != ':' || (index + 1 < source.size() && source.at(index + 1) != ':'))
                            {
                                // expression end
                                break;
                            }

                            // skip ::
                            ++index;
                        }
                    }
                }

                ++index;
            }

            std::string_view expression {source.data(), index};
            trim_chars(expression, whitespaces);
            trim_end_chars(expression, whitespaces);

            return expression;
        }
#endif

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
            constexpr std::string_view delimiters = "<>[]{}:;,";

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

        std::string get_spelling(CXCursor cursor)
        {
            CXString string = clang_getCursorSpelling(cursor);
            return to_string(string);
        }

        std::string get_file(CXCursor cursor)
        {
            CXSourceLocation location = clang_getCursorLocation(cursor);
            CXFile file;
            clang_getFileLocation(location, &file, nullptr, nullptr, nullptr);
            return to_string(clang_getFileName(file));
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

        std::string_view get_epilogue(CXCursor cursor, std::string_view source)
        {
            CXSourceLocation location = clang_getCursorLocation(cursor);

            CXFile file;
            std::uint32_t line;
            std::uint32_t column;
            std::uint32_t offset;

            clang_getFileLocation(location, &file, &line, &column, &offset);

            return std::string_view {source.data() + offset, source.size() - offset};
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

#if 0
        void add_attributes(std::string_view spelling, std::map<std::string, std::string>& attributes)
        {
            constexpr std::string_view key_delimiters = ",";
            constexpr std::string_view value_delimiters = "=";
            constexpr std::string_view default_value = "true";

            while (!spelling.empty())
            {
                std::size_t index_begin = 0;
                std::size_t index_end = spelling.find_first_of(key_delimiters);
                std::size_t count = index_end != std::string_view::npos ? index_end - index_begin : std::string_view::npos;

                std::string_view pair = spelling.substr(index_begin, count);
                std::size_t index_value = pair.find_first_of(value_delimiters);

                std::string_view key;
                std::string_view value;

                if (index_value != std::string_view::npos)
                {
                    key = pair.substr(0, index_value - 1);
                    value = pair.substr(index_value + 1);
                }
                else
                {
                    key = pair;
                    value = default_value;
                }

                trim_chars(key, whitespaces);
                trim_chars(value, whitespaces);

                trim_end_chars(key, whitespaces);
                trim_end_chars(value, whitespaces);

                attributes.emplace(std::string(key), std::string(value));
                spelling.remove_prefix(count != std::string_view::npos ? count + 1 : spelling.size());
            }
        }
#endif

        ast_template_param make_template_param(CXCursor cursor, ast_file& data)
        {
            constexpr std::string_view equal_sign = "=";

            std::string_view preamble = get_preamble(cursor, data.source);
            std::string_view epilogue = get_epilogue(cursor, data.source);

            ast_template_param template_param;
            // template_param.type = find_expression<true>(preamble);
            template_param.type = rfind_type(preamble);
            template_param.name = get_name(cursor);
            template_param.is_variadic = template_param.type.ends_with("...");

            trim_begin_chars(epilogue, whitespaces);

            if (epilogue.starts_with(equal_sign))
            {
                trim_begin_chars(epilogue, equal_sign);
                // template_param.default_value = find_expression(epilogue);
            }

            return template_param;
        }

        ast_field make_field(CXCursor cursor, ast_file& data)
        {
            ast_field field;
            field.name = get_name(cursor);

            CXType type = clang_getCursorType(cursor);
            field.type.name = to_string(clang_getTypeSpelling(type));

            add_access_flags(cursor, field.flags);

            using closure_t = std::tuple<ast_field&, ast_file&>;
            const auto visitor = [](CXCursor cursor, CXCursor parent, CXClientData data_ptr) {
                auto& [closure_field, closure_data] = *static_cast<closure_t*>(data_ptr);
                switch (cursor.kind)
                {
                    case CXCursor_AnnotateAttr: {
                        ast::parse_pairs(get_spelling(cursor), closure_field.attributes);
                        break;
                    }

                    default: {
                        return CXChildVisit_Recurse;
                    }
                }

                return CXChildVisit_Continue;
            };

            closure_t closure {field, data};
            clang_visitChildren(cursor, visitor, &closure);

            return field;
        }

        ast_function make_function(CXCursor cursor, ast_file& data)
        {
            ast_function function;
            function.name = get_name(cursor);
            function.scope = get_scope(cursor);

            add_access_flags(cursor, function.flags);

            using closure_t = std::tuple<ast_function&, ast_file&>;
            const auto visitor = [](CXCursor cursor, CXCursor parent, CXClientData data_ptr) {
                auto& [closure_function, closure_data] = *static_cast<closure_t*>(data_ptr);
                switch (cursor.kind)
                {
                    case CXCursor_TemplateTypeParameter:
                    case CXCursor_NonTypeTemplateParameter: {
                        closure_function.template_params.emplace_back(make_template_param(cursor, closure_data));
                        break;
                    }

                    case CXCursor_AnnotateAttr: {
                        ast::parse_pairs(get_spelling(cursor), closure_function.attributes);
                        break;
                    }

                    default: {
                        return CXChildVisit_Recurse;
                    }
                }

                return CXChildVisit_Continue;
            };

            closure_t closure {function, data};
            clang_visitChildren(cursor, visitor, &closure);

            return function;
        }

        ast_class make_class(CXCursor cursor, ast_file& data)
        {
            ast_class class_;
            class_.name = get_name(cursor);
            class_.scope = get_scope(cursor);

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

            if (data.path.ends_with("asset.hpp"))
            {
                printf("");
            }

            using closure_t = std::tuple<ast_class&, ast_file&>;
            const auto visitor = [](CXCursor cursor, CXCursor parent, CXClientData data_ptr) {
                auto& [closure_class, closure_data] = *static_cast<closure_t*>(data_ptr);
                switch (cursor.kind)
                {
                    case CXCursor_FieldDecl: {
                        closure_class.fields.emplace_back(make_field(cursor, closure_data));
                        break;
                    }

                    case CXCursor_FunctionTemplate:
                    case CXCursor_FunctionDecl: {
                        closure_class.functions.emplace_back(make_function(cursor, closure_data));
                        break;
                    }

                    case CXCursor_CXXBaseSpecifier: {
                        ast_ref& base = closure_class.bases.emplace_back();
                        base.name = get_name(cursor);
                        break;
                    }

                    case CXCursor_TemplateTypeParameter:
                    case CXCursor_NonTypeTemplateParameter: {
                        closure_class.template_params.emplace_back(make_template_param(cursor, closure_data));
                        //                        std::string_view preamble = get_preamble(cursor, closure_data.source);
                        //                        ast_template_param& template_param = closure_class.template_params.emplace_back();
                        //                        template_param.type = rfind_type(preamble);
                        //                        template_param.name = get_name(cursor);
                        //                        template_param.is_variadic = template_param.type.ends_with("...");
                        break;
                    }

                    case CXCursor_AnnotateAttr: {
                        ast::parse_pairs(get_spelling(cursor), closure_class.attributes);
                        break;
                    }

                    case CXCursor_StructDecl:
                    case CXCursor_ClassDecl:
                    case CXCursor_ClassTemplate: {
                        // TODO @sporacid handle inner classes
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

        void make_file(CXCursor cursor, ast_file& data)
        {
            using closure_t = std::tuple<CXCursorVisitor, ast_file&>;
            const auto visitor = [](CXCursor cursor, CXCursor parent, CXClientData data_ptr) {
                const auto& [closure_visitor, closure_data] = *static_cast<closure_t*>(data_ptr);

                switch (cursor.kind)
                {
                    case CXCursor_Namespace: {
                        clang_visitChildren(cursor, closure_visitor, data_ptr);
                        break;
                    }

                    case CXCursor_ClassTemplate:
                    case CXCursor_ClassDecl:
                    case CXCursor_StructDecl: {
                        // TODO @sporacid handle inner classes
                        closure_data.classes.emplace_back(make_class(cursor, closure_data));
                        break;
                    }

                    case CXCursor_FunctionTemplate:
                    case CXCursor_FunctionDecl: {
                        closure_data.functions.emplace_back(make_function(cursor, closure_data));
                        return CXChildVisit_Continue;
                    }

                    default:
                        return CXChildVisit_Recurse;
                }

                return CXChildVisit_Continue;
            };

            closure_t closure {visitor, data};
            clang_visitChildren(cursor, visitor, &closure);
        }
    }

    struct ast_parser_clang : ast_parser
    {
        std::vector<std::string> args;
        std::vector<const char*> args_view;
        CXIndex clang_index;

        explicit ast_parser_clang(const codegen_options& options)
        {
            args.emplace_back("-xc++");
            args.emplace_back("-E");
            args.emplace_back("-P");
            args.emplace_back("-dM");
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

        bool parse_file(const std::string& path, ast_file& ast_file) override
        {
            constexpr auto flags = static_cast<CXTranslationUnit_Flags>(
                CXTranslationUnit_Incomplete |
                CXTranslationUnit_KeepGoing |
                CXTranslationUnit_SkipFunctionBodies |
                CXTranslationUnit_IncludeAttributedTypes |
                CXTranslationUnit_VisitImplicitAttributes);

            std::string source;
            if (!read_file(path, source))
            {
                SPDLOG_ERROR("cannot read source file, path={}", path);
                return false;
            }

            CXUnsavedFile memory_file {path.data(), source.data(), static_cast<unsigned long>(source.size())};
            CXTranslationUnit translation_unit = clang_parseTranslationUnit(
                clang_index, path.data(), args_view.data(), static_cast<int>(args_view.size()), &memory_file, 1, flags);

            if (translation_unit == nullptr)
            {
                SPDLOG_ERROR("unable to parse translation unit, path={}", path);
                return false;
            }

            defer dispose_translation_unit = [&] { clang_disposeTranslationUnit(translation_unit); };
            CXCursor cursor = clang_getTranslationUnitCursor(translation_unit);

            ast_file.path = path;

            detail::make_file(cursor, ast_file);

#if 0
            {
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
            }
#endif
            return true;
        }

        bool parse_files(const std::vector<std::string>& paths, std::vector<ast_file>& ast_files) override
        {
            constexpr auto flags = static_cast<CXTranslationUnit_Flags>(
                CXTranslationUnit_Incomplete |
                CXTranslationUnit_KeepGoing |
                CXTranslationUnit_SkipFunctionBodies |
                CXTranslationUnit_IncludeAttributedTypes |
                CXTranslationUnit_VisitImplicitAttributes);

            std::string main_source;
            for (const std::string& path : paths)
            {
                main_source += "#include \"";
                main_source += path;
                main_source += "\"\n";
            }

            ast_files.clear();
            ast_files.reserve(paths.size());

            for (const std::string& path : paths)
            {
                ast_file ast_file = {.path = path};

                if (!read_file(path, ast_file.source))
                {
                    SPDLOG_ERROR("cannot read source file, path={}", path);
                    return false;
                }

                ast_files.emplace_back(std::move(ast_file));
            }

            std::vector<CXUnsavedFile> source_files;
            source_files.reserve(ast_files.size() + 1);

            for (std::size_t index = 0; index < paths.size(); ++index)
            {
                const std::string& source = ast_files[index].source;
                source_files.emplace_back() = CXUnsavedFile {paths[index].data(), source.data(), static_cast<unsigned long>(source.size())};
            }

            std::size_t index = 0;
            std::map<std::string_view, std::size_t> file_map;

            for (const std::string& path : paths)
            {
                file_map.emplace(path, index++);
            }

            constexpr std::string_view source_file = "source.cpp";
            source_files.emplace_back() = CXUnsavedFile {source_file.data(), main_source.data(), static_cast<unsigned long>(main_source.size())};

            CXTranslationUnit translation_unit = clang_parseTranslationUnit(
                clang_index, source_file.data(), args_view.data(), static_cast<int>(args_view.size()), source_files.data(), static_cast<unsigned long>(source_files.size()), flags);

            if (translation_unit == nullptr)
            {
                SPDLOG_ERROR("unable to parse translation unit");
                return false;
            }

            defer dispose_translation_unit = [&] { clang_disposeTranslationUnit(translation_unit); };
            CXCursor cursor = clang_getTranslationUnitCursor(translation_unit);

            using closure_t = std::tuple<std::vector<ast_file>&, std::map<std::string_view, std::size_t>&>;
            const auto visitor = [](CXCursor cursor, CXCursor parent, CXClientData data_ptr) {
                const auto& [closure_ast_files, closure_file_map] = *static_cast<closure_t*>(data_ptr);

                const auto it_file = closure_file_map.find(detail::get_file(cursor));
                if (it_file != closure_file_map.end())
                {
                    std::size_t index = it_file->second;
                    detail::make_file(cursor, closure_ast_files[index]);
                }

                return CXChildVisit_Continue;
            };

            closure_t closure {ast_files, file_map};
            clang_visitChildren(cursor, visitor, &closure);

            return true;
        }
    };
}
