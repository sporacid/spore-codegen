#pragma once

#include "spirv_reflect.h"

#include "spore/codegen/parsers/codegen_parser.hpp"
#include "spore/codegen/parsers/spirv/ast/spirv_module.hpp"

namespace spore::codegen
{
    namespace detail
    {
        template <typename variant_t, typename value_t>
        struct variant_supports;

        template <typename value_t, typename... args_t>
        struct variant_supports<std::variant<args_t...>, value_t>
            : std::disjunction<std::is_same<value_t, args_t>...>
        {
        };

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

        template <typename seed_t = decltype([] {})>
        std::string get_or_make_name(const char* name)
        {
            if (name == nullptr || std::strlen(name) == 0)
            {
                return fmt::format("_var{}", make_unique_id<seed_t>());
            }

            return name;
        }

        inline void to_type(const SpvReflectTypeDescription& spv_type, spirv_type& type)
        {
            const auto has_any_flag = [&](const SpvReflectTypeFlags other_spv_flags) {
                return (spv_type.type_flags & other_spv_flags) != 0;
            };

            if (has_any_flag(SPV_REFLECT_TYPE_FLAG_BOOL | SPV_REFLECT_TYPE_FLAG_INT | SPV_REFLECT_TYPE_FLAG_FLOAT))
            {
                spirv_scalar scalar {
                    .width = spv_type.traits.numeric.scalar.width,
                    .signed_ = static_cast<bool>(spv_type.traits.numeric.scalar.signedness),
                };

                if (has_any_flag(SPV_REFLECT_TYPE_FLAG_BOOL))
                {
                    scalar.kind = spirv_scalar_kind::bool_;
                }
                else if (has_any_flag(SPV_REFLECT_TYPE_FLAG_INT))
                {
                    scalar.kind = spirv_scalar_kind::int_;
                }
                else if (has_any_flag(SPV_REFLECT_TYPE_FLAG_FLOAT))
                {
                    scalar.kind = spirv_scalar_kind::float_;
                }

                type = scalar;
            }

            if (has_any_flag(SPV_REFLECT_TYPE_FLAG_MATRIX))
            {
                const bool row_major = (spv_type.decoration_flags & SPV_REFLECT_DECORATION_ROW_MAJOR) == SPV_REFLECT_DECORATION_ROW_MAJOR;
                type = spirv_mat {
                    .scalar = std::get<spirv_scalar>(type),
                    .rows = spv_type.traits.numeric.matrix.row_count,
                    .cols = spv_type.traits.numeric.matrix.column_count,
                    .row_major = row_major,
                };
            }
            else if (has_any_flag(SPV_REFLECT_TYPE_FLAG_VECTOR))
            {
                type = spirv_vec {
                    .scalar = std::get<spirv_scalar>(type),
                    .components = spv_type.traits.numeric.vector.component_count,
                };
            }
            else if (has_any_flag(SPV_REFLECT_TYPE_FLAG_STRUCT))
            {
                type = spirv_struct {
                    .name = spv_type.type_name,
                };
            }

            if (has_any_flag(SPV_REFLECT_TYPE_FLAG_ARRAY))
            {
                spirv_array array;
                const auto visitor = [&]<typename value_t>(value_t&& value) {
                    if constexpr (variant_supports<spirv_array::variant_t, std::decay_t<value_t>>::value)
                    {
                        array.type = std::forward<value_t>(value);
                    }
                };

                std::visit(visitor, type);

                for (std::size_t index = 0; index < spv_type.traits.array.dims_count && index < array.dims.size(); ++index)
                {
                    array.dims[index] = static_cast<std::size_t>(spv_type.traits.array.dims[index]);
                }
            }
        }

        // inline void to_type(
        //     const SpvReflectNumericTraits& spv_traits,
        //     const SpvReflectArrayTraits& spv_array_traits,
        //     const SpvReflectTypeFlags spv_flags,
        //     spirv_type& type)
        // {
        //     const auto has_any_flag = [&](const SpvReflectTypeFlags other_spv_flags) {
        //         return (spv_flags & other_spv_flags) != 0;
        //     };
        //
        //     if (has_any_flag(SPV_REFLECT_TYPE_FLAG_BOOL | SPV_REFLECT_TYPE_FLAG_INT | SPV_REFLECT_TYPE_FLAG_FLOAT))
        //     {
        //         spirv_scalar scalar {
        //             .width = spv_traits.scalar.width,
        //             .is_signed = static_cast<bool>(spv_traits.scalar.signedness),
        //         };
        //
        //         if (has_any_flag(SPV_REFLECT_TYPE_FLAG_BOOL))
        //         {
        //             scalar.kind = spirv_scalar_kind::bool_;
        //         }
        //         else if (has_any_flag(SPV_REFLECT_TYPE_FLAG_INT))
        //         {
        //             scalar.kind = spirv_scalar_kind::int_;
        //         }
        //         else if (has_any_flag(SPV_REFLECT_TYPE_FLAG_FLOAT))
        //         {
        //             scalar.kind = spirv_scalar_kind::float_;
        //         }
        //
        //         type = std::move(scalar);
        //     }
        //
        //     if (has_any_flag(SPV_REFLECT_TYPE_FLAG_STRUCT))
        //     {
        //         printf("");
        //     }
        //
        //     if (has_any_flag(SPV_REFLECT_TYPE_FLAG_VECTOR))
        //     {
        //         auto& scalar = std::get<spirv_scalar>(type);
        //         spirv_vec vec {
        //             .scalar = std::move(scalar),
        //             .components = spv_traits.vector.component_count,
        //         };
        //         type = std::move(vec);
        //     }
        //     else if (has_any_flag(SPV_REFLECT_TYPE_FLAG_MATRIX))
        //     {
        //         auto& scalar = std::get<spirv_scalar>(type);
        //         spirv_mat mat {
        //             .scalar = std::move(scalar),
        //             .rows = spv_traits.matrix.row_count,
        //             .cols = spv_traits.matrix.column_count,
        //         };
        //         type = std::move(mat);
        //     }
        //
        //     if (has_any_flag(SPV_REFLECT_TYPE_FLAG_ARRAY))
        //     {
        //         spirv_array array;
        //         const auto visitor = [&]<typename value_t>(value_t&& value) {
        //             if constexpr (not std::is_same_v<spirv_array, std::decay_t<value_t>>)
        //             {
        //                 array.type = std::forward<value_t>(value);
        //             }
        //         };
        //
        //         std::visit(visitor, type);
        //
        //         for (std::size_t index = 0; index < spv_array_traits.dims_count && index < array.dims.size(); ++index)
        //         {
        //             array.dims[index] = static_cast<std::size_t>(spv_array_traits.dims[index]);
        //         }
        //     }
        // }

        inline void to_variable(const SpvReflectInterfaceVariable& spv_variable, spirv_variable& variable)
        {
            variable.name = get_or_make_name(spv_variable.name);
            variable.index = static_cast<std::size_t>(spv_variable.location);

            if (const SpvReflectTypeDescription* spv_type = spv_variable.type_description)
            {
                to_type(*spv_type, variable.type);
            }
        }

        inline void to_variable(const std::size_t index, const SpvReflectBlockVariable& spv_variable, spirv_variable& variable)
        {
            variable.name = get_or_make_name(spv_variable.name);
            variable.index = index;

            if (const SpvReflectTypeDescription* spv_type = spv_variable.type_description)
            {
                to_type(*spv_type, variable.type);
            }
        }

        inline void to_variable(const std::size_t index, const SpvReflectTypeDescription& spv_member, spirv_variable& variable)
        {
            variable.name = get_or_make_name(spv_member.struct_member_name);
            variable.index = index;

            to_type(spv_member, variable.type);
        }

        inline void to_descriptor_binding(const SpvReflectDescriptorBinding& spv_descriptor_binding, spirv_descriptor_binding& descriptor_binding)
        {
            descriptor_binding.name = get_or_make_name(spv_descriptor_binding.name);
            descriptor_binding.index = static_cast<std::size_t>(spv_descriptor_binding.binding);
            descriptor_binding.count = 1;

            if (const SpvReflectTypeDescription* spv_type = spv_descriptor_binding.type_description)
            {
                descriptor_binding.type = spv_type->type_name;
                descriptor_binding.variables.reserve(spv_type->member_count);

                for (const SpvReflectTypeDescription& spv_member : std::span {spv_type->members, spv_type->member_count})
                {
                    const std::size_t spv_member_index = descriptor_binding.variables.size();
                    to_variable(spv_member_index, spv_member, descriptor_binding.variables.emplace_back());
                }
            }

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

        inline void to_descriptor_set(const SpvReflectDescriptorSet& spv_descriptor_set, spirv_descriptor_set& descriptor_set)
        {
            descriptor_set.index = static_cast<std::size_t>(spv_descriptor_set.set);
            descriptor_set.bindings.reserve(spv_descriptor_set.binding_count);

            for (const SpvReflectDescriptorBinding* spv_descriptor_binding : std::span {spv_descriptor_set.bindings, spv_descriptor_set.binding_count})
            {
                to_descriptor_binding(*spv_descriptor_binding, descriptor_set.bindings.emplace_back());
            }

            std::ranges::sort(descriptor_set.bindings, std::less<>(), [](const spirv_descriptor_binding& descriptor_binding) { return descriptor_binding.index; });
        }

        inline void to_constant(const SpvReflectBlockVariable& spv_variable, spirv_constant& constant)
        {
            constant.name = get_or_make_name(spv_variable.name);
            constant.offset = spv_variable.offset;

            for (const SpvReflectBlockVariable& spv_member : std::span {spv_variable.members, spv_variable.member_count})
            {
                const std::size_t spv_member_index = constant.variables.size();
                to_variable(spv_member_index, spv_member, constant.variables.emplace_back());
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
                to_descriptor_set(*spv_descriptor_set, module.descriptor_sets.emplace_back());
            }

            for (SpvReflectBlockVariable* spv_push_constant : spv_push_constants)
            {
                to_constant(*spv_push_constant, module.constants.emplace_back());
            }

            for (SpvReflectInterfaceVariable* spv_input : spv_inputs)
            {
                to_variable(*spv_input, module.inputs.emplace_back());
            }

            for (SpvReflectInterfaceVariable* spv_output : spv_outputs)
            {
                to_variable(*spv_output, module.outputs.emplace_back());
            }

            const auto projection_index = [](const auto& value) { return value.index; };
            std::ranges::sort(module.inputs, std::less<>(), projection_index);
            std::ranges::sort(module.outputs, std::less<>(), projection_index);
            std::ranges::sort(module.descriptor_sets, std::less<>(), projection_index);
            std::ranges::sort(module.constants, std::less<>(), [](const spirv_constant& constant) { return constant.offset; });
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
            SpvReflectShaderModule spv_module {};

            if (!files::read_file(path, module.byte_code))
            {
                SPDLOG_ERROR("cannot read SPIR-V file, path={}", path);
                return false;
            }

            const SpvReflectResult spv_result = spvReflectCreateShaderModule(module.byte_code.size(), module.byte_code.data(), &spv_module);
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