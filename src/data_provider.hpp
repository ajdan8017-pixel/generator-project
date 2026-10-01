#pragma once
#include "json_parser.hpp"
#include <optional>
#include <unordered_map>

class DataProvider {
public:
    void load_json(const std::string& path);
    void add_kv(const std::string& key, const std::string& value);
    std::optional<std::string> get(const std::string& key) const;

private:
    JsonValue root_;
    bool has_json_ = false;
    std::unordered_map<std::string, std::string> kv_;
};
