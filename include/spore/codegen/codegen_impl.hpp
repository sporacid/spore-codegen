#pragma once

#include "nlohmann/json.hpp"

#include "spore/codegen/parsers/codegen_condition.hpp"
#include "spore/codegen/parsers/codegen_converter.hpp"
#include "spore/codegen/parsers/codegen_parser.hpp"

namespace spore::codegen
{
    template <typename ast_t>
    struct codegen_impl_traits;

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
            return codegen_impl_traits<ast_t>::name();
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
            codegen_impl_traits<ast_t>::register_conditions(factory);
            return std::ignore;
        }();
    };
}
