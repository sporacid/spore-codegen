#include "spore/codegen/parsers/slang/codegen_parser_slang.hpp"

#include <filesystem>

#include "slang-com-helper.h"
#include "slang.h"

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

        slang_variable make_variable(slang::IModule& sl_module, slang::VariableReflection& sl_variable)
        {
            slang_variable variable;
            variable.name = sl_variable.getName();

            if (slang::TypeReflection* sl_type = sl_variable.getType())
            {
                variable.type = get_type_name(*sl_type);

                if (slang::ProgramLayout* sl_layout = sl_module.getLayout())
                {
                    if (slang::TypeLayoutReflection* sl_type_layout = sl_layout->getTypeLayout(sl_type))
                    {
                        if (slang::VariableLayoutReflection* sl_variable_layout = sl_type_layout->getElementVarLayout())
                        {
                            variable.layout = slang_layout {
                                .space = sl_variable_layout->getBindingSpace(),
                                .binding = sl_variable_layout->getBindingIndex(),
                            };
                        }
                    }
                }
            }

            return variable;
        }

        slang_struct make_struct(slang::IModule& sl_module, slang::TypeReflection& sl_type)
        {
            slang_struct struct_;
            struct_.name = sl_type.getName();

            const std::size_t field_num = sl_type.getFieldCount();

            struct_.fields.reserve(field_num);

            for (std::size_t index = 0; index < field_num; ++index)
            {
                if (slang::VariableReflection* sl_field = sl_type.getFieldByIndex(index))
                {
                    struct_.fields.emplace_back(make_variable(sl_module, *sl_field));
                }
            }

            return struct_;
        }

        slang_entry_point make_entry_point(slang::IModule& sl_module, slang::IEntryPoint& sl_entry_point)
        {
            slang_entry_point entry_point;

            if (slang::FunctionReflection* sl_function = sl_entry_point.getFunctionReflection())
            {
                entry_point.name = sl_function->getName();
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
                                module.structs.emplace_back(make_struct(sl_module, *sl_child_decl->getType()));
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

        const slang::TargetDesc target_desc {
            .format = SLANG_SPIRV,
        };

        const slang::SessionDesc session_desc {
            .targets = &target_desc,
            .targetCount = 1,
        };

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