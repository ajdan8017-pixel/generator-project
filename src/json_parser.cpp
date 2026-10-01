#include "json_parser.hpp"
#include <fstream>
#include <sstream>
#include <cctype>
#include <cstdio>

std::string JsonValue::to_string() const {
    if (std::holds_alternative<std::nullptr_t>(v_)) return "";
    if (auto p = std::get_if<bool>(&v_)) return *p ? "true" : "false";
    if (auto p = std::get_if<double>(&v_)) {
        char buf[64];
        double d = *p;
        if (d == (long long)d) std::snprintf(buf, sizeof(buf), "%lld", (long long)d);
        else std::snprintf(buf, sizeof(buf), "%g", d);
        return buf;
    }
    if (auto p = std::get_if<std::string>(&v_)) return *p;
    if (auto p = std::get_if<Array>(&v_)) {
        std::string out;
        for (size_t i = 0; i < p->size(); ++i) {
            if (i) out += ", ";
            out += (*p)[i].to_string();
        }
        return out;
    }
    if (auto p = std::get_if<Object>(&v_)) {
        std::string out;
        bool first = true;
        for (auto& kv : *p) {
            if (!first) out += ", ";
            first = false;
            out += kv.first + "=" + kv.second.to_string();
        }
        return out;
    }
    return "";
}

const JsonValue* JsonValue::find(const std::string& path) const {
    const JsonValue* cur = this;
    size_t start = 0;
    while (start <= path.size()) {
        size_t dot = path.find('.', start);
        std::string key = path.substr(start, dot == std::string::npos ? std::string::npos : dot - start);
        if (!cur->is_object()) return nullptr;
        auto it = cur->as_object().find(key);
        if (it == cur->as_object().end()) return nullptr;
        cur = &it->second;
        if (dot == std::string::npos) break;
        start = dot + 1;
    }
    return cur;
}

namespace {

struct Parser {
    const std::string& s;
    size_t i = 0;

    [[noreturn]] void err(const std::string& msg) {
        throw std::runtime_error("JSON parse error at " + std::to_string(i) + ": " + msg);
    }

    void skip_ws() {
        while (i < s.size()) {
            char c = s[i];
            if (c==' '||c=='\t'||c=='\n'||c=='\r') { ++i; continue; }
            if (c=='/' && i+1 < s.size() && s[i+1]=='/') {
                while (i < s.size() && s[i] != '\n') ++i;
                continue;
            }
            break;
        }
    }

    JsonValue parse_value() {
        skip_ws();
        if (i >= s.size()) err("unexpected end");
        char c = s[i];
        if (c == '{') return parse_object();
        if (c == '[') return parse_array();
        if (c == '"') return JsonValue(parse_string());
        if (c == 't' || c == 'f') return parse_bool();
        if (c == 'n') return parse_null();
        return parse_number();
    }

    std::string parse_string() {
        if (s[i] != '"') err("expected '\"'");
        ++i;
        std::string out;
        while (i < s.size() && s[i] != '"') {
            if (s[i] == '\\' && i+1 < s.size()) {
                ++i;
                switch (s[i]) {
                    case 'n': out.push_back('\n'); break;
                    case 't': out.push_back('\t'); break;
                    case 'r': out.push_back('\r'); break;
                    case 'b': out.push_back('\b'); break;
                    case 'f': out.push_back('\f'); break;
                    case '\\': out.push_back('\\'); break;
                    case '/': out.push_back('/'); break;
                    case '"': out.push_back('"'); break;
                    case 'u': {
                        if (i + 4 >= s.size()) err("bad \\u escape");
                        unsigned code = 0;
                        for (int k = 0; k < 4; ++k) {
                            char h = s[++i];
                            code <<= 4;
                            if (h >= '0' && h <= '9') code |= h - '0';
                            else if (h >= 'a' && h <= 'f') code |= h - 'a' + 10;
                            else if (h >= 'A' && h <= 'F') code |= h - 'A' + 10;
                            else err("bad hex in \\u");
                        }
                        if (code < 0x80) out.push_back((char)code);
                        else out.push_back('?');
                        break;
                    }
                    default: out.push_back(s[i]);
                }
                ++i;
            } else {
                out.push_back(s[i++]);
            }
        }
        if (i >= s.size()) err("unterminated string");
        ++i;
        return out;
    }

    JsonValue parse_number() {
        size_t start = i;
        if (s[i] == '-') ++i;
        while (i < s.size() && std::isdigit((unsigned char)s[i])) ++i;
        if (i < s.size() && s[i] == '.') {
            ++i;
            while (i < s.size() && std::isdigit((unsigned char)s[i])) ++i;
        }
        if (i < s.size() && (s[i] == 'e' || s[i] == 'E')) {
            ++i;
            if (i < s.size() && (s[i]=='+'||s[i]=='-')) ++i;
            while (i < s.size() && std::isdigit((unsigned char)s[i])) ++i;
        }
        if (i == start) err("expected number");
        return JsonValue(std::stod(s.substr(start, i - start)));
    }

    JsonValue parse_bool() {
        if (s.compare(i, 4, "true") == 0)  { i += 4; return JsonValue(true); }
        if (s.compare(i, 5, "false") == 0) { i += 5; return JsonValue(false); }
        err("expected bool");
    }

    JsonValue parse_null() {
        if (s.compare(i, 4, "null") == 0) { i += 4; return JsonValue(nullptr); }
        err("expected null");
    }

    JsonValue parse_array() {
        ++i;
        JsonValue::Array arr;
        skip_ws();
        if (i < s.size() && s[i] == ']') { ++i; return JsonValue(std::move(arr)); }
        while (true) {
            arr.push_back(parse_value());
            skip_ws();
            if (i < s.size() && s[i] == ',') { ++i; continue; }
            if (i < s.size() && s[i] == ']') { ++i; break; }
            err("expected ',' or ']'");
        }
        return JsonValue(std::move(arr));
    }

    JsonValue parse_object() {
        ++i;
        JsonValue::Object obj;
        skip_ws();
        if (i < s.size() && s[i] == '}') { ++i; return JsonValue(std::move(obj)); }
        while (true) {
            skip_ws();
            std::string key = parse_string();
            skip_ws();
            if (i >= s.size() || s[i] != ':') err("expected ':'");
            ++i;
            obj.emplace(std::move(key), parse_value());
            skip_ws();
            if (i < s.size() && s[i] == ',') { ++i; continue; }
            if (i < s.size() && s[i] == '}') { ++i; break; }
            err("expected ',' or '}'");
        }
        return JsonValue(std::move(obj));
    }
};

} // namespace

JsonValue JsonParser::parse(const std::string& text) {
    Parser p{text};
    JsonValue v = p.parse_value();
    p.skip_ws();
    if (p.i != text.size())
        throw std::runtime_error("trailing data after JSON value");
    return v;
}

JsonValue JsonParser::parse_file(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("cannot open JSON file: " + path);
    std::ostringstream ss;
    ss << in.rdbuf();
    return parse(ss.str());
}
