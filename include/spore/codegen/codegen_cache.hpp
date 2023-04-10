#pragma once

#include <mutex>
#include <string>
#include <string_view>
#include <vector>

#include "nlohmann/json.hpp"
#include "spdlog/spdlog.h"

#include "spore/codegen/codegen_helpers.hpp"

namespace spore::codegen
{
    struct codegen_cache_entry
    {
        std::string file;
        std::string hash;

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
        std::vector<codegen_cache_entry> entries;

        bool check_and_update(const std::string_view& file)
        {
            std::string hash;
            if (!spore::codegen::hash_file(file, hash))
            {
                return false;
            }

            std::lock_guard lock(mutex);

            const auto it = std::find(entries.begin(), entries.end(), file.data());
            if (it == entries.end())
            {
                codegen_cache_entry& entry = entries.emplace_back();
                entry.file = file.data();
                entry.hash = std::move(hash);
                return false;
            }

            if (hash != it->hash)
            {
                codegen_cache_entry& entry = *it;
                entry.hash = std::move(hash);
                return false;
            }

            return true;
        }
    };

    void to_json(nlohmann::json& json, const codegen_cache_entry& value)
    {
        json["file"] = value.file;
        json["hash"] = value.hash;
    }

    void from_json(const nlohmann::json& json, codegen_cache_entry& value)
    {
        json["file"].get_to(value.file);
        json["hash"].get_to(value.hash);
    }

    void to_json(nlohmann::json& json, const codegen_cache& value)
    {
        json = value.entries;
    }

    void from_json(const nlohmann::json& json, codegen_cache& value)
    {
        json.get_to(value.entries);
    }
}