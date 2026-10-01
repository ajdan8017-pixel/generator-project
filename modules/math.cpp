#include "../src/function_registry.hpp"
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <string>

static char* dup_str(const std::string& s) {
    char* p = (char*)std::malloc(s.size() + 1);
    std::memcpy(p, s.c_str(), s.size() + 1);
    return p;
}

static std::string num_to_str(double d) {
    char buf[64];
    if (d == (long long)d) std::snprintf(buf, sizeof(buf), "%lld", (long long)d);
    else std::snprintf(buf, sizeof(buf), "%g", d);
    return buf;
}

static char* fn_sum(const char** argv, int argc) {
    if (argc < 2) return dup_str("sum: need 2 args");
    double a = std::atof(argv[0]);
    double b = std::atof(argv[1]);
    return dup_str(num_to_str(a + b));
}

static char* fn_mul(const char** argv, int argc) {
    if (argc < 2) return dup_str("mul: need 2 args");
    return dup_str(num_to_str(std::atof(argv[0]) * std::atof(argv[1])));
}

static char* fn_avg(const char** argv, int argc) {
    if (argc < 1) return dup_str("0");
    double s = 0;
    for (int i = 0; i < argc; ++i) s += std::atof(argv[i]);
    return dup_str(num_to_str(s / argc));
}

static const FuncEntry funcs[] = {
    {"sum", fn_sum},
    {"mul", fn_mul},
    {"avg", fn_avg},
};

static const ModuleDesc desc = {
    "math",
    funcs,
    sizeof(funcs)/sizeof(funcs[0])
};

extern "C" const ModuleDesc* register_module() {
    return &desc;
}
