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
        template <typename value_t, typename func_t>
        void insert_at_end(std::vector<value_t>& values, func_t&& func)
        {
            std::size_t index = values.size();
            value_t value = func();
            values.insert(values.begin() + index, std::move(value));
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
            const char* c_string = clang_getCString(value);
            return c_string != nullptr ? c_string : std::string();
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

        ast_template_param make_template_param(CXCursor cursor, ast_file& data, std::size_t index)
        {
            constexpr std::string_view equal_sign = "=";

            std::string_view preamble = get_preamble(cursor, data.source);
            std::string_view epilogue = get_epilogue(cursor, data.source);

            ast_template_param template_param;
            // template_param.type = find_expression<true>(preamble);
            template_param.type = rfind_type(preamble);
            template_param.name = get_name(cursor);
            template_param.is_variadic = template_param.type.ends_with("...");

            if (template_param.name.empty())
            {
                template_param.name = fmt::format("_t{}", index);
            }

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
            if (ast::parse_pairs(attributes, json))
            {
                return json;
            }

            SPDLOG_WARN("invalid attributes, file={} value={}", get_file(cursor), attributes);
            return nlohmann::json {};
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

        ast_argument make_argument(CXCursor cursor, ast_file& data, std::size_t index)
        {
            ast_argument argument;
            argument.name = get_name(cursor);

            if (argument.name.empty())
            {
                argument.name = fmt::format("_arg{}", index);
            }

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
            function.name = get_spelling(cursor);
            function.scope = get_scope(cursor);

            CXType type = clang_getCursorType(cursor);
            CXType return_type = clang_getResultType(type);
            function.return_type.name = to_string(clang_getTypeSpelling(return_type));

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

            using closure_t = std::tuple<ast_function&, ast_file&>;
            const auto visitor = [](CXCursor cursor, CXCursor parent, CXClientData data_ptr) {
                auto& [closure_function, closure_data] = *static_cast<closure_t*>(data_ptr);
                switch (cursor.kind)
                {
                    case CXCursor_ParmDecl: {
                        closure_function.arguments.emplace_back(make_argument(cursor, closure_data, closure_function.arguments.size()));
                        break;
                    }

                    case CXCursor_TemplateTypeParameter:
                    case CXCursor_NonTypeTemplateParameter: {
                        closure_function.template_params.emplace_back(make_template_param(cursor, closure_data, closure_function.template_params.size()));
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

        ast_constructor make_constructor(CXCursor cursor, ast_file& data)
        {
            ast_function function = make_function(cursor, data);

            ast_constructor constructor;
            constructor.flags = function.flags;
            constructor.arguments = std::move(function.arguments);
            constructor.attributes = std::move(function.attributes);
            constructor.template_params = std::move(function.template_params);
            constructor.template_specialization_params = std::move(function.template_specialization_params);

            return constructor;
        }

        ast_enum_value make_enum_value(CXCursor cursor, ast_file& data)
        {
            ast_enum_value enum_value;
            enum_value.name = get_name(cursor);
            enum_value.value = clang_getEnumConstantDeclValue(cursor);

            using closure_t = std::tuple<ast_enum_value&, ast_file&>;
            const auto visitor = [](CXCursor cursor, CXCursor parent, CXClientData data_ptr) {
                auto& [closure_enum_value, closure_data] = *static_cast<closure_t*>(data_ptr);
                switch (cursor.kind)
                {
                    case CXCursor_AnnotateAttr: {
                        closure_enum_value.attributes = make_attributes(cursor);
                        break;
                    }

                    default: {
                        return CXChildVisit_Recurse;
                    }
                }

                return CXChildVisit_Continue;
            };

            closure_t closure {enum_value, data};
            clang_visitChildren(cursor, visitor, &closure);

            return enum_value;
        }

        ast_enum make_enum(CXCursor cursor, ast_file& data)
        {
            ast_enum enum_;
            enum_.name = get_name(cursor);
            enum_.scope = get_scope(cursor);
            enum_.type = clang_EnumDecl_isScoped(cursor) ? ast_enum_type::enum_class : ast_enum_type::enum_;

            CXType base_type = clang_getEnumDeclIntegerType(cursor);
            enum_.base.name = to_string(clang_getTypeSpelling(base_type));

            using closure_t = std::tuple<ast_enum&, ast_file&>;
            const auto visitor = [](CXCursor cursor, CXCursor parent, CXClientData data_ptr) {
                auto& [closure_enum, closure_data] = *static_cast<closure_t*>(data_ptr);
                switch (cursor.kind)
                {
                    case CXCursor_EnumConstantDecl: {
                        closure_enum.values.emplace_back(make_enum_value(cursor, closure_data));
                        break;
                    }

                    case CXCursor_AnnotateAttr: {
                        closure_enum.attributes = make_attributes(cursor);
                        break;
                    }

                    default: {
                        return CXChildVisit_Recurse;
                    }
                }

                return CXChildVisit_Continue;
            };

            closure_t closure {enum_, data};
            clang_visitChildren(cursor, visitor, &closure);

            return enum_;
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
                    case CXCursor_TemplateTypeParameter:
                    case CXCursor_NonTypeTemplateParameter: {
                        closure_class.template_params.emplace_back(make_template_param(cursor, closure_data, closure_class.template_params.size()));
                        break;
                    }

                    case CXCursor_AnnotateAttr: {
                        closure_class.attributes = make_attributes(cursor);
                        break;
                    }

                    case CXCursor_CXXBaseSpecifier: {
                        ast_ref& base = closure_class.bases.emplace_back();
                        base.name = get_name(cursor);
                        break;
                    }

                    case CXCursor_FieldDecl: {
                        insert_at_end(closure_class.fields, [&] { return make_field(cursor, closure_data); });
                        break;
                    }

                    case CXCursor_Constructor: {
                        insert_at_end(closure_class.constructors, [&] { return make_constructor(cursor, closure_data); });
                        break;
                    }

                    case CXCursor_FunctionDecl:
                    case CXCursor_FunctionTemplate:
                    case CXCursor_CXXMethod: {
                        insert_at_end(closure_class.functions, [&] { return make_function(cursor, closure_data); });
                        break;
                    }

                    case CXCursor_StructDecl:
                    case CXCursor_ClassDecl:
                    case CXCursor_ClassTemplate: {
                        insert_at_end(closure_data.classes, [&] { return make_class(cursor, closure_data); });
                        break;
                    }

                    case CXCursor_EnumDecl: {
                        insert_at_end(closure_data.enums, [&] { return make_enum(cursor, closure_data); });
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

        void make_file(CXCursor cursor, CXCursor parent, ast_file& data)
        {
            using closure_t = std::tuple<ast_file&>;
            const auto visitor = [](CXCursor cursor, CXCursor parent, CXClientData data_ptr) {
                const auto& [closure_data] = *static_cast<closure_t*>(data_ptr);

                std::string spelling = get_spelling(cursor);
                std::string parent_spelling = get_spelling(parent);

                switch (cursor.kind)
                {
                    case CXCursor_ClassTemplate:
                    case CXCursor_ClassDecl:
                    case CXCursor_StructDecl: {
                        insert_at_end(closure_data.classes, [&] { return make_class(cursor, closure_data); });
                        break;
                    }

                    case CXCursor_EnumDecl: {
                        insert_at_end(closure_data.enums, [&] { return make_enum(cursor, closure_data); });
                        break;
                    }

                    case CXCursor_FunctionTemplate:
                    case CXCursor_FunctionDecl:
                    case CXCursor_CXXMethod: {
                        insert_at_end(closure_data.functions, [&] { return make_function(cursor, closure_data); });
                        break;
                    }

                    default: {
                        return CXChildVisit_Recurse;
                    }
                }

                return CXChildVisit_Continue;
            };

            closure_t closure {data};
            if (visitor(cursor, parent, &closure) == CXChildVisit_Recurse)
            {
                clang_visitChildren(cursor, visitor, &closure);
            }
        }
    }

    struct ast_parser_clang : ast_parser
    {
        std::vector<std::string> args;
        std::vector<const char*> args_view;
        CXIndex clang_index;
        bool print_diagnostics;

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
            print_diagnostics = options.debug;
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

            CXTranslationUnit translation_unit = nullptr;
            CXErrorCode error = clang_parseTranslationUnit2(
                clang_index, source_file.data(), args_view.data(), static_cast<int>(args_view.size()), source_files.data(), static_cast<unsigned long>(source_files.size()), flags, &translation_unit);

            if (error != CXError_Success)
            {
                SPDLOG_ERROR("unable to parse translation unit, code={}", static_cast<int>(error));
                return false;
            }

            if (print_diagnostics)
            {
                std::size_t diagnostics_count = clang_getNumDiagnostics(translation_unit);
                for (std::size_t diagnostics_index = 0; diagnostics_index < diagnostics_count; ++diagnostics_index)
                {
                    CXDiagnostic diagnostic = clang_getDiagnostic(translation_unit, diagnostics_index);
                    defer dispose_diagnostic = [&] { clang_disposeDiagnostic(diagnostic); };
                    std::string diagnostic_str = detail::to_string(clang_formatDiagnostic(diagnostic, clang_defaultDiagnosticDisplayOptions()));
                    std::puts(diagnostic_str.data());
                }
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
                    detail::make_file(cursor, parent, closure_ast_files[index]);
                }

                return CXChildVisit_Continue;
            };

            closure_t closure {ast_files, file_map};
            clang_visitChildren(cursor, visitor, &closure);
            return true;
        }
    };
}
