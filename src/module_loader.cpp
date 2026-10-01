#include "module_loader.hpp"
#include <dirent.h>
#include <dlfcn.h>
#include <sys/stat.h>
#include <cstring>
#include <cstdlib>
#include <iostream>

std::vector<std::string> ModuleLoader::scan(const std::string& dir) {
    std::vector<std::string> out;
    DIR* d = opendir(dir.c_str());
    if (!d) return out;
    while (auto* e = readdir(d)) {
        std::string name = e->d_name;
        if (name.size() > 3 && name.substr(name.size() - 3) == ".so") {
            out.push_back(dir + "/" + name);
        }
    }
    closedir(d);
    return out;
}

void ModuleLoader::load_one(const std::string& path) {
    struct stat st{};
    if (stat(path.c_str(), &st) != 0) return;

    void* h = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (!h) {
        std::cerr << "[module] cannot load " << path << ": " << dlerror() << "\n";
        return;
    }

    auto reg = reinterpret_cast<register_module_t>(dlsym(h, "register_module"));
    if (!reg) {
        std::cerr << "[module] " << path << " has no register_module: "
                  << dlerror() << "\n";
        dlclose(h);
        return;
    }

    const ModuleDesc* desc = reg();
    if (!desc || !desc->module_name) {
        std::cerr << "[module] " << path << " returned invalid descriptor\n";
        dlclose(h);
        return;
    }

    for (int i = 0; i < desc->func_count; ++i) {
        const FuncEntry& fe = desc->funcs[i];
        std::string fname = fe.name;
        auto c_fn = fe.call;
        g_function_registry[fname] =
            [c_fn](const std::vector<std::string>& args) -> std::string {
                std::vector<const char*> argv;
                argv.reserve(args.size());
                for (auto& a : args) argv.push_back(a.c_str());
                char* res = c_fn(argv.data(), (int)argv.size());
                std::string out = res ? res : "";
                std::free(res);
                return out;
            };
    }

    LoadedModule m;
    m.path = path;
    m.handle = h;
    m.mtime = st.st_mtime;
    m.name = desc->module_name;
    modules_.push_back(std::move(m));
    std::cerr << "[module] loaded " << desc->module_name << " (" << path << ")\n";
}

void ModuleLoader::unload_one(LoadedModule& m) {
    if (m.handle) {
        dlclose(m.handle);
        m.handle = nullptr;
    }
}

void ModuleLoader::load_directory(const std::string& dir) {
    dir_ = dir;
    for (auto& p : scan(dir)) load_one(p);
}

std::vector<std::string> ModuleLoader::refresh() {
    std::vector<std::string> log;
    if (dir_.empty()) return log;

    bool changed = false;
    auto files = scan(dir_);
    for (auto& p : files) {
        struct stat st{};
        if (stat(p.c_str(), &st) != 0) { changed = true; continue; }
        bool found = false;
        for (auto& m : modules_) {
            if (m.path == p) {
                found = true;
                if (m.mtime != st.st_mtime) { changed = true; m.mtime = st.st_mtime; }
            }
        }
        if (!found) changed = true;
    }
    if (!changed) return log;

    log.push_back("[hot-reload] modules changed, reloading");
    for (auto& m : modules_) unload_one(m);
    modules_.clear();

    g_function_registry.clear();
    register_builtin_functions();

    for (auto& p : files) load_one(p);
    return log;
}
