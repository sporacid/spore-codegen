#pragma once

#include "nlohmann/json.hpp"

#include "spore/codegen/parsers/codegen_condition.hpp"
#include "spore/codegen/parsers/cpp/codegen_condition_cpp_attribute.hpp"
#include "spore/codegen/parsers/cpp/codegen_converter_cpp.hpp"
#include "spore/codegen/parsers/cpp/codegen_parser_cpp.hpp"
#include "spore/codegen/parsers/spirv/codegen_converter_spirv.hpp"
#include "spore/codegen/parsers/spirv/codegen_parser_spirv.hpp"

namespace spore::codegen
{
    namespace detail
    {
        template <typename ast_t>
        constexpr std::string_view impl_name;

        template <>
        constexpr std::string_view impl_name<cpp_file> = "cpp";

        template <>
        constexpr std::string_view impl_name<spirv_module> = "spirv";

        template <typename ast_t>
        void register_default_conditions(codegen_condition_factory<ast_t>& factory)
        {
            factory.template register_condition<codegen_condition_any<ast_t>>();
            factory.template register_condition<codegen_condition_all<ast_t>>();
            factory.template register_condition<codegen_condition_none<ast_t>>();
        }

        template <typename ast_t>
        void register_conditions(codegen_condition_factory<ast_t>& factory);

        template <>
        inline void register_conditions<cpp_file>(codegen_condition_factory<cpp_file>& factory)
        {
            register_default_conditions<cpp_file>(factory);
            factory.register_condition<codegen_condition_cpp_attribute>();
        }

        template <>
        inline void register_conditions<spirv_module>(codegen_condition_factory<spirv_module>& factory)
        {
            register_default_conditions<spirv_module>(factory);
        }
    }

    template <typename ast_t>
    struct codegen_impl
    {
        codegen_impl() = default;

        template <typename parser_t, typename converter_t>
        codegen_impl(parser_t&& parser, converter_t&& converter) requires
            std::derived_from<std::decay_t<parser_t>, codegen_parser<ast_t>>and std::derived_from<std::decay_t<converter_t>, codegen_converter<ast_t>>
        {
            _parser = std::make_unique<std::decay_t<parser_t>>(std::forward<parser_t>(parser));
            _converter = std::make_unique<std::decay_t<converter_t>>(std::forward<converter_t>(converter));
        }

        static constexpr std::string_view name()
        {
            return detail::impl_name<ast_t>;
        }

        codegen_parser<ast_t>& parser() const
        {
            return *_parser;
        }

        codegen_converter<ast_t>& converter() const
        {
            return *_converter;
        }

        std::shared_ptr<codegen_condition<ast_t>> condition(const nlohmann::json& data) const
        {
            return codegen_condition_factory<ast_t>::get().make_condition(data);
        }

      private:
        std::unique_ptr<codegen_parser<ast_t>> _parser;
        std::unique_ptr<codegen_converter<ast_t>> _converter;

        static inline auto _setup = [] {
            codegen_condition_factory<ast_t>& factory = codegen_condition_factory<ast_t>::get();
            detail::register_conditions(factory);
            return std::ignore;
        }();
    };

    using codegen_impl_cpp = codegen_impl<cpp_file>;
    using codegen_impl_spirv = codegen_impl<spirv_module>;
}
