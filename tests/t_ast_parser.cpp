#include <filesystem>

#include "catch2/catch_all.hpp"

#include "spore/codegen/ast/ast_file.hpp"
#include "spore/codegen/ast/parsers/ast_parser_cppast.hpp"

namespace details
{
    template <typename ast_parser_t>
    std::shared_ptr<ast_parser_t> make_parser()
    {
        if constexpr (std::is_same_v<ast_parser_t, spore::codegen::ast_parser_cppast>)
        {
            return std::make_shared<ast_parser_t>(cppast::libclang_compile_config());
        }

        std::abort();
    }

    std::string get_input_file()
    {
        std::filesystem::path current_file = __FILE__;
        std::filesystem::path current_dir = current_file.parent_path();
        std::filesystem::path input_file = current_dir / "t_ast_parser_input.hpp";
        return input_file.string();
    }
}

TEMPLATE_TEST_CASE("spore::codegen::ast_parser", "[spore::codegen][spore::codegen::ast_parser]", spore::codegen::ast_parser_cppast)
{
    using ast_parser_t = TestType;

    spore::codegen::ast_file file;
    std::shared_ptr<ast_parser_t> parser = details::make_parser<ast_parser_t>();

    REQUIRE(parser->parse_file(details::get_input_file(), file));
    REQUIRE(file.enums.size() == 1);
    REQUIRE(file.classes.size() == 3);
    REQUIRE(file.functions.size() == 3);

    SECTION("parse include is feature complete")
    {
        REQUIRE(file.includes.size() == 1);
        REQUIRE(file.includes[0].type == spore::codegen::ast_include_type::system);
        REQUIRE(file.includes[0].name == "string");
        REQUIRE_FALSE(file.includes[0].path.empty());
    }

    SECTION("parse enum is feature complete")
    {
        const auto& enum_ = file.enums[0];

        REQUIRE(enum_.name == "_enum");
        REQUIRE(enum_.scope == "_namespace1::_namespace2");
        REQUIRE(enum_.full_name() == "_namespace1::_namespace2::_enum");
        REQUIRE(enum_.attributes.size() == 1);
        REQUIRE(enum_.attributes[0].name == "_enum_attribute");
        REQUIRE(enum_.attributes[0].values.find("key1") != enum_.attributes[0].values.end());
        REQUIRE(enum_.attributes[0].values.at("key1") == "value1");
        REQUIRE(enum_.attributes[0].values.find("key2") != enum_.attributes[0].values.end());
        REQUIRE(enum_.attributes[0].values.at("key2") == "true");

        SECTION("parse enum values is feature complete")
        {
            REQUIRE(enum_.enum_values.size() == 2);

            REQUIRE(enum_.enum_values[0].name == "_value1");
            REQUIRE(enum_.enum_values[0].value.has_value() == false);
            REQUIRE(enum_.enum_values[0].attributes.size() == 1);
            REQUIRE(enum_.enum_values[0].attributes[0].name == "_enum_value_attribute");

            REQUIRE(enum_.enum_values[1].name == "_value2");
            REQUIRE(enum_.enum_values[1].value == "42");
        }
    }

    SECTION("parse class is feature complete")
    {
        const auto& class_ = file.classes[1];

        REQUIRE(class_.name == "_struct");
        REQUIRE(class_.scope == "_namespace1::_namespace2");
        REQUIRE(class_.full_name() == "_namespace1::_namespace2::_struct");
        REQUIRE(class_.type == spore::codegen::ast_class_type::struct_);
        REQUIRE(class_.attributes.size() == 1);
        REQUIRE(class_.attributes[0].name == "_struct_attribute");

        SECTION("parse class base is feature complete")
        {
            REQUIRE(class_.bases.size() == 1);
            REQUIRE(class_.bases[0].name == "_base");
        }

        SECTION("parse class field is feature complete")
        {
            REQUIRE(class_.fields.size() == 2);

            REQUIRE(class_.fields[0].name == "_i");
            REQUIRE(class_.fields[0].type.name == "int");
            REQUIRE(class_.fields[0].default_value == "42");
            REQUIRE(class_.fields[0].attributes.size() == 1);
            REQUIRE(class_.fields[0].attributes[0].name == "_field_attribute");
            // REQUIRE(class_.fields[0].scope == "class_.full_name()");

            REQUIRE(class_.fields[1].name == "_s");
            REQUIRE(class_.fields[1].type.name == "std::string");
            // REQUIRE(class_.fields[1].scope == "class_.full_name()");
        }

        SECTION("parse class constructor is feature complete")
        {
            REQUIRE(class_.constructors.size() == 1);

            REQUIRE(class_.constructors[0].attributes.size() == 1);
            REQUIRE(class_.constructors[0].attributes[0].name == "_constructor_attribute");
            REQUIRE(class_.constructors[0].arguments.size() == 1);
            REQUIRE(class_.constructors[0].arguments[0].name == "_arg");
            REQUIRE(class_.constructors[0].arguments[0].type.name == "int");
            REQUIRE(class_.constructors[0].arguments[0].attributes.size() == 1);
            REQUIRE(class_.constructors[0].arguments[0].attributes[0].name == "_argument_attribute");
            REQUIRE(class_.constructors[0].arguments[0].default_value.has_value() == false);
        }

        SECTION("parse class function is feature complete")
        {
            REQUIRE(class_.functions.size() == 2);

            REQUIRE(class_.functions[0].name == "_member_func");
            REQUIRE(class_.functions[0].return_type.name == "int");
            REQUIRE(class_.functions[0].attributes.size() == 1);
            REQUIRE(class_.functions[0].attributes[0].name == "_func_attribute");
            REQUIRE(class_.functions[0].arguments.size() == 1);
            REQUIRE(class_.functions[0].arguments[0].name == "_arg");
            REQUIRE(class_.functions[0].arguments[0].type.name == "int");
            REQUIRE(class_.functions[0].arguments[0].attributes.size() == 1);
            REQUIRE(class_.functions[0].arguments[0].attributes[0].name == "_argument_attribute");
            REQUIRE(class_.functions[0].arguments[0].default_value.has_value() == false);

            REQUIRE(class_.functions[1].name == "_template_member_func");
            REQUIRE(class_.functions[1].return_type.name == "_value_t");
            REQUIRE(class_.functions[1].attributes.size() == 1);
            REQUIRE(class_.functions[1].attributes[0].name == "_func_attribute");
            REQUIRE(class_.functions[1].template_params.size() == 2);
            REQUIRE(class_.functions[1].template_params[0].type == "typename");
            REQUIRE(class_.functions[1].template_params[0].name == "_value_t");
            REQUIRE(class_.functions[1].template_params[0].default_value.has_value() == false);
            REQUIRE(class_.functions[1].template_params[1].type == "int");
            REQUIRE(class_.functions[1].template_params[1].name == "_n");
            REQUIRE(class_.functions[1].template_params[1].default_value.has_value());
            REQUIRE(class_.functions[1].template_params[1].default_value == "42");
            REQUIRE(class_.functions[1].arguments.size() == 1);
            REQUIRE(class_.functions[1].arguments[0].name == "_arg");
            REQUIRE(class_.functions[1].arguments[0].type.name == "_value_t");
            REQUIRE(class_.functions[1].arguments[0].attributes.size() == 1);
            REQUIRE(class_.functions[1].arguments[0].attributes[0].name == "_argument_attribute");
            REQUIRE(class_.functions[1].arguments[0].default_value.has_value() == false);
        }
    }

    SECTION("parse class template is feature complete")
    {
        const auto& class_ = file.classes[2];

        REQUIRE(class_.name == "_struct_template");
        REQUIRE(class_.scope == "_namespace1::_namespace2");
        REQUIRE(class_.full_name() == "_namespace1::_namespace2::_struct_template<_value_t, _n>");
        REQUIRE(class_.type == spore::codegen::ast_class_type::struct_);
        REQUIRE(class_.attributes.size() == 1);
        REQUIRE(class_.attributes[0].name == "_struct_attribute");
        REQUIRE(class_.bases.size() == 1);
        REQUIRE(class_.bases[0].name == "_base");
        REQUIRE(class_.template_params.size() == 2);
        REQUIRE(class_.template_params[0].type == "typename");
        REQUIRE(class_.template_params[0].name == "_value_t");
        REQUIRE(class_.template_params[0].default_value.has_value() == false);
        REQUIRE(class_.template_params[1].type == "int");
        REQUIRE(class_.template_params[1].name == "_n");
        REQUIRE(class_.template_params[1].default_value.has_value());
        REQUIRE(class_.template_params[1].default_value == "42");

        REQUIRE(class_.fields.size() == 1);

        REQUIRE(class_.fields[0].name == "_value");
        REQUIRE(class_.fields[0].type.name == "_value_t");
        REQUIRE(class_.fields[0].default_value.has_value() == false);
        REQUIRE(class_.fields[0].attributes.size() == 1);
        REQUIRE(class_.fields[0].attributes[0].name == "_field_attribute");
    }

    SECTION("parse free function is feature complete")
    {
        REQUIRE(file.functions.size() == 3);

        REQUIRE(file.functions[0].name == "_free_func");
        REQUIRE(file.functions[0].scope == "_namespace1::_namespace2");
        REQUIRE(file.functions[0].full_name() == "_namespace1::_namespace2::_free_func");
        REQUIRE(file.functions[0].return_type.name == "int");
        REQUIRE(file.functions[0].attributes.size() == 1);
        REQUIRE(file.functions[0].attributes[0].name == "_func_attribute");
        REQUIRE(file.functions[0].arguments.size() == 1);
        REQUIRE(file.functions[0].arguments[0].name == "_arg");
        REQUIRE(file.functions[0].arguments[0].type.name == "int");
        REQUIRE(file.functions[0].arguments[0].default_value.has_value());
        REQUIRE(file.functions[0].arguments[0].default_value.value() == "42");
        REQUIRE(file.functions[0].arguments[0].attributes.size() == 1);
        REQUIRE(file.functions[0].arguments[0].attributes[0].name == "_argument_attribute");

        REQUIRE(file.functions[1].name == "_template_free_func");
        REQUIRE(file.functions[1].return_type.name == "_value_t");
        REQUIRE(file.functions[1].attributes.size() == 1);
        REQUIRE(file.functions[1].attributes[0].name == "_func_attribute");
        REQUIRE(file.functions[1].template_params.size() == 2);
        REQUIRE(file.functions[1].template_params[0].type == "typename");
        REQUIRE(file.functions[1].template_params[0].name == "_value_t");
        REQUIRE(file.functions[1].template_params[0].default_value.has_value() == false);
        REQUIRE(file.functions[1].template_params[1].type == "int");
        REQUIRE(file.functions[1].template_params[1].name == "_n");
        REQUIRE(file.functions[1].template_params[1].default_value.has_value());
        REQUIRE(file.functions[1].template_params[1].default_value == "42");
        REQUIRE(file.functions[1].arguments.size() == 1);
        REQUIRE(file.functions[1].arguments[0].name == "_arg");
        REQUIRE(file.functions[1].arguments[0].type.name == "_value_t");
        REQUIRE(file.functions[1].arguments[0].attributes.size() == 1);
        REQUIRE(file.functions[1].arguments[0].attributes[0].name == "_argument_attribute");
        REQUIRE(file.functions[1].arguments[0].default_value.has_value() == false);

        REQUIRE(file.functions[2].name == "_global_func");
        REQUIRE(file.functions[2].scope.empty());
        REQUIRE(file.functions[2].full_name() == "_global_func");
        REQUIRE(file.functions[2].return_type.name == "void");
        REQUIRE(file.functions[2].attributes.empty());
        REQUIRE(file.functions[2].arguments.empty());
    }
}
