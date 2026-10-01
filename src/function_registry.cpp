#include "function_registry.hpp"
#include <ctime>
#include <algorithm>
#include <cctype>

std::unordered_map<std::string, ModuleFunc> g_function_registry;

static std::string now_date(const std::vector<std::string>&) {
    std::time_t t = std::time(nullptr);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d", std::localtime(&t));
    return buf;
}

static std::string upper(const std::vector<std::string>& a) {
    if (a.empty()) return "";
    std::string s = a[0];
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c){ return std::toupper(c); });
    return s;
}

static std::string lower(const std::vector<std::string>& a) {
    if (a.empty()) return "";
    std::string s = a[0];
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return s;
}

void register_builtin_functions() {
    g_function_registry["date"]  = now_date;
    g_function_registry["upper"] = upper;
    g_function_registry["lower"] = lower;
}
