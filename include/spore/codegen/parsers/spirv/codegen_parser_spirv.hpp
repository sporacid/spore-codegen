#pragma once

#include <format>
#include <ranges>

#include "spdlog/spdlog.h"
#include "spirv_reflect.h"

#include "spore/codegen/misc/defer.hpp"
#include "spore/codegen/misc/make_unique_id.hpp"
#include "spore/codegen/parsers/codegen_parser.hpp"
#include "spore/codegen/parsers/spirv/ast/spirv_module.hpp"
#include "spore/codegen/utils/files.hpp"

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

        template <typename seed_t>
        std::string get_or_make_name(const char* name, const char* prefix)
        {
            if (name == nullptr || std::strlen(name) == 0)
            {
                return std::format("_{}{}", prefix, make_unique_id<seed_t>());
            }

            return name;
        }

        template <typename flags_t, typename other_flags_t>
        bool has_all_flags(flags_t flag, other_flags_t other_flag)
        {
            return (flag & other_flag) == other_flag;
        }

        template <typename flags_t, typename other_flags_t>
        bool has_any_flag(flags_t flag, other_flags_t other_flag)
        {
            return (flag & other_flag) != 0;
        }

        template <typename spv_parent_t>
        void to_type(const spv_parent_t& spv_parent, const SpvReflectTypeDescription& spv_type, spirv_type& type)
        {
            if (has_any_flag(spv_parent.decoration_flags, SPV_REFLECT_DECORATION_BUILT_IN))
            {
                type = spirv_builtin {
                    .name = spv_type.type_name,
                };
            }
            else
            {
                if (has_any_flag(spv_type.type_flags, SPV_REFLECT_TYPE_FLAG_BOOL | SPV_REFLECT_TYPE_FLAG_INT | SPV_REFLECT_TYPE_FLAG_FLOAT))
                {
                    spirv_scalar scalar {
                        .width = spv_type.traits.numeric.scalar.width,
                    };

                    if (has_any_flag(spv_type.type_flags, SPV_REFLECT_TYPE_FLAG_BOOL))
                    {
                        scalar.kind = spirv_scalar_kind::bool_;
                    }
                    else if (has_any_flag(spv_type.type_flags, SPV_REFLECT_TYPE_FLAG_INT))
                    {
                        scalar.kind = spirv_scalar_kind::int_;
                        scalar.signed_ = static_cast<bool>(spv_type.traits.numeric.scalar.signedness);
                    }
                    else if (has_any_flag(spv_type.type_flags, SPV_REFLECT_TYPE_FLAG_FLOAT))
                    {
                        scalar.kind = spirv_scalar_kind::float_;
                        scalar.signed_ = true;
                    }

                    type = scalar;
                }

                if (has_any_flag(spv_type.type_flags, SPV_REFLECT_TYPE_FLAG_MATRIX))
                {
                    const bool row_major =
                        has_any_flag(spv_type.decoration_flags, SPV_REFLECT_DECORATION_ROW_MAJOR) and
                        not has_any_flag(spv_type.decoration_flags, SPV_REFLECT_DECORATION_COLUMN_MAJOR);

                    type = spirv_mat {
                        .scalar = std::get<spirv_scalar>(type),
                        .rows = spv_type.traits.numeric.matrix.row_count,
                        .cols = spv_type.traits.numeric.matrix.column_count,
                        .row_major = row_major,
                    };
                }
                else if (has_any_flag(spv_type.type_flags, SPV_REFLECT_TYPE_FLAG_VECTOR))
                {
                    type = spirv_vec {
                        .scalar = std::get<spirv_scalar>(type),
                        .components = spv_type.traits.numeric.vector.component_count,
                    };
                }
                else if (has_any_flag(spv_type.type_flags, SPV_REFLECT_TYPE_FLAG_STRUCT))
                {
                    type = spirv_struct {
                        .name = spv_type.type_name,
                    };
                }
                else if (has_any_flag(spv_type.type_flags, SPV_REFLECT_TYPE_FLAG_EXTERNAL_IMAGE | SPV_REFLECT_TYPE_FLAG_EXTERNAL_SAMPLER | SPV_REFLECT_TYPE_FLAG_EXTERNAL_SAMPLED_IMAGE))
                {
                    std::size_t dim;
                    switch (spv_type.traits.image.dim)
                    {
                        case SpvDim1D:
                            dim = 1;
                            break;
                        case SpvDim2D:
                            dim = 2;
                            break;
                        case SpvDim3D:
                            dim = 3;
                            break;
                        default:
                            dim = 0;
                            SPDLOG_DEBUG("unknown image dimension type, value={}", static_cast<std::uint32_t>(spv_type.traits.image.dim));
                            break;
                    }

                    type = spirv_image {
                        .dims = dim,
                        .depth = static_cast<std::size_t>(spv_type.traits.image.depth),
                        .sampled = static_cast<bool>(spv_type.traits.image.sampled),
                    };
                }
            }

            if (has_any_flag(spv_type.type_flags, SPV_REFLECT_TYPE_FLAG_ARRAY))
            {
                spirv_array array;
                array.dims.resize(spv_type.traits.array.dims_count);

                for (std::size_t index = 0; index < spv_type.traits.array.dims_count && index < array.dims.size(); ++index)
                {
                    array.dims[index] = static_cast<std::size_t>(spv_type.traits.array.dims[index]);
                }

                const auto visitor = [&]<typename value_t>(value_t&& value) {
                    if constexpr (variant_supports<spirv_single_type, std::decay_t<value_t>>::value)
                    {
                        array.type = std::forward<value_t>(value);
                    }
                };

                std::visit(visitor, type);

                type = std::move(array);
            }
        }

        template <typename spv_parent_t>
        void to_variable(const spv_parent_t& spv_parent, const SpvReflectTypeDescription& spv_member, const std::size_t index, spirv_variable& variable)
        {
            variable.name = get_or_make_name<spirv_variable>(spv_member.struct_member_name, "var");
            variable.index = index;

            to_type(spv_parent, spv_member, variable.type);
        }

        inline void to_variable(const SpvReflectInterfaceVariable& spv_variable, spirv_variable& variable)
        {
            variable.name = get_or_make_name<spirv_variable>(spv_variable.name, "var");

            using spv_location_t = decltype(spv_variable.location);

            if (spv_variable.location == std::numeric_limits<spv_location_t>::max())
            {
                variable.index = std::numeric_limits<std::size_t>::max();
            }
            else
            {
                variable.index = static_cast<std::size_t>(spv_variable.location);
            }

            if (const SpvReflectTypeDescription* spv_type = spv_variable.type_description)
            {
                to_type(spv_variable, *spv_type, variable.type);
            }
        }

        inline void to_variable(const std::size_t index, const SpvReflectBlockVariable& spv_variable, spirv_variable& variable)
        {
            variable.name = get_or_make_name<spirv_variable>(spv_variable.name, "var");
            variable.index = index;

            if (const SpvReflectTypeDescription* spv_type = spv_variable.type_description)
            {
                to_type(spv_variable, *spv_type, variable.type);
            }
        }

        inline void to_descriptor_binding(const SpvReflectDescriptorBinding& spv_descriptor_binding, spirv_descriptor_binding& descriptor_binding)
        {
            descriptor_binding.name = get_or_make_name<spirv_descriptor_binding>(spv_descriptor_binding.name, "descriptor");
            descriptor_binding.index = static_cast<std::size_t>(spv_descriptor_binding.binding);
            descriptor_binding.count = static_cast<std::size_t>(spv_descriptor_binding.count);

            if (const SpvReflectTypeDescription* spv_type = spv_descriptor_binding.type_description)
            {
                to_type(spv_descriptor_binding, *spv_type, descriptor_binding.type);

                descriptor_binding.variables.reserve(spv_type->member_count);

                for (const SpvReflectTypeDescription& spv_member : std::span {spv_type->members, spv_type->member_count})
                {
                    const std::size_t spv_member_index = descriptor_binding.variables.size();
                    to_variable(spv_descriptor_binding, spv_member, spv_member_index, descriptor_binding.variables.emplace_back());
                }
            }

            switch (spv_descriptor_binding.descriptor_type)
            {
                case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER:
                case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
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
            constant.name = get_or_make_name<spirv_constant>(spv_variable.name, "constant");
            constant.offset = spv_variable.offset;

            if (const SpvReflectTypeDescription* spv_type = spv_variable.type_description)
            {
                to_type(spv_variable, *spv_type, constant.type);
            }

            for (const SpvReflectBlockVariable& spv_member : std::span {spv_variable.members, spv_variable.member_count})
            {
                const std::size_t spv_member_index = constant.variables.size();
                to_variable(spv_member_index, spv_member, constant.variables.emplace_back());
            }
        }

        inline void to_stage(const SpvReflectShaderStageFlagBits spv_stage, spirv_module_stage& stage)
        {
            static const std::map<SpvReflectShaderStageFlagBits, spirv_module_stage> stage_map {
                {SPV_REFLECT_SHADER_STAGE_VERTEX_BIT, spirv_module_stage::vertex},
                {SPV_REFLECT_SHADER_STAGE_TESSELLATION_CONTROL_BIT, spirv_module_stage::tesselation_control},
                {SPV_REFLECT_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, spirv_module_stage::tesselation_eval},
                {SPV_REFLECT_SHADER_STAGE_GEOMETRY_BIT, spirv_module_stage::geometry},
                {SPV_REFLECT_SHADER_STAGE_FRAGMENT_BIT, spirv_module_stage::fragment},
                {SPV_REFLECT_SHADER_STAGE_COMPUTE_BIT, spirv_module_stage::compute},
                {SPV_REFLECT_SHADER_STAGE_MESH_BIT_NV, spirv_module_stage::mesh},
                {SPV_REFLECT_SHADER_STAGE_MESH_BIT_EXT, spirv_module_stage::mesh},
            };

            const auto it_stage = stage_map.find(spv_stage);
            stage = it_stage != stage_map.end() ? it_stage->second : spirv_module_stage::none;
        }

        inline void to_module(std::string_view path, const SpvReflectShaderModule& spv_module, spirv_module& module)
        {
            module.path = path;
            module.entry_point = spv_module.entry_point_name;
            to_stage(spv_module.shader_stage, module.stage);

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

            defer destroy_module = [&] { spvReflectDestroyShaderModule(&spv_module); };
            detail::to_module(path, spv_module, module);
            return true;
        }
    };
}