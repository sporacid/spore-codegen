#pragma once

#include <filesystem>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

#include "nlohmann/json.hpp"
#include "spdlog/spdlog.h"

#include "spore/codegen/codegen_helpers.hpp"
#include "spore/codegen/codegen_version.hpp"

namespace spore::codegen
{
    struct codegen_cache_entry
    {
        std::string file;
        std::string hash;
        std::uintmax_t file_size = 0;
        std::int64_t last_write_time = 0;

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
        std::mutex mutex;
        std::string version;
        std::vector<codegen_cache_entry> entries;

        bool check_and_update(const std::string_view& file)
        {
            std::string hash;
            if (!spore::codegen::hash_file(file, hash))
            {
                return false;
            }

            std::uintmax_t file_size = std::filesystem::file_size(file);
            std::int64_t file_last_write_time = std::filesystem::last_write_time(file).time_since_epoch().count();

            std::lock_guard lock(mutex);

            const auto it = std::find(entries.begin(), entries.end(), file.data());
            if (it == entries.end())
            {
                codegen_cache_entry& entry = entries.emplace_back();
                entry.file = file.data();
                entry.hash = std::move(hash);
                entry.file_size = file_size;
                entry.last_write_time = file_last_write_time;
                return false;
            }

            if (file_size != it->file_size ||
                file_last_write_time != it->last_write_time ||
                hash != it->hash)
            {
                codegen_cache_entry& entry = *it;
                entry.hash = std::move(hash);
                entry.file_size = file_size;
                entry.last_write_time = file_last_write_time;
                return false;
            }

            return true;
        }
    };

    void to_json(nlohmann::json& json, const codegen_cache_entry& value)
    {
        json["file"] = value.file;
        json["hash"] = value.hash;
        json["file_size"] = value.file_size;
        json["last_write_time"] = value.last_write_time;
    }

    void from_json(const nlohmann::json& json, codegen_cache_entry& value)
    {
        json["file"].get_to(value.file);
        json["hash"].get_to(value.hash);
        json["file_size"].get_to(value.file_size);
        json["last_write_time"].get_to(value.last_write_time);
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
