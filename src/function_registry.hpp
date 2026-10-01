#pragma once
#include "function_registry_internal.hpp"

extern std::unordered_map<std::string, ModuleFunc> g_function_registry;

void register_builtin_functions();

// ABI модулей (C-linkage)
extern "C" {
    struct FuncEntry {
        const char* name;
        char* (*call)(const char** argv, int argc);
    };

    struct ModuleDesc {
        const char* module_name;
        const FuncEntry* funcs;
        int func_count;
    };

    typedef const ModuleDesc* (*register_module_t)();
}
