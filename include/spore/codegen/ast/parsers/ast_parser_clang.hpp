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
#include "spore/codegen/codegen_options.hpp"
#include "spore/codegen/misc/defer.hpp"
#include "spore/codegen/utils/files.hpp"
#include "spore/codegen/utils/strings.hpp"

namespace spore::codegen
{
    namespace detail
    {
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

            strings::trim_end(preamble);

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
            strings::trim(type);

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

            strings::trim_start(epilogue);

            if (epilogue.starts_with(equal_sign))
            {
                strings::trim_start(epilogue, equal_sign);
                // template_param.default_value = find_expression(epilogue);
            }

            return template_param;
        }

        nlohmann::json make_attributes(CXCursor cursor)
        {
            std::string attributes = get_spelling(cursor);

            nlohmann::json json;
            if (ast::parse_pairs_to_json(attributes, json))
            {
                return json;
            }

            SPDLOG_WARN("invalid attributes, file={} value={}", get_file(cursor), attributes);
            return {};
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
                        closure_field.attributes = make_attributes(cursor);
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

        ast_argument make_argument(CXCursor cursor, ast_file& data)
        {
            ast_argument argument;
            argument.name = get_name(cursor);

            CXType type = clang_getCursorType(cursor);
            argument.type.name = to_string(clang_getTypeSpelling(type));

            using closure_t = std::tuple<ast_argument&, ast_file&>;
            const auto visitor = [](CXCursor cursor, CXCursor parent, CXClientData data_ptr) {
                auto& [closure_argument, closure_data] = *static_cast<closure_t*>(data_ptr);
                switch (cursor.kind)
                {
                    case CXCursor_AnnotateAttr: {
                        closure_argument.attributes = make_attributes(cursor);
                        break;
                    }

                    default: {
                        return CXChildVisit_Recurse;
                    }
                }

                return CXChildVisit_Continue;
            };

            closure_t closure {argument, data};
            clang_visitChildren(cursor, visitor, &closure);

            return argument;
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
                    case CXCursor_ParmDecl: {
                        closure_function.arguments.emplace_back(make_argument(cursor, closure_data));
                        break;
                    }

                    case CXCursor_TemplateTypeParameter:
                    case CXCursor_NonTypeTemplateParameter: {
                        closure_function.template_params.emplace_back(make_template_param(cursor, closure_data));
                        break;
                    }

                    case CXCursor_AnnotateAttr: {
                        closure_function.attributes = make_attributes(cursor);
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
                        break;
                    }

                    case CXCursor_AnnotateAttr: {
                        closure_class.attributes = make_attributes(cursor);
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

                if (!files::read_file(path, ast_file.source))
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
