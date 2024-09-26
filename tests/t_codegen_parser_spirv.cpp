#include <filesystem>
#include <source_location>
#include <string>

#include "catch2/catch_all.hpp"

#include "spore/codegen/parsers/spirv/codegen_parser_spirv.hpp"

namespace spore::codegen::detail
{
    static std::string get_spirv_file()
    {
        constexpr std::source_location source_location = std::source_location::current();
        const std::filesystem::path current_file = source_location.file_name();
        const std::filesystem::path current_dir = current_file.parent_path();
        const std::filesystem::path input_file = current_dir / "t_codegen_parser_spirv_data.spv";
        return input_file.string();
    }
}

TEST_CASE("spore::codegen::codegen_parser_spirv", "[spore::codegen][spore::codegen::codegen_parser_spirv]")
{
    using namespace spore::codegen;

    constexpr std::initializer_list<std::string_view> parser_args {};

    codegen_parser_spirv parser {parser_args};
    std::vector input_files {detail::get_spirv_file()};
    std::vector<spirv_module> spirv_modules;

    REQUIRE(parser.parse_asts(input_files, spirv_modules));
    REQUIRE(spirv_modules.size() == input_files.size());

    const spirv_module& spirv_module = spirv_modules[0];

    REQUIRE(spirv_module.path == input_files[0]);
    REQUIRE(spirv_module.stage == spirv_module_stage::vertex);
    REQUIRE(spirv_module.byte_code.size() == std::filesystem::file_size(input_files[0]));
    REQUIRE(spirv_module.entry_point == "main");
    REQUIRE(spirv_module.inputs.size() == 3);
    REQUIRE(spirv_module.outputs.size() == 4);
    REQUIRE(spirv_module.constants.size() == 1);
    REQUIRE(spirv_module.descriptor_sets.size() == 2);
    REQUIRE(spirv_module.descriptor_sets[0].bindings.size() == 1);
    REQUIRE(spirv_module.descriptor_sets[1].bindings.size() == 2);

    SECTION("parse inputs is feature complete")
    {
        REQUIRE(spirv_module.inputs[0].index == 0);
        REQUIRE(spirv_module.inputs[0].name == "input0");

        const spirv_type& type0 = spirv_module.inputs[0].type;
        REQUIRE(std::holds_alternative<spirv_vec>(type0));
        REQUIRE(std::get<spirv_vec>(type0).components == 3);
        REQUIRE(std::get<spirv_vec>(type0).scalar.width == 32);
        REQUIRE(std::get<spirv_vec>(type0).scalar.kind == spirv_scalar_kind::float_);

        REQUIRE(spirv_module.inputs[1].index == 1);
        REQUIRE(spirv_module.inputs[1].name == "input1");

        const spirv_type& type1 = spirv_module.inputs[1].type;
        REQUIRE(std::holds_alternative<spirv_vec>(type1));
        REQUIRE(std::get<spirv_vec>(type1).components == 3);
        REQUIRE(std::get<spirv_vec>(type1).scalar.width == 32);
        REQUIRE(std::get<spirv_vec>(type1).scalar.kind == spirv_scalar_kind::float_);

        REQUIRE(spirv_module.inputs[2].index == 2);
        REQUIRE(spirv_module.inputs[2].name == "input2");

        const spirv_type& type2 = spirv_module.inputs[2].type;
        REQUIRE(std::holds_alternative<spirv_array>(type2));
        REQUIRE(std::get<spirv_array>(type2).dims.size() == 1);
        REQUIRE(std::get<spirv_array>(type2).dims[0] == 4);

        const spirv_single_type& single_type2 = std::get<spirv_array>(type2).type;
        REQUIRE(std::holds_alternative<spirv_vec>(single_type2));
        REQUIRE(std::get<spirv_vec>(single_type2).components == 2);
        REQUIRE(std::get<spirv_vec>(single_type2).scalar.width == 32);
        REQUIRE(std::get<spirv_vec>(single_type2).scalar.kind == spirv_scalar_kind::float_);
    }

    SECTION("parse outputs is feature complete")
    {
        REQUIRE(spirv_module.outputs[0].index == 0);
        REQUIRE(spirv_module.outputs[0].name == "output0");

        const spirv_type& output_type0 = spirv_module.outputs[0].type;
        REQUIRE(std::holds_alternative<spirv_vec>(output_type0));
        REQUIRE(std::get<spirv_vec>(output_type0).components == 3);
        REQUIRE(std::get<spirv_vec>(output_type0).scalar.width == 32);
        REQUIRE(std::get<spirv_vec>(output_type0).scalar.kind == spirv_scalar_kind::float_);

        REQUIRE(spirv_module.outputs[1].index == 1);
        REQUIRE(spirv_module.outputs[1].name == "output1");

        const spirv_type& output_type1 = spirv_module.outputs[1].type;
        REQUIRE(std::holds_alternative<spirv_vec>(output_type1));
        REQUIRE(std::get<spirv_vec>(output_type1).components == 3);
        REQUIRE(std::get<spirv_vec>(output_type1).scalar.width == 32);
        REQUIRE(std::get<spirv_vec>(output_type1).scalar.kind == spirv_scalar_kind::float_);

        REQUIRE(spirv_module.outputs[2].index == 2);
        REQUIRE(spirv_module.outputs[2].name == "output2");

        const spirv_type& output_type2 = spirv_module.outputs[2].type;
        REQUIRE(std::holds_alternative<spirv_array>(output_type2));
        REQUIRE(std::get<spirv_array>(output_type2).dims.size() == 1);
        REQUIRE(std::get<spirv_array>(output_type2).dims[0] == 4);

        const spirv_single_type& output_elem_type2 = std::get<spirv_array>(output_type2).type;
        REQUIRE(std::holds_alternative<spirv_vec>(output_elem_type2));
        REQUIRE(std::get<spirv_vec>(output_elem_type2).components == 2);
        REQUIRE(std::get<spirv_vec>(output_elem_type2).scalar.width == 32);
        REQUIRE(std::get<spirv_vec>(output_elem_type2).scalar.kind == spirv_scalar_kind::float_);

        // TODO @sporacid invalid value should be max of std::size_t
        REQUIRE(spirv_module.outputs[3].index == std::numeric_limits<std::uint32_t>::max());
        REQUIRE(not spirv_module.outputs[3].name.empty());

        const spirv_type& output_type3 = spirv_module.outputs[3].type;
        REQUIRE(std::holds_alternative<spirv_builtin>(output_type3));
        REQUIRE(std::get<spirv_builtin>(output_type3).name == "gl_PerVertex");
    }

    SECTION("parse descriptors is feature complete")
    {
        REQUIRE(spirv_module.descriptor_sets[0].index == 0);
        REQUIRE(spirv_module.descriptor_sets[1].index == 1);

        const spirv_descriptor_binding& descriptor0_0 = spirv_module.descriptor_sets[0].bindings[0];

        REQUIRE(descriptor0_0.index == 0);
        REQUIRE(descriptor0_0.kind == spirv_descriptor_kind::uniform);
        REQUIRE(not descriptor0_0.name.empty());

        REQUIRE(std::holds_alternative<spirv_struct>(descriptor0_0.type));
        REQUIRE(std::get<spirv_struct>(descriptor0_0.type).name == "descriptors");

        REQUIRE(descriptor0_0.variables.size() == 2);
        REQUIRE(descriptor0_0.variables[0].index == 0);
        REQUIRE(descriptor0_0.variables[0].name == "descriptor0");

        REQUIRE(std::holds_alternative<spirv_vec>(descriptor0_0.variables[0].type));
        REQUIRE(std::holds_alternative<spirv_scalar>(descriptor0_0.variables[1].type));

        const spirv_descriptor_binding& descriptor1_0 = spirv_module.descriptor_sets[1].bindings[0];

        REQUIRE(descriptor1_0.index == 0);
        REQUIRE(descriptor1_0.name == "sampler0");
        REQUIRE(descriptor1_0.variables.empty());
        REQUIRE(std::holds_alternative<spirv_image>(descriptor1_0.type));

        const spirv_descriptor_binding& descriptor1_1 = spirv_module.descriptor_sets[1].bindings[1];

        REQUIRE(descriptor1_1.index == 1);
        REQUIRE(descriptor1_1.name == "sampler1");
        REQUIRE(descriptor1_1.variables.empty());
        REQUIRE(std::holds_alternative<spirv_image>(descriptor1_1.type));
    }

    SECTION("parse constants is feature complete")
    {
        const spirv_constant& constant0 = spirv_module.constants[0];

        REQUIRE(not constant0.name.empty());
        REQUIRE(constant0.offset == 0);
        REQUIRE(std::holds_alternative<spirv_struct>(constant0.type));
        REQUIRE(constant0.variables.size() == 2);

        REQUIRE(constant0.variables[0].index == 0);
        REQUIRE(constant0.variables[0].name == "constant0");
        REQUIRE(std::holds_alternative<spirv_vec>(constant0.variables[0].type));
        REQUIRE(std::holds_alternative<spirv_scalar>(constant0.variables[1].type));
    }
}