#include <filesystem>
#include <source_location>

#include "catch2/catch_all.hpp"

#include "spore/codegen/parsers/cpp/codegen_parser_cpp.hpp"

namespace spore::codegen::detail
{
    std::string get_cpp_file()
    {
        constexpr std::source_location source_location = std::source_location::current();
        const std::filesystem::path current_file = source_location.file_name();
        const std::filesystem::path current_dir = current_file.parent_path();
        const std::filesystem::path input_file = current_dir / "t_codegen_parser_cpp_data.hpp";
        return input_file.string();
    }
}

TEST_CASE("spore::codegen::codegen_parser_cpp", "[spore::codegen][spore::codegen::codegen_parser_cpp]")
{
    using namespace spore::codegen;

    constexpr std::string_view parser_args[] {"-std=c++20"};

    codegen_parser_cpp parser {parser_args};
    std::vector input_files {detail::get_cpp_file()};
    std::vector<cpp_file> cpp_files;

    REQUIRE(parser.parse_asts(input_files, cpp_files));
    REQUIRE(cpp_files.size() == input_files.size());

    cpp_file& cpp_file = cpp_files[0];

    REQUIRE(cpp_file.classes.size() == 14);
    REQUIRE(cpp_file.functions.size() == 3);
    REQUIRE(cpp_file.enums.size() == 2);
    REQUIRE(cpp_file.variables.size() == 9);

    SECTION("parse enum is feature complete")
    {
        const auto& enum_ = cpp_file.enums[0];

        REQUIRE(enum_.name == "_enum");
        REQUIRE(enum_.outer_scope == "_namespace1::_namespace2");
        REQUIRE(enum_.inner_scope.empty());
        REQUIRE(enum_.scope() == "_namespace1::_namespace2");
        REQUIRE(enum_.full_name() == "_namespace1::_namespace2::_enum");
        REQUIRE(enum_.type == cpp_enum_type::enum_class);
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
        const auto& class_ = cpp_file.classes[1];

        REQUIRE(class_.name == "_struct");
        REQUIRE(class_.outer_scope == "_namespace1::_namespace2");
        REQUIRE(class_.inner_scope.empty());
        REQUIRE(class_.scope() == "_namespace1::_namespace2");
        REQUIRE(class_.full_name() == "_namespace1::_namespace2::_struct");
        REQUIRE(class_.type == cpp_class_type::struct_);
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
            REQUIRE((class_.constructors[0].flags & cpp_flags::default_) == cpp_flags::default_);

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
            REQUIRE(class_.functions.size() == 3);

            const cpp_function& func0 = class_.functions[0];

            REQUIRE(func0.name == "_member_func");
            REQUIRE(func0.return_type.name == "int");
            REQUIRE(func0.attributes.size() == 1);
            REQUIRE(func0.attributes.contains("_function"));
            REQUIRE(func0.attributes["_function"] == true);
            REQUIRE(func0.arguments.size() == 1);
            REQUIRE(func0.arguments[0].name == "_arg");
            REQUIRE(func0.arguments[0].type.name == "int");
            REQUIRE(func0.arguments[0].attributes.size() == 1);
            REQUIRE(func0.arguments[0].attributes.contains("_argument"));
            REQUIRE(func0.arguments[0].attributes["_argument"] == true);
            REQUIRE(func0.arguments[0].default_value.has_value() == false);

            const cpp_function& func1 = class_.functions[1];

            REQUIRE(func1.name == "_template_member_func");
            REQUIRE(func1.return_type.name == "_value_t");
            REQUIRE(func1.attributes.size() == 1);
            REQUIRE(func1.attributes.contains("_function"));
            REQUIRE(func1.attributes["_function"] == true);
            REQUIRE(func1.template_params.size() == 2);
            REQUIRE(func1.template_params[0].type == "typename");
            REQUIRE(func1.template_params[0].name == "_value_t");
            REQUIRE(func1.template_params[0].default_value.has_value() == false);
            REQUIRE(func1.template_params[1].type == "int");
            REQUIRE(func1.template_params[1].name == "_n");
            REQUIRE(func1.template_params[1].default_value.has_value());
            REQUIRE(func1.template_params[1].default_value == "42");
            REQUIRE(func1.arguments.size() == 1);
            REQUIRE(func1.arguments[0].name == "_arg");
            REQUIRE(func1.arguments[0].type.name == "_value_t");
            REQUIRE(func1.arguments[0].attributes.size() == 1);
            REQUIRE(func1.arguments[0].attributes.contains("_argument"));
            REQUIRE(func1.arguments[0].attributes["_argument"] == true);
            REQUIRE(func1.arguments[0].default_value.has_value() == false);

            const cpp_function& func2 = class_.functions[2];

            REQUIRE(func2.name == "_virtual_const_func");
            REQUIRE(func2.has_flags(cpp_flags::const_));
            REQUIRE(func2.has_flags(cpp_flags::virtual_));
        }
    }

    SECTION("parse nested class is feature complete")
    {
        const auto& class_ = cpp_file.classes[2];

        REQUIRE(class_.name == "_nested");
        REQUIRE(class_.outer_scope == "_namespace1::_namespace2");
        REQUIRE(class_.inner_scope == "_struct");
        REQUIRE(class_.scope() == "_namespace1::_namespace2::_struct");
        REQUIRE(class_.full_name() == "_namespace1::_namespace2::_struct::_nested");
        REQUIRE(class_.type == cpp_class_type::struct_);
        REQUIRE(class_.nested);
    }

    SECTION("parse nested enum is feature complete")
    {
        const auto& enum_ = cpp_file.enums[1];

        REQUIRE(enum_.name == "_nested_enum");
        REQUIRE(enum_.outer_scope == "_namespace1::_namespace2");
        REQUIRE(enum_.inner_scope == "_struct");
        REQUIRE(enum_.scope() == "_namespace1::_namespace2::_struct");
        REQUIRE(enum_.full_name() == "_namespace1::_namespace2::_struct::_nested_enum");
        REQUIRE(enum_.type == cpp_enum_type::enum_);
        REQUIRE(enum_.nested);
    }

    SECTION("parse class template is feature complete")
    {
        const auto& class_ = cpp_file.classes[3];

        REQUIRE(class_.name == "_struct_template");
        REQUIRE(class_.outer_scope == "_namespace1::_namespace2");
        REQUIRE(class_.inner_scope.empty());
        REQUIRE(class_.scope() == "_namespace1::_namespace2");
        REQUIRE(class_.full_name() == "_namespace1::_namespace2::_struct_template<_value_t, _n>");
        REQUIRE(class_.type == cpp_class_type::struct_);
        REQUIRE(class_.attributes.size() == 1);
        REQUIRE(class_.attributes.contains("_struct"));
        REQUIRE(class_.attributes["_struct"] == true);
        REQUIRE(class_.bases.size() == 1);
        REQUIRE(class_.bases[0].name == "_base");
        REQUIRE(class_.template_params.size() == 2);
        REQUIRE(class_.template_params[0].kind == cpp_template_param_kind::type);
        REQUIRE(class_.template_params[0].type == "typename");
        REQUIRE(class_.template_params[0].name == "_value_t");
        REQUIRE(class_.template_params[0].default_value.has_value() == false);
        REQUIRE(class_.template_params[1].kind == cpp_template_param_kind::non_type);
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
        const auto& class_ = cpp_file.classes[4];

        REQUIRE(class_.name == "_nested_template");
        REQUIRE(class_.outer_scope == "_namespace1::_namespace2");
        REQUIRE(class_.inner_scope == "_struct_template<_value_t, _n>");
        REQUIRE(class_.scope() == "_namespace1::_namespace2::_struct_template<_value_t, _n>");
        REQUIRE(class_.full_name() == "_namespace1::_namespace2::_struct_template<_value_t, _n>::_nested_template<_nested_value_t>");
        REQUIRE(class_.type == cpp_class_type::struct_);
        REQUIRE(class_.template_params[0].kind == cpp_template_param_kind::type);
        REQUIRE(class_.template_params[0].type == "typename");
        REQUIRE(class_.template_params[0].name == "_nested_value_t");
        REQUIRE(class_.template_params[0].default_value.has_value());
        REQUIRE(class_.template_params[0].default_value.value() == "int");
        REQUIRE(class_.nested);
    }

    SECTION("parse free function is feature complete")
    {
        REQUIRE(cpp_file.functions.size() == 3);

        const auto& function0 = cpp_file.functions[0];

        REQUIRE(function0.name == "_free_func");
        REQUIRE(function0.outer_scope == "_namespace1::_namespace2");
        REQUIRE(function0.inner_scope.empty());
        REQUIRE(function0.scope() == "_namespace1::_namespace2");
        REQUIRE(function0.full_name() == "_namespace1::_namespace2::_free_func");
        REQUIRE(function0.return_type.name == "int");
        REQUIRE(function0.attributes.size() == 1);
        REQUIRE(function0.attributes.contains("_function"));
        REQUIRE(function0.attributes["_function"] == true);
        REQUIRE(function0.arguments.size() == 1);
        REQUIRE(function0.arguments[0].name == "_arg");
        REQUIRE(function0.arguments[0].type.name == "int");
        REQUIRE(function0.arguments[0].default_value.has_value());
        REQUIRE(function0.arguments[0].default_value.value() == "42");
        REQUIRE(function0.arguments[0].attributes.size() == 1);
        REQUIRE(function0.arguments[0].attributes.contains("_argument"));
        REQUIRE(function0.arguments[0].attributes["_argument"] == true);

        const auto& function1 = cpp_file.functions[1];

        REQUIRE(function1.name == "_template_free_func");
        REQUIRE(function1.return_type.name == "_value_t");
        REQUIRE(function1.attributes.size() == 1);
        REQUIRE(function1.attributes.contains("_function"));
        REQUIRE(function1.attributes["_function"] == true);
        REQUIRE(function1.template_params.size() == 2);
        REQUIRE(function1.template_params[0].kind == cpp_template_param_kind::type);
        REQUIRE(function1.template_params[0].type == "typename");
        REQUIRE(function1.template_params[0].name == "_value_t");
        REQUIRE(function1.template_params[0].default_value.has_value() == false);
        REQUIRE(function1.template_params[1].kind == cpp_template_param_kind::non_type);
        REQUIRE(function1.template_params[1].type == "int");
        REQUIRE(function1.template_params[1].name == "_n");
        REQUIRE(function1.template_params[1].default_value.has_value());
        REQUIRE(function1.template_params[1].default_value == "42");
        REQUIRE(function1.arguments.size() == 1);
        REQUIRE(function1.arguments[0].name == "_arg");
        REQUIRE(function1.arguments[0].type.name == "_value_t");
        REQUIRE(function1.arguments[0].attributes.size() == 1);
        REQUIRE(function1.arguments[0].attributes.contains("_argument"));
        REQUIRE(function1.arguments[0].attributes["_argument"] == true);
        REQUIRE(function1.arguments[0].default_value.has_value() == false);

        const auto& function2 = cpp_file.functions[2];

        REQUIRE(function2.name == "_global_func");
        REQUIRE(function2.outer_scope.empty());
        REQUIRE(function2.inner_scope.empty());
        REQUIRE(function2.scope().empty());
        REQUIRE(function2.full_name() == "_global_func");
        REQUIRE(function2.return_type.name == "void");
        REQUIRE(function2.attributes.empty());
        REQUIRE(function2.arguments.empty());
    }

    SECTION("parse attributes is feature complete")
    {
        cpp_class& class_ = cpp_file.classes[5];

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
        cpp_class& class_ = cpp_file.classes[6];

        REQUIRE(class_.name == "_template");
        REQUIRE(class_.template_params.size() == 6);
        REQUIRE(class_.template_params[0].kind == cpp_template_param_kind::type);
        REQUIRE(class_.template_params[0].name == "value_t");
        REQUIRE(class_.template_params[0].type == "typename");
        REQUIRE(class_.template_params[1].kind == cpp_template_param_kind::non_type);
        REQUIRE(class_.template_params[1].name == "size_v");
        REQUIRE(class_.template_params[1].type == "int");
        REQUIRE(class_.template_params[2].kind == cpp_template_param_kind::template_);
        REQUIRE(class_.template_params[2].name == "template_t");
        REQUIRE(class_.template_params[2].type == "template <typename arg_t, typename, int> typename");
        REQUIRE(class_.template_params[3].kind == cpp_template_param_kind::type);
        REQUIRE(class_.template_params[3].name == "concept_t");
        REQUIRE(class_.template_params[3].type == "_concept");
        REQUIRE(class_.template_params[4].kind == cpp_template_param_kind::type);
        REQUIRE(class_.template_params[4].name == "nested_concept_t");
        REQUIRE(class_.template_params[4].type == "_nested::_other_concept<int>");
        REQUIRE(class_.template_params[5].kind == cpp_template_param_kind::type);
        REQUIRE(class_.template_params[5].name == "variadic_t");
        REQUIRE(class_.template_params[5].type == "typename...");
        REQUIRE(class_.template_params[5].is_variadic);

        REQUIRE(class_.functions.size() == 1);
        REQUIRE(class_.functions[0].arguments.size() == 1);
        REQUIRE(class_.functions[0].arguments[0].name == "args");
        REQUIRE(class_.functions[0].arguments[0].type.name == "args_t &&");
        REQUIRE(class_.functions[0].arguments[0].is_variadic);
    }

    SECTION("parse unnamed template is feature complete")
    {
        cpp_class& class_ = cpp_file.classes[7];

        REQUIRE(class_.name == "_unnamed_template");
        REQUIRE(class_.template_params.size() == 6);
        REQUIRE(class_.template_params[0].kind == cpp_template_param_kind::type);
        REQUIRE(class_.template_params[0].name == "_t0");
        REQUIRE(class_.template_params[0].type == "typename");
        REQUIRE(class_.template_params[1].kind == cpp_template_param_kind::non_type);
        REQUIRE(class_.template_params[1].name == "_t1");
        REQUIRE(class_.template_params[1].type == "int");
        REQUIRE(class_.template_params[2].kind == cpp_template_param_kind::template_);
        REQUIRE(class_.template_params[2].name == "_t2");
        REQUIRE(class_.template_params[2].type == "template <typename, typename, int> typename");
        REQUIRE(class_.template_params[3].kind == cpp_template_param_kind::type);
        REQUIRE(class_.template_params[3].name == "_t3");
        REQUIRE(class_.template_params[3].type == "_concept");
        REQUIRE(class_.template_params[4].kind == cpp_template_param_kind::type);
        REQUIRE(class_.template_params[4].name == "_t4");
        REQUIRE(class_.template_params[4].type == "_nested::_other_concept<int>");
        REQUIRE(class_.template_params[5].kind == cpp_template_param_kind::type);
        REQUIRE(class_.template_params[5].name == "_t5");
        REQUIRE(class_.template_params[5].type == "typename...");
        REQUIRE(class_.template_params[5].is_variadic);
    }

    SECTION("parse template specialization is feature complete")
    {
        cpp_class& class_ = cpp_file.classes[9];

        REQUIRE(class_.name == "_template_specialization");
        REQUIRE(class_.full_name() == "_template_specialization<int, float, value_t, _template_specialization<int, float, value_t>>");
        REQUIRE(class_.template_params.size() == 1);
        REQUIRE(class_.template_params[0].kind == cpp_template_param_kind::type);
        REQUIRE(class_.template_params[0].name == "value_t");
        REQUIRE(class_.template_params[0].type == "typename");
        REQUIRE(class_.template_specialization_params.size() == 4);
        REQUIRE(class_.template_specialization_params[0] == "int");
        REQUIRE(class_.template_specialization_params[1] == "float");
        REQUIRE(class_.template_specialization_params[2] == "value_t");
        REQUIRE(class_.template_specialization_params[3] == "_template_specialization<int, float, value_t>");
    }

    SECTION("parse weird template is feature complete")
    {
        cpp_class& class_ = cpp_file.classes[10];

        REQUIRE(class_.name == "_weird_template");
        REQUIRE(class_.full_name() == "_weird_template<_t0>");
        REQUIRE(class_.template_params.size() == 1);

        std::string template_param0 = class_.template_params[0].type;
        std::string template_param0_trimmed;

        const auto predicate = [](const char c) { return std::isspace(c); };
        std::ranges::remove_copy_if(template_param0, std::back_inserter(template_param0_trimmed), predicate);

        REQUIRE(class_.template_params[0].kind == cpp_template_param_kind::type);
        REQUIRE(class_.template_params[0].name == "_t0");
        REQUIRE(template_param0_trimmed == "_nested::_concept");
    }

    SECTION("parse variadic base is feature complete")
    {
        cpp_class& class_ = cpp_file.classes[12];

        REQUIRE(class_.name == "_variadic_impl");
        REQUIRE(class_.full_name() == "_variadic_impl<args_t...>");
        REQUIRE(class_.bases.size() == 1);
        REQUIRE(class_.bases[0].name == "_variadic_base<args_t>");
        REQUIRE(class_.bases[0].is_variadic);
    }

    SECTION("parse variadic template template param is feature complete")
    {
        cpp_class& class_ = cpp_file.classes[13];

        REQUIRE(class_.name == "_variadic_template_template_param");
        REQUIRE(class_.full_name() == "_variadic_template_template_param<args_t...>");
        REQUIRE(class_.template_params[0].kind == cpp_template_param_kind::template_);
        REQUIRE(class_.template_params[0].name == "args_t");
        REQUIRE(class_.template_params[0].type == "template <typename ...> typename ...");
        REQUIRE(class_.template_params[0].is_variadic);
    }

    SECTION("parse variable is feature complete")
    {
        cpp_variable var0 = cpp_file.variables[0];

        REQUIRE(var0.name == "_var");
        REQUIRE(var0.default_value.has_value());
        REQUIRE(var0.default_value.value() == "42");
        REQUIRE(var0.type.name == "int");

        cpp_variable var1 = cpp_file.variables[1];

        REQUIRE(var1.name == "_static_var");
        REQUIRE(var1.default_value.has_value());
        REQUIRE(var1.default_value.value() == "42");
        REQUIRE(var1.type.name == "int");
        REQUIRE(var1.has_flags(cpp_flags::static_));

        cpp_variable var2 = cpp_file.variables[2];

        REQUIRE(var2.name == "_const_var");
        REQUIRE(var2.default_value.has_value());
        REQUIRE(var2.default_value.value() == "42");
        REQUIRE(var2.type.name == "const int");
        REQUIRE(var2.type.has_flags(cpp_flags::const_));

        cpp_variable var3 = cpp_file.variables[3];

        REQUIRE(var3.name == "_constexpr_var");
        REQUIRE(var3.default_value.has_value());
        REQUIRE(var3.default_value.value() == "42");
        REQUIRE(var3.type.name == "const int");
        REQUIRE(var3.type.has_flags(cpp_flags::const_));
        REQUIRE(var3.has_flags(cpp_flags::constexpr_));

        cpp_variable var4 = cpp_file.variables[4];

        REQUIRE(var4.name == "_inline_var");
        REQUIRE(var4.default_value.has_value());
        REQUIRE(var4.default_value.value() == "42");
        REQUIRE(var4.type.name == "int");
        REQUIRE(var4.has_flags(cpp_flags::inline_));

        cpp_variable var5 = cpp_file.variables[5];

        REQUIRE(var5.name == "_complex_var");
        REQUIRE(var5.default_value.has_value());
        REQUIRE(var5.default_value.value() == "42");
        REQUIRE(var5.type.name == "const int");
        REQUIRE(var5.type.has_flags(cpp_flags::const_));
        REQUIRE(var5.has_flags(cpp_flags::inline_ | cpp_flags::static_));
        REQUIRE(var5.attributes.size() == 1);
        REQUIRE(var5.attributes.contains("_var"));
        REQUIRE(var5.attributes["_var"] == true);

        cpp_variable var6 = cpp_file.variables[6];

        REQUIRE(var6.name == "_nested_var");
        REQUIRE(var6.full_name() == "_nested::_nested_var");
        REQUIRE(var6.default_value.has_value());
        REQUIRE(var6.default_value.value() == "42");

        cpp_variable var7 = cpp_file.variables[7];

        REQUIRE(var7.name == "_template_var");
        REQUIRE(var7.full_name() == "_template_var<value_t>");
        REQUIRE(var7.template_params.size() == 1);
        REQUIRE(var7.template_params[0].kind == cpp_template_param_kind::type);
        REQUIRE(var7.template_params[0].name == "value_t");
        REQUIRE(var7.template_params[0].type == "typename");
        REQUIRE(var7.default_value.has_value());
        REQUIRE(var7.default_value.value() == "{}");

        cpp_variable var8 = cpp_file.variables[8];

        REQUIRE(var8.name == "_template_var");
        REQUIRE(var8.full_name() == "_template_var<int>");
        REQUIRE(var8.template_params.size() == 0);
        REQUIRE(var8.template_specialization_params.size() == 1);
        REQUIRE(var8.template_specialization_params[0] == "int");
        REQUIRE(var8.default_value.has_value());
        REQUIRE(var8.default_value.value() == "42");
    }
}