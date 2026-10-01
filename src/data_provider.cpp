#include "data_provider.hpp"

void DataProvider::load_json(const std::string& path) {
    root_ = JsonParser::parse_file(path);
    has_json_ = true;
}

void DataProvider::add_kv(const std::string& key, const std::string& value) {
    kv_[key] = value;
}

std::optional<std::string> DataProvider::get(const std::string& key) const {
    auto it = kv_.find(key);
    if (it != kv_.end()) return it->second;
    if (has_json_) {
        if (const JsonValue* v = root_.find(key)) return v->to_string();
    }
    return std::nullopt;
}
