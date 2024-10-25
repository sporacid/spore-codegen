#pragma once

#include <filesystem>
#include <map>
#include <set>
#include <string>
#include <string_view>

#include "nlohmann/json.hpp"

#include "spore/codegen/codegen_version.hpp"
#include "spore/codegen/utils/files.hpp"
#include "spore/codegen/utils/json.hpp"

namespace spore::codegen
{
    enum class codegen_cache_status
    {
        up_to_date,
        new_,
        dirty,
    };

    struct codegen_cache_entry
    {
        std::string file;
        std::string hash;
        std::size_t size = 0;
    };

    struct codegen_cache
    {
        struct entry_comparator
        {
            using is_transparent = void;

            bool operator()(const codegen_cache_entry& entry, const codegen_cache_entry& other_entry) const
            {
                return entry.file < other_entry.file;
            }

            bool operator()(const codegen_cache_entry& entry, const std::string_view& other_file) const
            {
                return entry.file < other_file;
            }

            bool operator()(const std::string_view& file, const codegen_cache_entry& other_entry) const
            {
                return file < other_entry.file;
            }
        };

        std::string version;
        std::set<codegen_cache_entry, entry_comparator> entries;

        [[nodiscard]] bool empty() const
        {
            return entries.empty();
        }

        [[nodiscard]] codegen_cache_status check_and_update(const std::string_view file)
        {
            std::string hash;
            if (!files::hash_file(file, hash))
            {
                return codegen_cache_status::dirty;
            }

            const std::size_t file_size = std::filesystem::file_size(file);
            const auto make_entry = [&] {
                return codegen_cache_entry {
                    .file = std::string(file),
                    .hash = std::move(hash),
                    .size = file_size,
                };
            };

            const auto it_entry = entries.find(file);

            if (it_entry == entries.end())
            {
                entries.emplace(make_entry());
                return codegen_cache_status::new_;
            }

            const bool is_entry_dirty = file_size != it_entry->size || hash != it_entry->hash;

            if (is_entry_dirty)
            {
                entries.erase(it_entry);
                entries.emplace(make_entry());
                return codegen_cache_status::dirty;
            }

            return codegen_cache_status::up_to_date;
        }

        void reset()
        {
            version = SPORE_CODEGEN_VERSION;
            entries.clear();
        }
    };

    namespace detail
    {
        constexpr std::string_view cache_context = "cache";
    }

    inline void to_json(nlohmann::json& json, const codegen_cache_entry& value)
    {
        json["file"] = value.file;
        json["hash"] = value.hash;
        json["size"] = value.size;
    }

    inline void from_json(const nlohmann::json& json, codegen_cache_entry& value)
    {
        json::get_checked(json, "file", value.file, detail::cache_context);
        json::get_checked(json, "hash", value.hash, detail::cache_context);
        json::get_checked(json, "size", value.size, detail::cache_context);
    }

    inline void to_json(nlohmann::json& json, const codegen_cache& value)
    {
        json["version"] = SPORE_CODEGEN_VERSION;
        json["entries"] = value.entries;
    }

    inline void from_json(const nlohmann::json& json, codegen_cache& value)
    {
        json::get_checked(json, "version", value.version, detail::cache_context);
        json::get_checked(json, "entries", value.entries, detail::cache_context);
    }

    inline void to_json(nlohmann::json& json, const codegen_cache_status& value)
    {
        static const std::map<codegen_cache_status, std::string_view> value_map {
            {codegen_cache_status::up_to_date, "up-to-date"},
            {codegen_cache_status::new_, "new"},
            {codegen_cache_status::dirty, "dirty"},
        };

        const auto it_value = value_map.find(value);

        if (it_value != value_map.end())
        {
            json = it_value->second;
        }
    }
}
