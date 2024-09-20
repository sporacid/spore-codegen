#pragma once

#include "spirv_reflect.h"

#include "spore/codegen/parsers/codegen_parser.hpp"
#include "spore/codegen/parsers/spirv/ast/spirv_module.hpp"

namespace spore::codegen
{
    namespace detail
    {
        template <typename spirv_value_t>
        using enumerate_func_t = SpvReflectResult (*)(const SpvReflectShaderModule*, uint32_t*, spirv_value_t**);

        template <typename spirv_value_t>
        SpvReflectResult spv_enumerate(const SpvReflectShaderModule& module, const enumerate_func_t<spirv_value_t>& enumerate_func, std::vector<spirv_value_t*>& values)
        {
            std::uint32_t count = 0;
            SpvReflectResult result = enumerate_func(&module, &count, nullptr);

            if (result == SPV_REFLECT_RESULT_SUCCESS) [[unlikely]]
            {
                values.resize(count);
                result = enumerate_func(&module, &count, values.data());
            }

            return result;
        }

        template <typename spirv_value_t, typename seed_t = decltype([] {})>
        std::string get_or_generate_name(const spirv_value_t& value)
        {
            if (value.name == nullptr || std::strlen(value.name) == 0)
            {
                return fmt::format("_var{}", make_unique_id<seed_t>());
            }

            return value.name;
        }

        inline void to_descriptor_binding(const SpvReflectShaderModule& spv_module, const SpvReflectDescriptorBinding& spv_descriptor_binding, spirv_descriptor_binding& descriptor_binding)
        {
            descriptor_binding.name = get_or_generate_name(spv_descriptor_binding);
            descriptor_binding.index = static_cast<std::size_t>(spv_descriptor_binding.binding);
            descriptor_binding.count = 1;

            for (const std::uint32_t dimension : std::span {spv_descriptor_binding.array.dims, spv_descriptor_binding.array.dims_count})
            {
                descriptor_binding.count *= dimension;
            }

            switch (spv_descriptor_binding.descriptor_type)
            {
                case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER:
                    descriptor_binding.kind = spirv_descriptor_kind::sampler;
                    break;

                case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                    descriptor_binding.kind = spirv_descriptor_kind::uniform;
                    break;

                case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
                    descriptor_binding.kind = spirv_descriptor_kind::storage;
                    break;

                default:
                    descriptor_binding.kind = spirv_descriptor_kind::unknown;
                    break;
            }
        }

        inline void to_descriptor_set(const SpvReflectShaderModule& spv_module, const SpvReflectDescriptorSet& spv_descriptor_set, spirv_descriptor_set& descriptor_set)
        {
            descriptor_set.index = static_cast<std::size_t>(spv_descriptor_set.set);
            descriptor_set.bindings.reserve(spv_descriptor_set.binding_count);

            for (const SpvReflectDescriptorBinding* spv_descriptor_binding : std::span {spv_descriptor_set.bindings, spv_descriptor_set.binding_count})
            {
                to_descriptor_binding(spv_module, *spv_descriptor_binding, descriptor_set.bindings.emplace_back());
            }

            std::ranges::sort(descriptor_set.bindings, std::less<>(), [](const spirv_descriptor_binding& descriptor_binding) { return descriptor_binding.index; });
        }

        inline void to_constant(const SpvReflectShaderModule& spv_module, const SpvReflectBlockVariable& spv_variable, spirv_constant& constant)
        {
            constant.name = get_or_generate_name(spv_variable);
            constant.offset = spv_variable.offset;
        }

        inline void to_variable(const SpvReflectShaderModule& spv_module, const SpvReflectInterfaceVariable& spv_variable, spirv_variable& variable)
        {
            variable.name = get_or_generate_name(spv_variable);
            variable.location = static_cast<std::size_t>(spv_variable.location);
            variable.format = static_cast<std::size_t>(spv_variable.format);
            variable.count = 1;

            for (const std::uint32_t dimension : std::span {spv_variable.array.dims, spv_variable.array.dims_count})
            {
                variable.count *= dimension;
            }

            const std::size_t scalar_size = spv_variable.numeric.scalar.width / 8;
            variable.size = variable.count * scalar_size;

            if (spv_variable.numeric.vector.component_count > 0)
            {
                variable.size *= spv_variable.numeric.vector.component_count;
            }
        }

        inline void to_module(std::string_view path, const SpvReflectShaderModule& spv_module, spirv_module& module)
        {
            module.path = path;
            module.entry_point = spv_module.entry_point_name;

            std::vector<SpvReflectDescriptorSet*> spv_descriptor_sets;
            SpvReflectResult spv_result = spv_enumerate(spv_module, &spvReflectEnumerateDescriptorSets, spv_descriptor_sets);

            if (spv_result != SPV_REFLECT_RESULT_SUCCESS)
            {
                SPDLOG_WARN("failed to enumerate descriptor sets, path={} result={}", path, static_cast<std::int32_t>(spv_result));
            }

            std::vector<SpvReflectBlockVariable*> spv_push_constants;
            spv_result = spv_enumerate(spv_module, &spvReflectEnumeratePushConstantBlocks, spv_push_constants);

            if (spv_result != SPV_REFLECT_RESULT_SUCCESS)
            {
                SPDLOG_WARN("failed to enumerate push constants, path={} result={}", path, static_cast<std::int32_t>(spv_result));
            }

            std::vector<SpvReflectInterfaceVariable*> spv_inputs;
            spv_result = spv_enumerate(spv_module, &spvReflectEnumerateInputVariables, spv_inputs);

            if (spv_result != SPV_REFLECT_RESULT_SUCCESS)
            {
                SPDLOG_WARN("failed to enumerate inputs, path={} result={}", path, static_cast<std::int32_t>(spv_result));
            }

            std::vector<SpvReflectInterfaceVariable*> spv_outputs;
            spv_result = spv_enumerate(spv_module, &spvReflectEnumerateOutputVariables, spv_outputs);

            if (spv_result != SPV_REFLECT_RESULT_SUCCESS)
            {
                SPDLOG_WARN("failed to enumerate outputs, path={} result={}", path, static_cast<std::int32_t>(spv_result));
            }

            for (SpvReflectDescriptorSet* spv_descriptor_set : spv_descriptor_sets)
            {
                to_descriptor_set(spv_module, *spv_descriptor_set, module.descriptor_sets.emplace_back());
            }

            for (SpvReflectBlockVariable* spv_push_constant : spv_push_constants)
            {
                to_constant(spv_module, *spv_push_constant, module.constants.emplace_back());
            }

            for (SpvReflectInterfaceVariable* spv_input : spv_inputs)
            {
                to_variable(spv_module, *spv_input, module.inputs.emplace_back());
            }

            for (SpvReflectInterfaceVariable* spv_output : spv_outputs)
            {
                to_variable(spv_module, *spv_output, module.outputs.emplace_back());
            }

            std::ranges::sort(module.descriptor_sets, std::less<>(), [](const spirv_descriptor_set& descriptor_set) { return descriptor_set.index; });
            std::ranges::sort(module.constants, std::less<>(), [](const spirv_constant& constant) { return constant.offset; });
            std::ranges::sort(module.inputs, std::less<>(), [](const spirv_variable& input) { return input.location; });
            std::ranges::sort(module.outputs, std::less<>(), [](const spirv_variable& output) { return output.location; });
        }

    }

    struct codegen_parser_spirv final : codegen_parser<spirv_module>
    {
        template <typename args_t>
        explicit codegen_parser_spirv(const args_t& args)
        {
            if (std::size(args) > 0)
            {
                SPDLOG_WARN("SPIR-V parser does not support additional arguments, they will be ignored");
            }
        }

        bool parse_asts(const std::vector<std::string>& paths, std::vector<spirv_module>& modules) override
        {
            modules.clear();
            modules.reserve(paths.size());

            for (const std::string& path : paths)
            {
                if (!parse_ast(path, modules.emplace_back()))
                {
                    return false;
                }
            }

            return true;
        }

      private:
        static bool parse_ast(std::string_view path, spirv_module& module)
        {
            SpvReflectResult spv_result {};
            SpvReflectShaderModule spv_module {};

            if (!files::read_file(path, module.byte_code))
            {
                SPDLOG_ERROR("cannot read SPIR-V file, path={}", path);
                return false;
            }

            spv_result = spvReflectCreateShaderModule(module.byte_code.size(), module.byte_code.data(), &spv_module);
            if (spv_result != SPV_REFLECT_RESULT_SUCCESS) [[unlikely]]
            {
                SPDLOG_ERROR("SPIR-V reflection failed, path={} result={}", path, static_cast<std::int32_t>(spv_result));
                return false;
            }

            detail::to_module(path, spv_module, module);
            return true;
        }
    };
}