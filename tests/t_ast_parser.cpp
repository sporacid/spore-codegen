#include <filesystem>

#include "catch2/catch_all.hpp"

#include "spore/codegen/ast/ast_file.hpp"
#include "spore/codegen/ast/parsers/ast_parser_clang.hpp"
#include "spore/codegen/codegen_options.hpp"

namespace spore::codegen::detail
{
    template <typename ast_parser_t>
    std::shared_ptr<ast_parser_t> make_parser()
    {
        constexpr std::string_view args[] {
            "-std=c++20",
        };
        return std::make_shared<ast_parser_t>(args);
    }

    std::string get_input_file()
    {
        std::filesystem::path current_file = __FILE__;
        std::filesystem::path current_dir = current_file.parent_path();
        std::filesystem::path input_file = current_dir / "t_ast_parser_input.hpp";
        return input_file.string();
    }
}

TEMPLATE_TEST_CASE("spore::codegen::ast_parser", "[spore::codegen][spore::codegen::ast_parser]", spore::codegen::ast_parser_clang)
{
    using namespace spore::codegen;

    std::shared_ptr<TestType> parser = detail::make_parser<TestType>();
    std::vector<std::string> input_files {detail::get_input_file()};
    std::vector<ast_file> ast_files;

    REQUIRE(parser->parse_files(input_files, ast_files));
    REQUIRE(ast_files.size() == input_files.size());

    ast_file& ast_file = ast_files.at(0);

    REQUIRE(ast_file.classes.size() == 7);
    REQUIRE(ast_file.functions.size() == 3);
    REQUIRE(ast_file.enums.size() == 2);

    SECTION("parse enum is feature complete")
    {
        const auto& enum_ = ast_file.enums[0];

        REQUIRE(enum_.name == "_enum");
        REQUIRE(enum_.scope == "_namespace1::_namespace2");
        REQUIRE(enum_.type == ast_enum_type::enum_class);
        REQUIRE(enum_.full_name() == "_namespace1::_namespace2::_enum");
        REQUIRE(enum_.attributes.size() == 1);
        REQUIRE(enum_.attributes.contains("_enum"));
        REQUIRE(enum_.attributes["_enum"] == true);

        SECTION("parse enum values is feature complete")
        {
            REQUIRE(enum_.attributes.size() == 1);
            REQUIRE(enum_.attributes.contains("_enum"));
            REQUIRE(enum_.attributes["_enum"] == true);

            REQUIRE(enum_.values.size() == 2);
            REQUIRE(enum_.values[0].name == "_value1");
            REQUIRE(enum_.values[0].value == 0);
            REQUIRE(enum_.values[0].attributes.size() == 1);
            REQUIRE(enum_.values[0].attributes.contains("_enum_value"));
            REQUIRE(enum_.values[0].attributes["_enum_value"] == true);
            REQUIRE(enum_.values[1].name == "_value2");
            REQUIRE(enum_.values[1].value == 42);
        }
    }

    SECTION("parse class is feature complete")
    {
        const auto& class_ = ast_file.classes[1];

        REQUIRE(class_.name == "_struct");
        REQUIRE(class_.scope == "_namespace1::_namespace2");
        REQUIRE(class_.full_name() == "_namespace1::_namespace2::_struct");
        REQUIRE(class_.type == ast_class_type::struct_);
        REQUIRE(class_.attributes.size() == 1);
        REQUIRE(class_.attributes.contains("_struct"));
        REQUIRE(class_.attributes["_struct"] == true);

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
            REQUIRE(class_.fields[0].attributes.contains("_field"));
            REQUIRE(class_.fields[0].attributes["_field"] == true);

            REQUIRE(class_.fields[1].name == "_f");
            REQUIRE(class_.fields[1].type.name == "float");
        }

        SECTION("parse class constructor is feature complete")
        {
            REQUIRE(class_.constructors.size() == 2);

            REQUIRE(class_.constructors[0].arguments.empty());
            REQUIRE(class_.constructors[0].attributes.size() == 1);
            REQUIRE(class_.constructors[0].attributes.contains("_constructor"));
            REQUIRE(class_.constructors[0].attributes["_constructor"] == true);
            REQUIRE((class_.constructors[0].flags & ast_flags::default_) == ast_flags::default_);

            REQUIRE(class_.constructors[1].attributes.size() == 1);
            REQUIRE(class_.constructors[1].attributes.contains("_constructor"));
            REQUIRE(class_.constructors[1].attributes["_constructor"] == true);
            REQUIRE(class_.constructors[1].arguments.size() == 1);
            REQUIRE(class_.constructors[1].arguments[0].name == "_arg");
            REQUIRE(class_.constructors[1].arguments[0].type.name == "int");
            REQUIRE(class_.constructors[1].arguments[0].attributes.size() == 1);
            REQUIRE(class_.constructors[1].arguments[0].attributes.contains("_argument"));
            REQUIRE(class_.constructors[1].arguments[0].attributes["_argument"] == true);
            REQUIRE(class_.constructors[1].arguments[0].default_value.has_value() == false);
        }

        SECTION("parse class function is feature complete")
        {
            REQUIRE(class_.functions.size() == 2);

            REQUIRE(class_.functions[0].name == "_member_func");
            REQUIRE(class_.functions[0].return_type.name == "int");
            REQUIRE(class_.functions[0].attributes.size() == 1);
            REQUIRE(class_.functions[0].attributes.contains("_function"));
            REQUIRE(class_.functions[0].attributes["_function"] == true);
            REQUIRE(class_.functions[0].arguments.size() == 1);
            REQUIRE(class_.functions[0].arguments[0].name == "_arg");
            REQUIRE(class_.functions[0].arguments[0].type.name == "int");
            REQUIRE(class_.functions[0].arguments[0].attributes.size() == 1);
            REQUIRE(class_.functions[0].arguments[0].attributes.contains("_argument"));
            REQUIRE(class_.functions[0].arguments[0].attributes["_argument"] == true);
            REQUIRE(class_.functions[0].arguments[0].default_value.has_value() == false);

            REQUIRE(class_.functions[1].name == "_template_member_func");
            REQUIRE(class_.functions[1].return_type.name == "_value_t");
            REQUIRE(class_.functions[1].attributes.size() == 1);
            REQUIRE(class_.functions[1].attributes.contains("_function"));
            REQUIRE(class_.functions[1].attributes["_function"] == true);
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
            REQUIRE(class_.functions[1].arguments[0].attributes.contains("_argument"));
            REQUIRE(class_.functions[1].arguments[0].attributes["_argument"] == true);
            REQUIRE(class_.functions[1].arguments[0].default_value.has_value() == false);
        }
    }

    SECTION("parse nested class is feature complete")
    {
        const auto& class_ = ast_file.classes[2];

        REQUIRE(class_.name == "_nested");
        REQUIRE(class_.scope == "_namespace1::_namespace2::_struct");
        REQUIRE(class_.full_name() == "_namespace1::_namespace2::_struct::_nested");
        REQUIRE(class_.type == ast_class_type::struct_);
        REQUIRE(class_.nested);
    }

    SECTION("parse nested enum is feature complete")
    {
        const auto& enum_ = ast_file.enums[1];

        REQUIRE(enum_.name == "_nested_enum");
        REQUIRE(enum_.scope == "_namespace1::_namespace2::_struct");
        REQUIRE(enum_.full_name() == "_namespace1::_namespace2::_struct::_nested_enum");
        REQUIRE(enum_.type == ast_enum_type::enum_);
        REQUIRE(enum_.nested);
    }

    SECTION("parse class template is feature complete")
    {
        const auto& class_ = ast_file.classes[3];

        REQUIRE(class_.name == "_struct_template");
        REQUIRE(class_.scope == "_namespace1::_namespace2");
        REQUIRE(class_.full_name() == "_namespace1::_namespace2::_struct_template<_value_t, _n>");
        REQUIRE(class_.type == ast_class_type::struct_);
        REQUIRE(class_.attributes.size() == 1);
        REQUIRE(class_.attributes.contains("_struct"));
        REQUIRE(class_.attributes["_struct"] == true);
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
        REQUIRE(class_.fields[0].attributes.contains("_field"));
        REQUIRE(class_.fields[0].attributes["_field"] == true);
    }

    SECTION("parse nested class template is feature complete")
    {
        const auto& class_ = ast_file.classes[4];

        REQUIRE(class_.name == "_nested_template");
        REQUIRE(class_.scope == "_namespace1::_namespace2::_struct_template<_value_t, _n>");
        REQUIRE(class_.full_name() == "_namespace1::_namespace2::_struct_template<_value_t, _n>::_nested_template<_nested_value_t>");
        REQUIRE(class_.type == ast_class_type::struct_);
        REQUIRE(class_.template_params[0].type == "typename");
        REQUIRE(class_.template_params[0].name == "_nested_value_t");
        REQUIRE(class_.template_params[0].default_value.has_value());
        REQUIRE(class_.template_params[0].default_value.value() == "int");
        REQUIRE(class_.nested);
    }

    SECTION("parse free function is feature complete")
    {
        REQUIRE(ast_file.functions.size() == 3);

        REQUIRE(ast_file.functions[0].name == "_free_func");
        REQUIRE(ast_file.functions[0].scope == "_namespace1::_namespace2");
        REQUIRE(ast_file.functions[0].full_name() == "_namespace1::_namespace2::_free_func");
        REQUIRE(ast_file.functions[0].return_type.name == "int");
        REQUIRE(ast_file.functions[0].attributes.size() == 1);
        REQUIRE(ast_file.functions[0].attributes.contains("_function"));
        REQUIRE(ast_file.functions[0].attributes["_function"] == true);
        REQUIRE(ast_file.functions[0].arguments.size() == 1);
        REQUIRE(ast_file.functions[0].arguments[0].name == "_arg");
        REQUIRE(ast_file.functions[0].arguments[0].type.name == "int");
        REQUIRE(ast_file.functions[0].arguments[0].default_value.has_value());
        REQUIRE(ast_file.functions[0].arguments[0].default_value.value() == "42");
        REQUIRE(ast_file.functions[0].arguments[0].attributes.size() == 1);
        REQUIRE(ast_file.functions[0].arguments[0].attributes.contains("_argument"));
        REQUIRE(ast_file.functions[0].arguments[0].attributes["_argument"] == true);

        REQUIRE(ast_file.functions[1].name == "_template_free_func");
        REQUIRE(ast_file.functions[1].return_type.name == "_value_t");
        REQUIRE(ast_file.functions[1].attributes.size() == 1);
        REQUIRE(ast_file.functions[1].attributes.contains("_function"));
        REQUIRE(ast_file.functions[1].attributes["_function"] == true);
        REQUIRE(ast_file.functions[1].template_params.size() == 2);
        REQUIRE(ast_file.functions[1].template_params[0].type == "typename");
        REQUIRE(ast_file.functions[1].template_params[0].name == "_value_t");
        REQUIRE(ast_file.functions[1].template_params[0].default_value.has_value() == false);
        REQUIRE(ast_file.functions[1].template_params[1].type == "int");
        REQUIRE(ast_file.functions[1].template_params[1].name == "_n");
        REQUIRE(ast_file.functions[1].template_params[1].default_value.has_value());
        REQUIRE(ast_file.functions[1].template_params[1].default_value == "42");
        REQUIRE(ast_file.functions[1].arguments.size() == 1);
        REQUIRE(ast_file.functions[1].arguments[0].name == "_arg");
        REQUIRE(ast_file.functions[1].arguments[0].type.name == "_value_t");
        REQUIRE(ast_file.functions[1].arguments[0].attributes.size() == 1);
        REQUIRE(ast_file.functions[1].arguments[0].attributes.contains("_argument"));
        REQUIRE(ast_file.functions[1].arguments[0].attributes["_argument"] == true);
        REQUIRE(ast_file.functions[1].arguments[0].default_value.has_value() == false);

        REQUIRE(ast_file.functions[2].name == "_global_func");
        REQUIRE(ast_file.functions[2].scope.empty());
        REQUIRE(ast_file.functions[2].full_name() == "_global_func");
        REQUIRE(ast_file.functions[2].return_type.name == "void");
        REQUIRE(ast_file.functions[2].attributes.empty());
        REQUIRE(ast_file.functions[2].arguments.empty());
    }

    SECTION("parse attributes is feature complete")
    {
        ast_class& class_ = ast_file.classes[5];

        REQUIRE(class_.name == "_attributes");
        REQUIRE(class_.attributes.size() == 5);
        REQUIRE(class_.attributes.contains("a"));
        REQUIRE(class_.attributes["a"] == true);
        REQUIRE(class_.attributes.contains("b"));
        REQUIRE(class_.attributes["b"] == 1);
        REQUIRE(class_.attributes.contains("c"));
        REQUIRE(class_.attributes["c"] == "2");
        REQUIRE(class_.attributes.contains("d"));
        REQUIRE(class_.attributes["d"] == 3.0);

        REQUIRE(class_.attributes.contains("e"));
        REQUIRE(class_.attributes["e"].is_object());
        REQUIRE(class_.attributes["e"].size() == 2);
        REQUIRE(class_.attributes["e"].contains("f"));
        REQUIRE(class_.attributes["e"]["f"] == 4);
        REQUIRE(class_.attributes["e"].contains("g"));
        REQUIRE(class_.attributes["e"]["g"].is_object());
        REQUIRE(class_.attributes["e"]["g"].size() == 1);
        REQUIRE(class_.attributes["e"]["g"].contains("h"));
        REQUIRE(class_.attributes["e"]["g"]["h"] == 5);

        REQUIRE(class_.fields.size() == 1);
        REQUIRE(class_.fields[0].attributes.contains("a"));
        REQUIRE(class_.fields[0].attributes["a"] == false);
    }

    SECTION("parse template is feature complete")
    {
        ast_class& class_ = ast_file.classes[6];

        REQUIRE(class_.name == "_template");
        REQUIRE(class_.template_params.size() == 5);
        REQUIRE(class_.template_params[0].name == "value_t");
        REQUIRE(class_.template_params[0].type == "typename");
        REQUIRE(class_.template_params[1].name == "size_v");
        REQUIRE(class_.template_params[1].type == "int");
        REQUIRE(class_.template_params[2].name == "template_t");
        REQUIRE(class_.template_params[2].type == "template <typename arg_t, typename, int> typename");
        REQUIRE(class_.template_params[3].name == "concept_t");
        REQUIRE(class_.template_params[3].type == "concept_");
        REQUIRE(class_.template_params[4].name == "variadic_t");
        REQUIRE(class_.template_params[4].type == "typename...");
        REQUIRE(class_.template_params[4].is_variadic);
    }
}