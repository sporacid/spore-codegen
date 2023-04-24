#pragma once

#include <algorithm>
#include <string>
#include <unordered_set>
#include <vector>

#include "cppast/cpp_class.hpp"
#include "cppast/cpp_class_template.hpp"
#include "cppast/cpp_entity_kind.hpp"
#include "cppast/cpp_enum.hpp"
#include "cppast/cpp_file.hpp"
#include "cppast/cpp_function.hpp"
#include "cppast/cpp_function_template.hpp"
#include "cppast/cpp_member_function.hpp"
#include "cppast/cpp_member_variable.hpp"
#include "cppast/cpp_namespace.hpp"
#include "cppast/cpp_preprocessor.hpp"
#include "cppast/cpp_template_parameter.hpp"
#include "cppast/cpp_type.hpp"
#include "cppast/cpp_type_alias.hpp"
#include "cppast/libclang_parser.hpp"
#include "cppast/visitor.hpp"
#include "fmt/format.h"
#include "spdlog/spdlog.h"

#include "spore/codegen/ast/parsers/ast_parser.hpp"
#include "spore/codegen/codegen_helpers.hpp"

namespace spore::codegen
{
    namespace details
    {
        struct parser_context
        {
            std::unordered_set<unsigned long> optional_type_ids;
        };

        struct cppast_logger_impl : cppast::diagnostic_logger
        {
            bool do_log(const char* source, const cppast::diagnostic& diagnostic) const override
            {
                std::string location = diagnostic.location.to_string();
                location = !location.empty() ? " at " + location : "";

                if (diagnostic.severity <= cppast::severity::warning)
                {
                    SPDLOG_DEBUG("cppast: {}{}", diagnostic.message, location);
                }
                else
                {
                    SPDLOG_WARN("cppast: {}{}", diagnostic.message, location);
                }

                return true;
            }
        };

        std::string find_scope(const cppast::cpp_entity& cpp_entity)
        {
            std::string scope;

            type_safe::optional_ref<const cppast::cpp_entity> cpp_parent = cpp_entity.parent();

            const bool is_class_template =
                cpp_parent.has_value() &&
                cpp_entity.kind() == cppast::cpp_entity_kind::class_t &&
                cpp_parent.value().kind() == cppast::cpp_entity_kind::class_template_t;

            if (is_class_template)
            {
                cpp_parent = cpp_parent.value().parent();
            }

            while (cpp_parent.has_value())
            {
                switch (cpp_parent.value().kind())
                {
                    case cppast::cpp_entity_kind::class_template_t: {
                        const auto& cpp_class_template = dynamic_cast<const cppast::cpp_class_template&>(cpp_parent.value());

                        scope += "<";

                        for (const cppast::cpp_template_parameter& cpp_template_param : cpp_class_template.parameters())
                        {
                            scope += cpp_template_param.name() + ", ";
                        }

                        if (spore::codegen::ends_with(scope, ", "))
                        {
                            scope.resize(scope.length() - 2);
                        }

                        scope += ">";

                        break;
                    }

                    case cppast::cpp_entity_kind::namespace_t:
                    case cppast::cpp_entity_kind::class_t: {
                        if (!scope.empty())
                        {
                            scope.insert(0, "::");
                        }

                        scope.insert(0, cpp_parent.value().name());

                        break;
                    }

                    default: {
                        break;
                    }
                }

                cpp_parent = cpp_parent.value().parent();
            }

            return scope;
        }

        std::string parse_expression(const cppast::cpp_expression& cpp_expr)
        {
            switch (cpp_expr.kind())
            {
                case cppast::cpp_expression_kind::literal_t: {
                    const auto& cpp_expr_literal = dynamic_cast<const cppast::cpp_literal_expression&>(cpp_expr);
                    return cpp_expr_literal.value();
                }

                case cppast::cpp_expression_kind::unexposed_t: {
                    const auto& cpp_expr_unexposed = dynamic_cast<const cppast::cpp_unexposed_expression&>(cpp_expr);
                    return cpp_expr_unexposed.expression().as_string();
                }

                default: {
                    SPDLOG_WARN("unhandled expression type, type={}", static_cast<std::int32_t>(cpp_expr.kind()));
                    return "";
                }
            }
        }

        ast_template_param parse_template_param(const cppast::cpp_template_parameter& cpp_template_param)
        {
            ast_template_param template_param;
            template_param.name = cpp_template_param.name();

            switch (cpp_template_param.kind())
            {
                case cppast::cpp_entity_kind::template_type_parameter_t: {
                    const auto& cpp_template_type_param = dynamic_cast<const cppast::cpp_template_type_parameter&>(cpp_template_param);
                    template_param.type = cppast::to_string(cpp_template_type_param.keyword());

                    if (cpp_template_type_param.default_type().has_value())
                    {
                        template_param.default_value = cppast::to_string(cpp_template_type_param.default_type().value());
                    }
                    break;
                }
                case cppast::cpp_entity_kind::non_type_template_parameter_t: {
                    const auto& cpp_template_value_param = dynamic_cast<const cppast::cpp_non_type_template_parameter&>(cpp_template_param);
                    template_param.type = cppast::to_string(cpp_template_value_param.type());

                    if (cpp_template_value_param.default_value().has_value())
                    {
                        const cppast::cpp_expression& cpp_expr = cpp_template_value_param.default_value().value();
                        template_param.default_value = parse_expression(cpp_expr);
                    }

                    break;
                }
                default: {
                    type_safe::optional_ref<const cppast::cpp_entity> parent = cpp_template_param.parent();
                    std::string parent_name = parent.has_value() ? parent.value().name() : "";
                    SPDLOG_WARN("unhandled template parameter, parent={} param={}", parent_name, cpp_template_param.name());
                    break;
                }
            }

            return template_param;
        }

        ast_ref parse_ref(const cppast::cpp_type& cpp_type)
        {
            ast_ref ref;
            ref.name = cppast::to_string(cpp_type);
            return ref;
        }

        ast_ref parse_ref(const cppast::cpp_base_class& cpp_base_class)
        {
            ast_ref ref;
            ref.name = cpp_base_class.name();
            return ref;
        }

        ast_attribute parse_attribute(const cppast::cpp_attribute& cpp_attribute)
        {
            ast_attribute attribute;
            attribute.name = cpp_attribute.name();
            attribute.scope = cpp_attribute.scope().value_or(std::string());

            if (cpp_attribute.arguments().has_value())
            {
                const auto add_value = [&](std::string name, std::optional<std::string> value) {
                    attribute.values.emplace(std::move(name), value.has_value() ? std::move(value.value()) : "true");
                };

                std::string name;
                std::optional<std::string> value;
                for (const cppast::cpp_token& cpp_token : cpp_attribute.arguments().value())
                {
                    switch (cpp_token.kind)
                    {
                        case cppast::cpp_token_kind::identifier:
                        case cppast::cpp_token_kind::keyword:
                        case cppast::cpp_token_kind::int_literal:
                        case cppast::cpp_token_kind::float_literal:
                        case cppast::cpp_token_kind::char_literal:
                        case cppast::cpp_token_kind::string_literal:
                            if (name.empty())
                            {
                                name = cpp_token.spelling;
                            }
                            else
                            {
                                value = cpp_token.spelling;

                                if (spore::codegen::starts_with(*value, "\"") || spore::codegen::starts_with(*value, "'"))
                                {
                                    value->erase(0, 1);
                                }

                                if (spore::codegen::ends_with(*value, "\"") || spore::codegen::ends_with(*value, "'"))
                                {
                                    value->erase(value->size() - 1);
                                }
                            }

                            break;
                        case cppast::cpp_token_kind::punctuation:
                            const bool expecting_value = cpp_token.spelling == "=" || cpp_token.spelling == ":";

                            if (!expecting_value && !name.empty())
                            {
                                add_value(std::move(name), std::move(value));

                                name = std::string();
                                value = std::optional<std::string>();
                            }

                            break;
                    }
                }

                if (!name.empty())
                {
                    add_value(std::move(name), std::move(value));
                }
            }

            return attribute;
        }

        bool is_optional(const cppast::cpp_type& cpp_type, const parser_context& context)
        {
            if (cpp_type.kind() == cppast::cpp_type_kind::template_instantiation_t)
            {
                const auto& template_ = static_cast<const cppast::cpp_template_instantiation_type&>(cpp_type).primary_template();

                for (const auto& id : template_.id())
                {
                    if (context.optional_type_ids.find(id.value_) != context.optional_type_ids.end())
                    {
                        return true;
                    }
                }
            }

            return false;
        }

        ast_field parse_field(const cppast::cpp_member_variable& cpp_variable, const parser_context& context)
        {
            ast_field field;
            field.name = cpp_variable.name();
            field.type = parse_ref(cpp_variable.type());
            field.is_optional = is_optional(cpp_variable.type(), context);

            if (cpp_variable.default_value().has_value())
            {
                const cppast::cpp_expression& cpp_expr = cpp_variable.default_value().value();
                field.default_value = parse_expression(cpp_expr);
            }

            std::transform(cpp_variable.attributes().begin(), cpp_variable.attributes().end(), std::back_inserter(field.attributes),
                [&](const cppast::cpp_attribute& cpp_attribute) {
                    return parse_attribute(cpp_attribute);
                });

            return field;
        }

        ast_argument parse_argument(const cppast::cpp_function_parameter& cpp_parameter)
        {
            ast_argument argument;
            argument.name = cpp_parameter.name();
            argument.type = parse_ref(cpp_parameter.type());

            if (cpp_parameter.default_value().has_value())
            {
                argument.default_value = parse_expression(cpp_parameter.default_value().value());
            }

            std::transform(cpp_parameter.attributes().begin(), cpp_parameter.attributes().end(), std::back_inserter(argument.attributes),
                [&](const cppast::cpp_attribute& cpp_attribute) {
                    return parse_attribute(cpp_attribute);
                });

            return argument;
        }

        ast_function parse_function(const cppast::cpp_function_base& cpp_function_base, const cppast::cpp_template* cpp_template = nullptr)
        {
            ast_function function;
            function.name = cpp_function_base.name();
            function.scope = find_scope(cpp_function_base);

            switch (cpp_function_base.kind())
            {
                case cppast::cpp_entity_kind::function_t: {
                    const auto& cpp_function = dynamic_cast<const cppast::cpp_function&>(cpp_function_base);

                    function.return_type = parse_ref(cpp_function.return_type());

                    if (cppast::is_static(cpp_function.storage_class()))
                    {
                        function.flags = function.flags | ast_flags::static_;
                    }

                    break;
                }
                case cppast::cpp_entity_kind::member_function_t: {
                    const auto& cpp_member_function = dynamic_cast<const cppast::cpp_member_function&>(cpp_function_base);

                    function.return_type = parse_ref(cpp_member_function.return_type());

                    if (cppast::is_const(cpp_member_function.cv_qualifier()))
                    {
                        function.flags = function.flags | ast_flags::const_;
                    }

                    if (cppast::is_volatile(cpp_member_function.cv_qualifier()))
                    {
                        function.flags = function.flags | ast_flags::volatile_;
                    }

                    break;
                }
                default: {
                    break;
                }
            }

            std::transform(cpp_function_base.attributes().begin(), cpp_function_base.attributes().end(), std::back_inserter(function.attributes),
                [&](const cppast::cpp_attribute& cpp_attribute) {
                    return parse_attribute(cpp_attribute);
                });

            std::transform(cpp_function_base.parameters().begin(), cpp_function_base.parameters().end(), std::back_inserter(function.arguments),
                [&](const cppast::cpp_function_parameter& cpp_parameter) {
                    return parse_argument(cpp_parameter);
                });

            if (cpp_template != nullptr)
            {
                std::transform(cpp_template->parameters().begin(), cpp_template->parameters().end(), std::back_inserter(function.template_params),
                    [&](const cppast::cpp_template_parameter& cpp_template_param) {
                        return parse_template_param(cpp_template_param);
                    });
            }

            return function;
        }

        ast_constructor parse_constructor(const cppast::cpp_constructor& cpp_constructor, const cppast::cpp_template* cpp_template = nullptr)
        {
            ast_constructor constructor;

            std::transform(cpp_constructor.attributes().begin(), cpp_constructor.attributes().end(), std::back_inserter(constructor.attributes),
                [&](const cppast::cpp_attribute& cpp_attribute) {
                    return parse_attribute(cpp_attribute);
                });

            std::transform(cpp_constructor.parameters().begin(), cpp_constructor.parameters().end(), std::back_inserter(constructor.arguments),
                [&](const cppast::cpp_function_parameter& cpp_parameter) {
                    return parse_argument(cpp_parameter);
                });

            if (cpp_template != nullptr)
            {
                std::transform(cpp_template->parameters().begin(), cpp_template->parameters().end(), std::back_inserter(constructor.template_params),
                    [&](const cppast::cpp_template_parameter& cpp_template_param) {
                        return parse_template_param(cpp_template_param);
                    });
            }

            return constructor;
        }

        void parse_classes(const cppast::cpp_class_template& cpp_class, std::vector<ast_class>& classes, const parser_context& context);

        void parse_classes(const cppast::cpp_class& cpp_class, std::vector<ast_class>& classes, const parser_context& context)
        {
            const std::size_t insert_index = classes.size();

            ast_class class_;
            class_.name = cpp_class.name();
            class_.scope = find_scope(cpp_class);

            switch (cpp_class.class_kind())
            {
                case cppast::cpp_class_kind::class_t:
                    class_.type = ast_class_type::class_;
                    break;
                case cppast::cpp_class_kind::struct_t:
                    class_.type = ast_class_type::struct_;
                    break;
                default:
                    SPDLOG_WARN("unhandled class type, class={} type={}", cpp_class.name(), cppast::to_string(cpp_class.class_kind()));
                    break;
            }

            std::transform(cpp_class.attributes().begin(), cpp_class.attributes().end(), std::back_inserter(class_.attributes),
                [&](const cppast::cpp_attribute& cpp_attribute) {
                    return parse_attribute(cpp_attribute);
                });

            std::transform(cpp_class.bases().begin(), cpp_class.bases().end(), std::back_inserter(class_.bases),
                [&](const cppast::cpp_base_class& cpp_base_class) {
                    return parse_ref(cpp_base_class);
                });

            const auto is_definition = [](const cppast::cpp_function_base& cpp_function) {
                return cpp_function.body_kind() == cppast::cpp_function_definition;
            };

            for (const cppast::cpp_entity& cpp_entity : cpp_class)
            {
                switch (cpp_entity.kind())
                {
                    case cppast::cpp_entity_kind::class_t: {
                        const auto& cpp_class_inner = dynamic_cast<const cppast::cpp_class&>(cpp_entity);
                        parse_classes(cpp_class_inner, classes, context);
                        break;
                    }

                    case cppast::cpp_entity_kind::class_template_t: {
                        const auto& cpp_class_inner = dynamic_cast<const cppast::cpp_class_template&>(cpp_entity);
                        parse_classes(cpp_class_inner, classes, context);
                        break;
                    }

                    case cppast::cpp_entity_kind::member_variable_t: {
                        const auto& cpp_variable = dynamic_cast<const cppast::cpp_member_variable&>(cpp_entity);
                        class_.fields.emplace_back(parse_field(cpp_variable, context));
                        break;
                    }

                    case cppast::cpp_entity_kind::constructor_t: {
                        const auto& cpp_constructor = dynamic_cast<const cppast::cpp_constructor&>(cpp_entity);

                        if (is_definition(cpp_constructor))
                        {
                            class_.constructors.emplace_back(parse_constructor(cpp_constructor));
                        }

                        break;
                    }

                    case cppast::cpp_entity_kind::function_t:
                    case cppast::cpp_entity_kind::member_function_t: {
                        const auto& cpp_function = dynamic_cast<const cppast::cpp_function_base&>(cpp_entity);

                        if (is_definition(cpp_function))
                        {
                            class_.functions.emplace_back(parse_function(cpp_function));
                        }

                        break;
                    }

                    case cppast::cpp_entity_kind::function_template_t: {
                        const auto& cpp_function_template = dynamic_cast<const cppast::cpp_function_template&>(cpp_entity);

                        if (is_definition(cpp_function_template.function()))
                        {
                            if (cpp_function_template.function().kind() == cppast::cpp_entity_kind::constructor_t)
                            {
                                const auto& cpp_constructor = dynamic_cast<const cppast::cpp_constructor&>(cpp_function_template.function());
                                class_.constructors.emplace_back(parse_constructor(cpp_constructor, &cpp_function_template));
                            }
                            else
                            {
                                class_.functions.emplace_back(parse_function(cpp_function_template.function(), &cpp_function_template));
                            }
                        }

                        break;
                    }

                    default: {
                        break;
                    }
                }
            }

            classes.insert(classes.begin() + insert_index, std::move(class_));
        }

        void parse_classes(const cppast::cpp_class_template& cpp_class, std::vector<ast_class>& classes, const parser_context& context)
        {
            const std::size_t insert_index = classes.size();

            parse_classes(cpp_class.class_(), classes, context);

            ast_class& class_ = classes.at(insert_index);

            std::transform(cpp_class.parameters().begin(), cpp_class.parameters().end(), std::back_inserter(class_.template_params),
                [&](const cppast::cpp_template_parameter& cpp_template_param) {
                    return parse_template_param(cpp_template_param);
                });
        }

        ast_enum_value parse_enum_value(const cppast::cpp_enum_value& cpp_enum_value)
        {
            ast_enum_value enum_value;
            enum_value.name = cpp_enum_value.name();

            if (cpp_enum_value.value().has_value())
            {
                const cppast::cpp_expression& cpp_enum_value_expr = cpp_enum_value.value().value();

                if (cpp_enum_value_expr.kind() == cppast::cpp_expression_kind::literal_t)
                {
                    const auto& cpp_enum_value_expr_literal = dynamic_cast<const cppast::cpp_literal_expression&>(cpp_enum_value_expr);
                    enum_value.value = cpp_enum_value_expr_literal.value();
                }
                else
                {
                    type_safe::optional_ref<const cppast::cpp_entity> parent = cpp_enum_value.parent();
                    std::string parent_name = parent.has_value() ? parent.value().name() : "";
                    SPDLOG_WARN("unhandled enum value expression, enum={} name={}", parent_name, cpp_enum_value.name());
                }
            }

            std::transform(cpp_enum_value.attributes().begin(), cpp_enum_value.attributes().end(), std::back_inserter(enum_value.attributes),
                [&](const cppast::cpp_attribute& cpp_attribute) {
                    return parse_attribute(cpp_attribute);
                });

            return enum_value;
        }

        ast_enum parse_enum(const cppast::cpp_enum& cpp_enum)
        {
            ast_enum enum_;
            enum_.name = cpp_enum.name();
            enum_.scope = find_scope(cpp_enum);

            std::transform(cpp_enum.attributes().begin(), cpp_enum.attributes().end(), std::back_inserter(enum_.attributes),
                [&](const cppast::cpp_attribute& cpp_attribute) {
                    return parse_attribute(cpp_attribute);
                });

            for (const cppast::cpp_enum_value& cpp_enum_value : cpp_enum)
            {
                enum_.enum_values.emplace_back(parse_enum_value(cpp_enum_value));
            }

            return enum_;
        }

        ast_include parse_include(const cppast::cpp_include_directive& cpp_include)
        {
            ast_include include;
            include.name = cpp_include.name();
            include.path = cpp_include.full_path();

            switch (cpp_include.include_kind())
            {
                case cppast::cpp_include_kind::system:
                    include.type = ast_include_type::system;
                    break;
                case cppast::cpp_include_kind::local:
                    include.type = ast_include_type::local;
                    break;
                default:
                    SPDLOG_WARN("unhandled include directive type, value={}", static_cast<std::int32_t>(cpp_include.include_kind()));
                    break;
            }

            return include;
        }

        ast_file parse_file(const cppast::cpp_file& cpp_file)
        {
            ast_file file;
            file.path = cpp_file.name();
            parser_context context;

            const auto predicate = [&](const cppast::cpp_entity& cpp_entity) {
                const bool is_definition = cppast::is_definition(cpp_entity);
                const bool is_enum = is_definition && cpp_entity.kind() == cppast::cpp_entity_kind::enum_t;
                const bool is_class = is_definition && (cpp_entity.kind() == cppast::cpp_entity_kind::class_t || cpp_entity.kind() == cppast::cpp_entity_kind::class_template_t);
                const bool is_function = is_definition && (cpp_entity.kind() == cppast::cpp_entity_kind::function_t || cpp_entity.kind() == cppast::cpp_entity_kind::function_template_t);
                const bool is_include = cpp_entity.kind() == cppast::cpp_entity_kind::include_directive_t;
                const bool is_type_alias = cpp_entity.kind() == cppast::cpp_entity_kind::type_alias_t;
                return is_enum || is_class || is_function || is_include || is_type_alias;
            };

            const auto action = [&](const cppast::cpp_entity& cpp_entity, const cppast::visitor_info& cppast_info) {
                if (cppast_info.is_new_entity())
                {
                    switch (cpp_entity.kind())
                    {
                        case cppast::cpp_entity_kind::enum_t: {
                            const auto& cpp_enum = dynamic_cast<const cppast::cpp_enum&>(cpp_entity);
                            file.enums.emplace_back(parse_enum(cpp_enum));
                            break;
                        }
                        case cppast::cpp_entity_kind::class_template_t: {
                            const auto& cpp_class_template = dynamic_cast<const cppast::cpp_class_template&>(cpp_entity);
                            parse_classes(cpp_class_template, file.classes, context);
                            break;
                        }
                        case cppast::cpp_entity_kind::class_t: {
                            const auto& cpp_class = dynamic_cast<const cppast::cpp_class&>(cpp_entity);
                            parse_classes(cpp_class, file.classes, context);
                            break;
                        }
                        case cppast::cpp_entity_kind::function_template_t: {
                            const auto& cpp_function_template = dynamic_cast<const cppast::cpp_function_template&>(cpp_entity);
                            file.functions.emplace_back(parse_function(cpp_function_template.function(), &cpp_function_template));
                            break;
                        }
                        case cppast::cpp_entity_kind::function_t: {
                            const auto& cpp_function = dynamic_cast<const cppast::cpp_function_base&>(cpp_entity);
                            file.functions.emplace_back(parse_function(cpp_function));
                            break;
                        }
                        case cppast::cpp_entity_kind::include_directive_t: {
                            const auto& cpp_include = dynamic_cast<const cppast::cpp_include_directive&>(cpp_entity);
                            file.includes.emplace_back(parse_include(cpp_include));
                            break;
                        }
                        case cppast::cpp_entity_kind::type_alias_t: {
                            const auto& cpp_type_alias = dynamic_cast<const cppast::cpp_type_alias&>(cpp_entity);
                            const auto& underlying_type = cpp_type_alias.underlying_type();

                            if (underlying_type.kind() == cppast::cpp_type_kind::template_instantiation_t)
                            {
                                const auto& tmpl_instantiation = static_cast<const cppast::cpp_template_instantiation_type&>(underlying_type);
                                if (tmpl_instantiation.primary_template().name() == "std::optional")
                                {
                                    for (const auto& id : tmpl_instantiation.primary_template().id())
                                    {
                                        context.optional_type_ids.emplace(id);
                                    }
                                }
                            }
                            break;
                        }
                        default:
                            break;
                    }
                }

                const bool result = cppast_info.event != cppast::visitor_info::container_entity_enter;
                return result;
            };

            cppast::visit(cpp_file, predicate, action);

            return file;
        }
    }

    struct ast_parser_cppast : ast_parser
    {
        cppast::libclang_compile_config cppast_config;

        explicit ast_parser_cppast(const cppast::libclang_compile_config& cppast_config)
            : cppast_config(cppast_config),
              cppast_parser {
                  type_safe::ref(cppast_index),
                  type_safe::ref(cppast_logger),
              }
        {
        }

        bool parse_file(const std::string& path, ast_file& file) override
        {
            type_safe::optional_ref<const cppast::cpp_file> cppast_file = cppast_parser.parse(path, cppast_config);
            if (!cppast_file.has_value() || cppast_parser.error())
            {
                cppast_parser.reset_error();
                SPDLOG_ERROR("unable to parse cpp file, path={}", path);
                return false;
            }

            try
            {
                file = details::parse_file(cppast_file.value());
                return true;
            }
            catch (const cppast::libclang_error& err)
            {
                std::string cause = std::strlen(err.what()) > 0 ? fmt::format(" error={}", err.what()) : "";
                SPDLOG_ERROR("failed to parse cpp file, path={}{}", path, cause);
                return false;
            }
        }

      private:
        details::cppast_logger_impl cppast_logger;
        cppast::cpp_entity_index cppast_index;
        cppast::simple_file_parser<cppast::libclang_parser> cppast_parser;
    };
}
