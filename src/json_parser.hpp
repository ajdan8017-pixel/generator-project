#pragma once
#include <string>
#include <map>
#include <vector>
#include <variant>
#include <stdexcept>

class JsonValue {
public:
    using Object = std::map<std::string, JsonValue>;
    using Array  = std::vector<JsonValue>;
    using Var    = std::variant<std::nullptr_t, bool, double, std::string, Array, Object>;

    JsonValue() : v_(nullptr) {}
    explicit JsonValue(Var v) : v_(std::move(v)) {}

    bool is_string() const { return std::holds_alternative<std::string>(v_); }
    bool is_object() const { return std::holds_alternative<Object>(v_); }
    bool is_array()  const { return std::holds_alternative<Array>(v_); }
    bool is_number() const { return std::holds_alternative<double>(v_); }

    const std::string& as_string() const { return std::get<std::string>(v_); }
    const Object& as_object() const { return std::get<Object>(v_); }
    const Array&  as_array()  const { return std::get<Array>(v_); }
    double as_number() const { return std::get<double>(v_); }

    std::string to_string() const;
    const JsonValue* find(const std::string& path) const;

private:
    Var v_;
};

class JsonParser {
public:
    static JsonValue parse_file(const std::string& path);
    static JsonValue parse(const std::string& text);
};
