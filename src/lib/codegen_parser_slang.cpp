#include "spore/codegen/parsers/slang/codegen_parser_slang.hpp"

#include <filesystem>

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

        slang_module make_module(const slang::IModule& sl_module)
        {
            slang_module module;
            return module;
        }
    }

    bool codegen_parser_slang::parse_asts(const std::vector<std::string>& paths, std::vector<slang_module>& modules)
    {
        SlangResult slang_result = SLANG_OK;

        slang::IGlobalSession& global_session = detail::get_global_session();

        constexpr slang::SessionDesc session_desc;

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

            const slang::IModule* module = session->loadModuleFromSourceString(
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