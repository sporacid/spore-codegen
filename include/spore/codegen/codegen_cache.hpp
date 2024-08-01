#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include "nlohmann/json.hpp"
#include "spdlog/spdlog.h"

#include "spore/codegen/codegen_version.hpp"
#include "spore/codegen/utils/files.hpp"

namespace spore::codegen
{
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
        enum class status
        {
            up_to_date,
            new_,
            dirty,
        };

        std::string version;
        std::vector<codegen_cache_entry> entries;

        status check_and_update(const std::string& file)
        {
            std::string hash;
            if (!files::hash_file(file, hash))
            {
                return status::dirty;
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
                return status::new_;
            }

            if (mtime != it->mtime || hash != it->hash)
            {
                codegen_cache_entry& entry = *it;
                entry.hash = std::move(hash);
                entry.mtime = mtime;
                return status::dirty;
            }

            return status::up_to_date;
        }

        void reset()
        {
            version = SPORE_CODEGEN_VERSION;
            entries.clear();
        }

        bool empty() const
        {
            return entries.empty();
        }
    };

    void to_json(nlohmann::json& json, const codegen_cache_entry& value)
    {
        json["file"] = value.file;
        json["hash"] = value.hash;
        json["mtime"] = value.mtime;
    }

    void from_json(const nlohmann::json& json, codegen_cache_entry& value)
    {
        json["file"].get_to(value.file);
        json["hash"].get_to(value.hash);

        if (json.contains("mtime"))
        {
            json["mtime"].get_to(value.mtime);
        }
    }

    void to_json(nlohmann::json& json, const codegen_cache& value)
    {
        json["version"] = SPORE_CODEGEN_VERSION;
        json["entries"] = value.entries;
    }

    void from_json(const nlohmann::json& json, codegen_cache& value)
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
}
