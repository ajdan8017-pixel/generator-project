#pragma once
#include "function_registry.hpp"
#include <string>
#include <vector>
#include <ctime>

struct LoadedModule {
    std::string path;
    void* handle = nullptr;
    std::time_t mtime = 0;
    std::string name;
};

class ModuleLoader {
public:
    void load_directory(const std::string& dir);
    std::vector<std::string> refresh();

private:
    void load_one(const std::string& path);
    void unload_one(LoadedModule& m);
    std::vector<std::string> scan(const std::string& dir);

    std::string dir_;
    std::vector<LoadedModule> modules_;
};
