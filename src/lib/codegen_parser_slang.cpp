#include "spore/codegen/parsers/slang/codegen_parser_slang.hpp"

#include <filesystem>
#include <string>

#include "slang-com-ptr.h"
#include "slang.h"
#include "spdlog/spdlog.h"

#include "spore/codegen/utils/files.hpp"

namespace spore::codegen
{
    namespace detail
    {
        struct slang_context
        {
            slang::IModule& module;
            slang::ISession& session;
            const slang::SessionDesc& session_desc;
        };

        void parse_args(const std::span<const std::string> args, std::vector<std::string>& out_args)
        {
            out_args.reserve(2 * args.size());
            // out_args.emplace_back(SPORE_CODEGEN_NAME);

            for (const std::string& arg : args)
            {
                const std::size_t index_equal = arg.find('=');

                if (index_equal != std::string::npos)
                {
                    out_args.emplace_back(arg.substr(0, index_equal));
                    out_args.emplace_back(arg.substr(index_equal + 1));
                }
                else
                {
                    out_args.emplace_back(arg);
                }
            }
        }

        slang::IGlobalSession& get_global_session()
        {
            static Slang::ComPtr<slang::IGlobalSession> global_session = [] {
                Slang::ComPtr<slang::IGlobalSession> value;
                slang::createGlobalSession(value.writeRef());
                return value;
            }();

            return *global_session;
        }

        std::string get_string(slang::IBlob& sl_blob)
        {
            return std::string(static_cast<const char*>(sl_blob.getBufferPointer()), sl_blob.getBufferSize());
        }

        std::string get_type_name(slang::TypeReflection& sl_type)
        {
            slang::IBlob* type_name_blob = nullptr;
            const SlangResult slang_result = sl_type.getFullName(&type_name_blob);

            if (SLANG_FAILED(slang_result))
            {
                return sl_type.getName();
            }

            return get_string(*type_name_blob);
        }

        std::string get_target_name(const slang::TargetDesc& sl_target_desc)
        {
            SlangCompileTarget sl_target = sl_target_desc.format;
            if (sl_target == SLANG_TARGET_UNKNOWN)
            {
                constexpr auto predicate = [](const slang::CompilerOptionEntry& entry) {
                    return entry.name == slang::CompilerOptionName::Target;
                };

                const auto sl_target_option = std::ranges::find_if(
                    sl_target_desc.compilerOptionEntries,
                    sl_target_desc.compilerOptionEntries + sl_target_desc.compilerOptionEntryCount,
                    predicate);

                if (sl_target_option != nullptr)
                {
                    sl_target = static_cast<SlangCompileTarget>(sl_target_option->value.intValue0);
                }
            }

            switch (sl_target)
            {
                case SLANG_GLSL:
                case SLANG_GLSL_VULKAN_DEPRECATED:
                case SLANG_GLSL_VULKAN_ONE_DESC_DEPRECATED:
                    return "glsl";
                case SLANG_HLSL:
                    return "hlsl";
                case SLANG_WGSL_SPIRV_ASM:
                case SLANG_WGSL_SPIRV:
                case SLANG_SPIRV:
                case SLANG_SPIRV_ASM:
                    return "spirv";
                case SLANG_DXBC:
                case SLANG_DXBC_ASM:
                    return "dxbc";
                case SLANG_DXIL:
                case SLANG_DXIL_ASM:
                    return "dxil";
                case SLANG_C_SOURCE:
                    return "c";
                case SLANG_CPP_SOURCE:
                    return "c++";
                case SLANG_CUDA_SOURCE:
                case SLANG_CUDA_OBJECT_CODE:
                    return "cuda";
                case SLANG_METAL:
                case SLANG_METAL_LIB:
                case SLANG_METAL_LIB_ASM:
                    return "metal";
                case SLANG_WGSL:
                    return "wgsl";
                default:
                    return "unknown";
            }
        }

        slang_layout_unit get_layout_unit(const slang::ParameterCategory sl_category)
        {
            switch (sl_category)
            {
                case slang::ParameterCategory::Uniform:
                    return slang_layout_unit::uniform;
                case slang::ParameterCategory::Mixed:
                    return slang_layout_unit::mixed;
                case slang::ParameterCategory::DescriptorTableSlot:
                    return slang_layout_unit::descriptor_table_slot;
                case slang::ParameterCategory::PushConstantBuffer:
                    return slang_layout_unit::push_constant_buffer;
                case slang::ParameterCategory::VaryingInput:
                    return slang_layout_unit::varying_input;
                case slang::ParameterCategory::VaryingOutput:
                    return slang_layout_unit::varying_output;
                case slang::ParameterCategory::SpecializationConstant:
                    return slang_layout_unit::specialization_constant;
                case slang::ParameterCategory::InputAttachmentIndex:
                    return slang_layout_unit::input_attachment_index;
                case slang::ParameterCategory::ConstantBuffer:
                    return slang_layout_unit::d3d_constant_buffer;
                case slang::ParameterCategory::ShaderResource:
                    return slang_layout_unit::d3d_shader_resource;
                case slang::ParameterCategory::UnorderedAccess:
                    return slang_layout_unit::d3d_unordered_access;
                case slang::ParameterCategory::SamplerState:
                    return slang_layout_unit::d3d_sampler_state;
                default:
                    return slang_layout_unit::none;
            }
        }

        slang_attribute make_attribute(slang::Attribute& sl_attribute)
        {
            slang_attribute attribute;
            attribute.name = sl_attribute.getName();

            const std::size_t argument_count = sl_attribute.getArgumentCount();
            attribute.args.reserve(argument_count);

            for (std::size_t index = 0; index < argument_count; ++index)
            {
                if (std::int32_t int_value; SLANG_SUCCEEDED(sl_attribute.getArgumentValueInt(index, &int_value)))
                {
                    attribute.args.emplace_back(int_value);
                }
                else if (std::float_t float_value; SLANG_SUCCEEDED(sl_attribute.getArgumentValueFloat(index, &float_value)))
                {
                    attribute.args.emplace_back(float_value);
                }
                else if (std::size_t string_size; const char* string_value = sl_attribute.getArgumentValueString(index, &string_size))
                {
                    attribute.args.emplace_back(std::string {string_value, string_size});
                }
            }

            return attribute;
        }

        slang_variable make_variable(const slang_context& context, slang::VariableReflection& sl_variable)
        {
            slang_variable variable;
            variable.name = sl_variable.getName();

            if (slang::TypeReflection* sl_type = sl_variable.getType())
            {
                variable.type = get_type_name(*sl_type);
                variable.targets.reserve(context.session_desc.targetCount);

                for (std::size_t index_target = 0; index_target < context.session_desc.targetCount; ++index_target)
                {
                    slang::ProgramLayout* sl_layout = context.module.getLayout(index_target);
                    if (sl_layout == nullptr)
                    {
                        continue;
                    }

                    slang::TypeLayoutReflection* sl_type_layout = sl_layout->getTypeLayout(sl_type);
                    if (sl_type_layout == nullptr)
                    {
                        continue;
                    }

                    slang::VariableLayoutReflection* sl_variable_layout = sl_type_layout->getElementVarLayout();
                    if (sl_variable_layout == nullptr)
                    {
                        continue;
                    }

                    slang_target<slang_variable_layout>& target = variable.targets.emplace_back();
                    target.name = get_target_name(context.session_desc.targets[index_target]);

                    const std::size_t layout_count = sl_variable_layout->getCategoryCount();
                    target.layouts.reserve(layout_count);

                    for (std::size_t index_layout = 0; index_layout < layout_count; ++index_layout)
                    {
                        const slang::ParameterCategory category = sl_variable_layout->getCategoryByIndex(index_layout);
                        slang_variable_layout& variable_layout = target.layouts.emplace_back();
                        variable_layout.unit = get_layout_unit(category);
                        variable_layout.offset = sl_variable_layout->getOffset(category);
                    }
                }
            }

            const std::size_t attribute_count = sl_variable.getUserAttributeCount();
            variable.attributes.reserve(attribute_count);

            for (std::size_t index = 0; index < attribute_count; ++index)
            {
                if (slang::Attribute* sl_attribute = sl_variable.getUserAttributeByIndex(index))
                {
                    variable.attributes.emplace_back(make_attribute(*sl_attribute));
                }
            }

            return variable;
        }

        slang_type make_type(const slang_context& context, slang::TypeReflection& sl_type)
        {
            slang_type type;
            type.name = sl_type.getName();

            const std::size_t field_num = sl_type.getFieldCount();

            type.fields.reserve(field_num);

            for (std::size_t index = 0; index < field_num; ++index)
            {
                if (slang::VariableReflection* sl_field = sl_type.getFieldByIndex(index))
                {
                    type.fields.emplace_back(make_variable(context, *sl_field));
                }
            }

            type.targets.reserve(context.session_desc.targetCount);

            for (std::size_t index_target = 0; index_target < context.session_desc.targetCount; ++index_target)
            {
                slang::ProgramLayout* sl_layout = context.module.getLayout(index_target);
                if (sl_layout == nullptr)
                {
                    continue;
                }

                slang::TypeLayoutReflection* sl_type_layout = sl_layout->getTypeLayout(&sl_type);
                if (sl_type_layout == nullptr)
                {
                    continue;
                }

                slang_target<slang_type_layout>& target = type.targets.emplace_back();
                target.name = get_target_name(context.session_desc.targets[index_target]);

                const std::size_t layout_count = sl_type_layout->getCategoryCount();
                target.layouts.reserve(layout_count);

                for (std::size_t index = 0; index < layout_count; ++index)
                {
                    const slang::ParameterCategory category = sl_type_layout->getCategoryByIndex(index);
                    slang_type_layout& type_layout = target.layouts.emplace_back();
                    type_layout.unit = get_layout_unit(category);
                    type_layout.size = sl_type_layout->getSize(category);
                    type_layout.stride = sl_type_layout->getStride(category);
                    type_layout.alignment = sl_type_layout->getAlignment(category);
                }
            }

            const std::size_t attribute_count = sl_type.getUserAttributeCount();
            type.attributes.reserve(attribute_count);

            for (std::size_t index = 0; index < attribute_count; ++index)
            {
                if (slang::Attribute* sl_attribute = sl_type.getUserAttributeByIndex(index))
                {
                    type.attributes.emplace_back(make_attribute(*sl_attribute));
                }
            }

            return type;
        }

        slang_entry_point make_entry_point(const slang_context& context, slang::IEntryPoint& sl_entry_point)
        {
            slang_entry_point entry_point;

            if (slang::FunctionReflection* sl_function = sl_entry_point.getFunctionReflection())
            {
                entry_point.name = sl_function->getName();

                const std::size_t input_count = sl_function->getParameterCount();
                entry_point.inputs.reserve(input_count);

                for (std::size_t index = 0; index < input_count; ++index)
                {
                    if (slang::VariableReflection* sl_variable = sl_function->getParameterByIndex(index))
                    {
                        entry_point.inputs.emplace_back(make_variable(context, *sl_variable));
                    }
                }

                const std::size_t attribute_count = sl_function->getUserAttributeCount();
                entry_point.attributes.reserve(attribute_count);

                for (std::size_t index = 0; index < attribute_count; ++index)
                {
                    if (slang::Attribute* sl_attribute = sl_function->getUserAttributeByIndex(index))
                    {
                        entry_point.attributes.emplace_back(make_attribute(*sl_attribute));
                    }
                }
            }

            return entry_point;
        }

        slang_module make_module(const slang_context& context)
        {
            slang_module module;
            module.path = context.module.getFilePath();

            SlangResult slang_result = SLANG_OK;
            const std::size_t entry_point_num = static_cast<std::size_t>(context.module.getDefinedEntryPointCount());

            module.entry_points.reserve(entry_point_num);

            for (std::size_t index = 0; index < entry_point_num; ++index)
            {
                slang::IEntryPoint* entry_point = nullptr;
                slang_result = context.module.getDefinedEntryPoint(index, &entry_point);

                if (SLANG_FAILED(slang_result))
                {
                    SPDLOG_WARN("failed to get slang entry point, index={}", index);
                    continue;
                }

                module.entry_points.emplace_back(make_entry_point(context, *entry_point));
            }

            if (slang::DeclReflection* sl_decl = context.module.getModuleReflection())
            {
                const std::size_t children_count = sl_decl->getChildrenCount();

                for (std::size_t index = 0; index < children_count; ++index)
                {
                    if (slang::DeclReflection* sl_child_decl = sl_decl->getChild(index))
                    {
                        switch (sl_child_decl->getKind())
                        {
                            case slang::DeclReflection::Kind::Namespace:
                                break;
                            case slang::DeclReflection::Kind::Struct:
                                module.types.emplace_back(make_type(context, *sl_child_decl->getType()));
                                break;
                            case slang::DeclReflection::Kind::Variable:
                                module.variables.emplace_back(make_variable(context, *sl_child_decl->asVariable()));
                                break;
                            case slang::DeclReflection::Kind::Generic:
                                break;
                            case slang::DeclReflection::Kind::Func:
                                break;
                            case slang::DeclReflection::Kind::Module:
                            case slang::DeclReflection::Kind::Unsupported:
                            default:
                                break;
                        }
                    }
                }
            }

            return module;
        }
    }

    codegen_parser_slang::codegen_parser_slang(const std::span<const std::string> args)
    {
        detail::parse_args(args, slang_args);
    }

    bool codegen_parser_slang::parse_asts(const std::vector<std::string>& paths, std::vector<slang_module>& modules)
    {
        SlangResult slang_result = SLANG_OK;

        slang::IGlobalSession& global_session = detail::get_global_session();

        std::vector<const char*> args;
        args.reserve(slang_args.size() + paths.size());

        constexpr auto transformer = [](const std::string& path) { return path.data(); };
        std::ranges::transform(slang_args, std::back_inserter(args), transformer);
        args.emplace_back("--");
        std::ranges::transform(paths, std::back_inserter(args), transformer);

        slang::SessionDesc session_desc;
        Slang::ComPtr<ISlangUnknown> session_memory;
        slang_result = global_session.parseCommandLineArguments(args.size(), args.data(), &session_desc, session_memory.writeRef());

        if (SLANG_FAILED(slang_result))
        {
            SPDLOG_ERROR("failed to parse slang arguments, error={}", slang::getLastInternalErrorMessage());
            return false;
        }

        Slang::ComPtr<slang::ISession> session;
        slang_result = global_session.createSession(session_desc, session.writeRef());

        if (SLANG_FAILED(slang_result))
        {
            SPDLOG_ERROR("failed to create slang session, error={}", slang::getLastInternalErrorMessage());
            return false;
        }

        modules.reserve(paths.size());

        for (const std::string& path : paths)
        {
            std::filesystem::path file_path = std::filesystem::absolute(path);
            std::string file_data;

            if (not files::read_file(path, file_data))
            {
                SPDLOG_ERROR("failed to read file, path={}", path);
                return false;
            }

            slang::IModule* module = session->loadModuleFromSourceString(
                file_path.filename().generic_string().data(),
                file_path.generic_string().data(),
                file_data.data(),
                nullptr);

            if (module == nullptr)
            {
                SPDLOG_ERROR("failed to load module from file, path={} error={}", path, slang::getLastInternalErrorMessage());
                return false;
            }

            const detail::slang_context context {
                .module = *module,
                .session = *session,
                .session_desc = session_desc,
            };

            modules.emplace_back(detail::make_module(context));
        }

        return true;
    }
}