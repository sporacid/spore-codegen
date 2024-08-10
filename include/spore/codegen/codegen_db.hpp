#pragma once

#include <array>
#include <string>
#include <string_view>

#include "nlohmann/json.hpp"

#include "spore/codegen/ast/ast_file.hpp"

// #ifndef SPORE_CODEGEN_KEY_COMPONENT_COUNT
// #define SPORE_CODEGEN_KEY_COMPONENT_COUNT 16
// #endif
//
// #ifndef SPORE_CODEGEN_KEY_COUNT
// #define SPORE_CODEGEN_KEY_COUNT 8
// #endif

// #ifndef SPORE_CODEGEN_KEY_NAME_COUNT
// #define SPORE_CODEGEN_KEY_NAME_COUNT 32
// #endif
//
// #ifndef SPORE_CODEGEN_KEY_PATH_COUNT
// #define SPORE_CODEGEN_KEY_PATH_COUNT 32
// #endif

namespace spore::codegen
{
    //    enum class codegen_match_type
    //    {
    //        none,
    //        exact,
    //        partial,
    //    };

    struct codegen_key
    {
        std::string name;
        std::vector<std::string> aliases;
    };

    template <typename ast_t>
    struct codegen_value
    {
        const ast_t* ast = nullptr;
        const nlohmann::json* json = nullptr;
    };

    template <typename ast_t, typename traits_t>
    struct codegen_collection
    {
        using value_type = std::tuple<const ast_t&, const nlohmann::json&>;
        using pair_type = std::tuple<codegen_key, value_type>;
        using index_type = std::tuple<std::size_t, std::size_t>;

        std::vector<pair_type> values;
        std::map<std::string, index_type> index_map;

        void add(const ast_t& ast, const nlohmann::json& json)
        {
            codegen_key key = traits_t::get_key(ast);

            std::size_t index = values.size();
            std::size_t relevancy = key.aliases.size();

            values.emplace(key, {ast, json});

            for (std::string& alias : key.aliases)
            {
                index_map.emplace(std::move(alias), index_type {index, relevancy--});
            }

            index_map.emplace(std::move(key.name), index_type {index, std::numeric_limits<std::size_t>::max()});
        }

        value_type find(std::string_view value)
        {
            const auto is_perfect = [](const index_type& match) {
                const auto [_, relevancy] = match;
                return relevancy == std::numeric_limits<std::size_t>::max();
            };

            const auto add_match = [&](std::string_view key, const index_type& match) {
                return *index_map.emplace(key, match).first;
            };

            codegen_key partial_key = traits_t::get_partial_key(value);
            index_type best_match {std::numeric_limits<std::size_t>::max(), 0};

            auto it_index = index_map.find(partial_key.name);

            if (it_index != index_map.end())
            {
                best_match = it_index->second;

                if (is_perfect(best_match))
                {
                    return best_match;
                }
            }

            for (const std::string& alias : partial_key.aliases)
            {
                it_index = index_map.find(alias);
                if (it_index != index_map.end())
                {
                    const auto [index, relevancy] = it_index->second;
                    const auto [best_index, best_relevancy] = best_match;

                    if (relevancy > best_relevancy)
                    {
                        best_match = it_index->second;

                        if (is_perfect(best_match))
                        {
                            return add_match(best_match);
                        }
                    }
                }
            }

            const auto [index, relevancy] = best_match;
            if (index != std::numeric_limits<std::size_t>::max() && relevancy > 0)
            {
                return add_match(best_match);
            }

            return best_match;

            //            trait_t::match_key()
            //                index_map.find(name)

            // -- in
            //  1- (!) std::float_t vs float
            //  2- light vs spore::light
            //  3- v2::light vs spore::v2::light
            //  4- shader<layout_t> vs shader<*> vs shader<my_layout> vs explicit instantiation of shader<my_layout>
            //  5- shader<attribute_t, uniform_t> vs shader<*> vs shader<my_layout> vs explicit instantiation of shader<my_layout>
            //  6- include/header.hpp vs C:/project/include/header.hpp
            //
            // -- out
            //  1- null, we don't keep std::float_t internally
            //  2- codegen_key{ "spore::light", {"light"} };
            //  3- codegen_key{ "spore::v2::light", {"v2::light", "light"} };
            //  4- codegen_key{ "spore::shader<*>", {"shader<*>"} };
            //     codegen_key{ "spore::shader<my_layout>", {"shader<my_layout>"} };
            //  5- codegen_key{ "spore::shader<*,*>", {"shader<*,*>"} };
            //  6- codegen_key{ std::filesystem::absolute("include/header.hpp"), {"include/header.hpp"} };

            // if full_name match -> 100%
            // if component match -> 100%
            // if template ->
            //    foreach params ->
            //      if wildcard or matches ->
            //        ++relevancy;
            //      else if not wildcard ->
            //        relevancy = 0
            //        break

            throw int();
        }
    };

    struct ast_file_traits
    {
        static codegen_key get_key(const ast_type_t& ast_type)
        {
            return codegen_key {};
        }

        static codegen_key get_partial_key(std::string_view partial_key)
        {
            return codegen_key {};
        }
        //        static void get_keys(const ast_file& ast_file, std::vector<std::string>& keys)
        //        {
        //        }
        //
        //        static codegen_match_type match_key(const ast_file& ast_file, std::string_view key)
        //        {
        //            return codegen_match_type::none;
        //            // -- in
        //            //  6- include/header.hpp vs C:/project/include/header.hpp
        //            //
        //            // -- out
        //            //  6- codegen_key{ std::filesystem::absolute("include/header.hpp"), {"include/header.hpp"} };
        //        }
    };

    template <typename ast_type_t>
    struct ast_type_traits
    {
        static codegen_key get_key(const ast_type_t& ast_type)
        {
            return codegen_key {};
        }

        static codegen_key get_partial_key(std::string_view partial_key)
        {
            return codegen_key {};
        }
        //
        //        static codegen_match_type match_key(const ast_type_t& ast_type, std::string_view key)
        //        {
        //            /*
        //             std::size_t
        //
        //             */
        //
        //            if constexpr (std::derived_from<ast_has_template_params<ast_type_t>, ast_type_t>)
        //            {
        //                // const auto& template_param : ast_type.template_params
        //                for (std::size_t index_i = 0; index_i < ast_type.template_params.size(); ++index_i)
        //                {
        //                }
        //            }
        //
        //            return codegen_match_type::none;
        //            // -- in
        //            //  2- light vs spore::light
        //            //  3- v2::light vs spore::v2::light
        //            //  4- shader<layout_t> vs shader<*> vs shader<my_layout> vs explicit instantiation of shader<my_layout>
        //            //  5- shader<attribute_t, uniform_t> vs shader<*> vs shader<my_layout> vs explicit instantiation of shader<my_layout>
        //            //
        //            // -- out
        //            //  2- codegen_key{ "spore::light", {"light"} };
        //            //  3- codegen_key{ "spore::v2::light", {"v2::light", "light"} };
        //            //  4- codegen_key{ "spore::shader<*>", {"shader<*>"} };
        //            //     codegen_key{ "spore::shader<my_layout>", {"shader<my_layout>"} };
        //            //  5- codegen_key{ "spore::shader<*,*>", {"shader<*,*>"} };
        //        }
    };

    struct codegen_db
    {
        // std::vector<codegen_value<ast_file>> files;
        // std::vector<codegen_value<ast_class>> classes;
        // std::vector<codegen_value<ast_enum>> enums;
        //
        // std::unordered_map<std::string_view, std::size_t> path_map;
        // std::unordered_map<std::string_view, std::size_t> full_name_map;
        // std::unordered_multimap<std::string_view, std::size_t> name_map;
        // std::unordered_multimap<std::string_view, std::size_t> component_map;

        template <typename ast_t>
        void add_value(const ast_t& ast, const nlohmann::json& json)
        {
        }

        const nlohmann::json& find_by_path(std::string_view path)
        {
        }

        const nlohmann::json& find_by_name(std::string_view name)
        {
        }

      private:
        codegen_collection<ast_file, ast_file_traits> files;
        codegen_collection<ast_class, ast_type_traits<ast_class>> classes;
        codegen_collection<ast_enum, ast_type_traits<ast_enum>> enums;
    };

#if 0
    struct codegen_key
    {
        std::string value;
        std::array<std::string_view, SPORE_CODEGEN_KEY_COMPONENT_COUNT> components;

        bool valid() const
        {
            return !value.empty();
        }
    };
    //    struct codegen_key
    //    {
    //        std::string path;
    //        std::string full_name;
    //        std::array<std::string_view, SPORE_CODEGEN_KEY_NAME_COUNT> names;
    //        std::array<std::string_view, SPORE_CODEGEN_KEY_PATH_COUNT> components;
    //    };

    template <typename ast_t>
    struct codegen_value
    {
        std::array<codegen_key, SPORE_CODEGEN_KEY_COUNT> keys;
        const ast_t* ast = nullptr;
        const nlohmann::json* json = nullptr;
    };

    template <typename ast_t>
    struct codegen_collection
    {
        std::vector<codegen_value<ast_t>> values;
        std::unordered_map<std::string_view, std::size_t> index_map;
        std::function<void(const ast_t&, std::span<codegen_key>)> keys_func;
        std::function<bool(const codegen_value<ast_t>&)> match_func;

        const codegen_value<ast_t>& add(const ast_t& ast, const nlohmann::json& json)
        {
            std::size_t index = values.size();

            codegen_value<ast_t> value = values.emplace();
            value.ast = &ast;
            value.json = &json;

            keys_func(ast, value.keys);

            for (std::size_t index_key = 0; index_key < SPORE_CODEGEN_KEY_COUNT && value.keys[index_key].valid(); ++index_key)
            {
                const codegen_key& key = value.keys[index_key];

                index_map.emplace(key.value, index);

                for (std::string_view component : key.components)
                {
                    index_map.emplace(component, index);
                }
            }
        }

        const codegen_value<ast_t>& find(std::string_view name)
        {
            // -- in
            //  1- (!) std::float_t vs float
            //  2- light vs spore::light
            //  3- v2::light vs spore::v2::light
            //  4- shader<layout_t> vs shader<*> vs shader<my_layout> vs explicit instantiation of shader<my_layout>
            //  5- shader<attribute_t, uniform_t> vs shader<*> vs shader<my_layout> vs explicit instantiation of shader<my_layout>
            //  6- include/header.hpp vs C:/project/include/header.hpp
            //
            // -- out
            //  1- null, we don't keep std::float_t internally
            //  2- codegen_key{ "spore::light", {"light"} };
            //  3- codegen_key{ "spore::v2::light", {"v2::light", "light"} };
            //  4- codegen_key{ "spore::shader<*>", {"shader<*>"} };
            //     codegen_key{ "spore::shader<my_layout>", {"shader<my_layout>"} };
            //  5- codegen_key{ "spore::shader<*,*>", {"shader<*,*>"} };
            //  6- codegen_key{ std::filesystem::absolute("include/header.hpp"), {"include/header.hpp"} };

            const auto it_index = index_map.find(name);

            // if full_name match -> 100%
            // if component match -> 100%
            // if template ->
            //    foreach params ->
            //      if wildcard or matches ->
            //        ++relevancy;
            //      else if not wildcard ->
            //        relevancy = 0
            //        break

            throw int();
        }
    };

    struct codegen_db
    {
        std::vector<codegen_value<ast_file>> files;
        std::vector<codegen_value<ast_class>> classes;
        std::vector<codegen_value<ast_enum>> enums;

        std::unordered_map<std::string_view, std::size_t> path_map;
        std::unordered_map<std::string_view, std::size_t> full_name_map;
        std::unordered_multimap<std::string_view, std::size_t> name_map;
        std::unordered_multimap<std::string_view, std::size_t> component_map;

        template <typename ast_t>
        void add_value(const ast_t& ast, const nlohmann::json& json)
        {
        }

        const nlohmann::json& find_by_path(std::string_view path)
        {
        }

        const nlohmann::json& find_by_name(std::string_view name)
        {
        }

      private:
        void index_db()
        {
        }

        template <typename ast_t>
        static void get_keys(const ast_t& ast, std::span<codegen_key> keys);

        template <>
        static void get_keys<ast_file>(const ast_file& ast_file, std::span<codegen_key> keys)
        {

        }

        template <>
        static void get_keys<ast_class>(const ast_class& ast_class, std::span<codegen_key> keys)
        {

        }

        template <>
        static void get_keys<ast_enum>(const ast_enum& ast_class, std::span<codegen_key> keys)
        {

        }
    };
#endif
}