#include "spore/codegen/parsers/slang/codegen_parser_slang.hpp"

#include <filesystem>

#include "slang-com-helper.h"
#include "slang-com-ptr.h"
#include "slang.h"
#include "spdlog/spdlog.h"

#include "spore/codegen/utils/files.hpp"

namespace spore::codegen
{
    namespace detail
    {
        slang::IGlobalSession& get_global_session()
        {
            static slang::IGlobalSession* global_session = [] {
                slang::IGlobalSession* value = nullptr;
                slang::createGlobalSession(&value);
                return value;
            }();

            return *global_session;
        }

        std::string make_string(slang::IBlob& sl_blob)
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

            return make_string(*type_name_blob);
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

        slang_type make_type(slang::IModule& sl_module, slang::TypeReflection& sl_type);

        slang_variable make_variable(slang::IModule& sl_module, slang::VariableReflection& sl_variable)
        {
            slang_variable variable;
            variable.name = sl_variable.getName();

            if (slang::TypeReflection* sl_type = sl_variable.getType())
            {
                // variable.type = get_type_name(*sl_type);
                variable.type = make_type(sl_module, *sl_type);

                if (slang::ProgramLayout* sl_layout = sl_module.getLayout())
                {
                    if (slang::TypeLayoutReflection* sl_type_layout = sl_layout->getTypeLayout(sl_type))
                    {
                        if (slang::VariableLayoutReflection* sl_variable_layout = sl_type_layout->getElementVarLayout())
                        {
                            const std::size_t layout_count = sl_variable_layout->getCategoryCount();
                            variable.layouts.reserve(layout_count);

                            for (std::size_t index = 0; index < layout_count; ++index)
                            {
                                const slang::ParameterCategory category = sl_variable_layout->getCategoryByIndex(index);
                                slang_variable_layout& variable_layout = variable.layouts.emplace_back();
                                variable_layout.unit = get_layout_unit(category);
                                variable_layout.offset = sl_variable_layout->getOffset(category);
                            }
                        }
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

        slang_type make_type(slang::IModule& sl_module, slang::TypeReflection& sl_type)
        {
            slang_type type;
            type.name = sl_type.getName();

            const std::size_t field_num = sl_type.getFieldCount();

            type.fields.reserve(field_num);

            for (std::size_t index = 0; index < field_num; ++index)
            {
                if (slang::VariableReflection* sl_field = sl_type.getFieldByIndex(index))
                {
                    type.fields.emplace_back(make_variable(sl_module, *sl_field));
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

        slang_entry_point make_entry_point(slang::IModule& sl_module, slang::IEntryPoint& sl_entry_point)
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
                        entry_point.inputs.emplace_back(make_variable(sl_module, *sl_variable));
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

        slang_module make_module(slang::IModule& sl_module)
        {
            slang_module module;
            module.path = sl_module.getFilePath();

            SlangResult slang_result = SLANG_OK;
            const std::size_t entry_point_num = static_cast<std::size_t>(sl_module.getDefinedEntryPointCount());

            module.entry_points.reserve(entry_point_num);

            for (std::size_t index = 0; index < entry_point_num; ++index)
            {
                slang::IEntryPoint* entry_point = nullptr;
                slang_result = sl_module.getDefinedEntryPoint(index, &entry_point);

                if (SLANG_FAILED(slang_result))
                {
                    SPDLOG_WARN("failed to get slang entry point, index={}", index);
                    continue;
                }

                module.entry_points.emplace_back(make_entry_point(sl_module, *entry_point));
            }

            if (slang::DeclReflection* sl_decl = sl_module.getModuleReflection())
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
                                module.types.emplace_back(make_type(sl_module, *sl_child_decl->getType()));
                                break;
                            case slang::DeclReflection::Kind::Variable:
                                module.variables.emplace_back(make_variable(sl_module, *sl_child_decl->asVariable()));
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

    bool codegen_parser_slang::parse_asts(const std::vector<std::string>& paths, std::vector<slang_module>& modules)
    {
        SlangResult slang_result = SLANG_OK;

        slang::IGlobalSession& global_session = detail::get_global_session();

#if 0
        std::vector<const char*> args;
        args.resize(slang_args.size());
        std::ranges::transform(slang_args, args.begin(),
            [](const std::string& arg) { return arg.data(); });

        slang::SessionDesc session_desc;
        Slang::ComPtr<ISlangUnknown> session_memory;
        slang_result = global_session.parseCommandLineArguments(args.size(), args.data(), &session_desc, session_memory.writeRef());

        if (SLANG_FAILED(slang_result))
        {
            SPDLOG_ERROR("failed to parse slang arguments, error={}", slang::getLastInternalErrorMessage());
            return false;
        }
#else
        constexpr slang::TargetDesc target_descs[] {
            slang::TargetDesc {
                .format = SLANG_SPIRV,
            },
            slang::TargetDesc {
                .format = SLANG_DXBC,
            },
        };

        const slang::SessionDesc session_desc {
            .targets = target_descs,
            .targetCount = 2,
        };
#endif

        slang::ISession* session = nullptr;
        slang_result = global_session.createSession(session_desc, &session);

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

            modules.emplace_back(detail::make_module(*module));
        }

        return true;
    }
}