#pragma once

#include <filesystem>
#include <map>
#include <string>
#include <string_view>
#include <vector>

#include "nlohmann/json.hpp"
#include "spdlog/spdlog.h"

#include "spore/codegen/codegen_version.hpp"
#include "spore/codegen/utils/files.hpp"

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
        std::int64_t mtime = 0;

        bool operator<(const codegen_cache_entry& other) const
        {
            return file < other.file;
        }

        bool operator==(const codegen_cache_entry& other) const
        {
            return file == other.file;
        }

        bool operator<(const std::string& other) const
        {
            return file < other;
        }

        bool operator==(const std::string& other) const
        {
            return file == other;
        }
    };

    struct codegen_cache
    {
        std::string version;
        std::vector<codegen_cache_entry> entries;

        [[nodiscard]] bool empty() const
        {
            return entries.empty();
        }

        [[nodiscard]] codegen_cache_status check_and_update(const std::string& file)
        {
            std::string hash;
            if (!files::hash_file(file, hash))
            {
                return codegen_cache_status::dirty;
            }

            std::int64_t mtime = std::filesystem::last_write_time(file).time_since_epoch().count();

            const auto it = std::lower_bound(entries.begin(), entries.end(), file);
            if (it == entries.end() || it->file != file)
            {
                codegen_cache_entry entry;
                entry.file = file;
                entry.hash = std::move(hash);
                entry.mtime = mtime;
                entries.insert(it, std::move(entry));
                return codegen_cache_status::new_;
            }

            if (mtime != it->mtime || hash != it->hash)
            {
                codegen_cache_entry& entry = *it;
                entry.hash = std::move(hash);
                entry.mtime = mtime;
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

    inline void to_json(nlohmann::json& json, const codegen_cache_entry& value)
    {
        json["file"] = value.file;
        json["hash"] = value.hash;
        json["mtime"] = value.mtime;
    }

    inline void from_json(const nlohmann::json& json, codegen_cache_entry& value)
    {
        json["file"].get_to(value.file);
        json["hash"].get_to(value.hash);

        if (json.contains("mtime"))
        {
            json["mtime"].get_to(value.mtime);
        }
    }

    inline void to_json(nlohmann::json& json, const codegen_cache& value)
    {
        json["version"] = SPORE_CODEGEN_VERSION;
        json["entries"] = value.entries;
    }

    inline void from_json(const nlohmann::json& json, codegen_cache& value)
    {
        if (json.type() == nlohmann::json::value_t::array)
        {
            json.get_to(value.entries);
        }
        else
        {
            json["version"].get_to(value.version);
            json["entries"].get_to(value.entries);
        }
    }

    inline void to_json(nlohmann::json& json, const codegen_cache_status& value)
    {
        static const std::map<codegen_cache_status, std::string_view> name_map {
            {codegen_cache_status::up_to_date, "up-to-date"},
            {codegen_cache_status::new_, "new"},
            {codegen_cache_status::dirty, "dirty"},
        };

        const auto it_name = name_map.find(value);

        if (it_name != name_map.end())
        {
            json = it_name->second;
        }
    }
}
