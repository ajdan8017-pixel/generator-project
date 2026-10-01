#include "template_parser.hpp"
#include "json_parser.hpp"
#include "module_loader.hpp"
#include "data_provider.hpp"
#include "function_registry.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <csignal>
#include <thread>
#include <atomic>
#include <chrono>
#include <sys/stat.h>

static std::atomic<bool> g_running{true};
static void on_sigint(int) { g_running = false; }

struct Args {
    std::string templ, data, output, modules_dir = "modules";
    std::vector<std::pair<std::string,std::string>> kv;
    bool watch = false;
    bool verbose = false;
};

static void usage() {
    std::cerr <<
        "Usage: generator --template FILE [--data FILE] [--output FILE]\n"
        "                 [--modules DIR] [--set key=value]...\n"
        "                 [--watch] [--verbose]\n";
}

static bool parse_args(int argc, char** argv, Args& a) {
    for (int i = 1; i < argc; ++i) {
        std::string s = argv[i];
        auto next = [&](std::string& dst) -> bool {
            if (i + 1 >= argc) { std::cerr << "missing value for " << s << "\n"; return false; }
            dst = argv[++i]; return true;
        };
        if (s == "--template") { if (!next(a.templ)) return false; }
        else if (s == "--data") { if (!next(a.data)) return false; }
        else if (s == "--output") { if (!next(a.output)) return false; }
        else if (s == "--modules") { if (!next(a.modules_dir)) return false; }
        else if (s == "--set") {
            std::string kv; if (!next(kv)) return false;
            auto eq = kv.find('=');
            if (eq == std::string::npos) { std::cerr << "--set expects key=value\n"; return false; }
            a.kv.emplace_back(kv.substr(0, eq), kv.substr(eq + 1));
        }
        else if (s == "--watch") a.watch = true;
        else if (s == "--verbose") a.verbose = true;
        else if (s == "-h" || s == "--help") { usage(); return false; }
        else { std::cerr << "unknown arg: " << s << "\n"; usage(); return false; }
    }
    if (a.templ.empty()) { usage(); return false; }
    return true;
}

static std::string read_file(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("cannot open " + path);
    std::ostringstream ss; ss << in.rdbuf();
    return ss.str();
}

static void write_file(const std::string& path, const std::string& data) {
    std::ofstream out(path, std::ios::binary);
    if (!out) throw std::runtime_error("cannot write " + path);
    out << data;
}

static std::string resolve_arg(const std::string& arg, const DataProvider& dp) {
    if (auto v = dp.get(arg)) return *v;
    return arg;
}

static std::string render(const std::vector<Node>& nodes,
                          const DataProvider& dp) {
    std::string out;
    for (const auto& n : nodes) {
        switch (n.kind) {
            case Node::Literal:
                out += n.text;
                break;
            case Node::Variable: {
                if (auto v = dp.get(n.name)) {
                    out += *v;
                } else {
                    auto it = g_function_registry.find(n.name);
                    if (it != g_function_registry.end()) {
                        try { out += it->second({}); }
                        catch (...) { out += "{{" + n.name + "}}"; }
                    } else {
                        out += "{{" + n.name + "}}";
                    }
                }
                break;
            }
            case Node::Call: {
                auto it = g_function_registry.find(n.name);
                if (it == g_function_registry.end()) {
                    out += "{{" + n.name + "(?)}}";
                    break;
                }
                std::vector<std::string> args;
                for (auto& a : n.args) args.push_back(resolve_arg(a, dp));
                try {
                    out += it->second(args);
                } catch (const std::exception& e) {
                    out += std::string("[error: ") + e.what() + "]";
                }
                break;
            }
        }
    }
    return out;
}

int main(int argc, char** argv) {
    std::signal(SIGINT, on_sigint);

    Args a;
    if (!parse_args(argc, argv, a)) return 1;

    register_builtin_functions();

    ModuleLoader loader;
    loader.load_directory(a.modules_dir);

    DataProvider dp;
    if (!a.data.empty()) {
        try { dp.load_json(a.data); }
        catch (const std::exception& e) {
            std::cerr << "JSON error: " << e.what() << "\n"; return 2;
        }
    }
    for (auto& kvp : a.kv) dp.add_kv(kvp.first, kvp.second);

    auto generate = [&]() -> int {
        try {
            std::string tpl = read_file(a.templ);
            auto nodes = TemplateParser::parse(tpl);
            std::string result = render(nodes, dp);
            if (a.output.empty()) std::cout << result;
            else { write_file(a.output, result); if (a.verbose) std::cerr << "written " << a.output << "\n"; }
            return 0;
        } catch (const std::exception& e) {
            std::cerr << "error: " << e.what() << "\n";
            return 3;
        }
    };

    int rc = generate();
    if (rc != 0 || !a.watch) return rc;

    struct stat st_t{}, st_d{};
    stat(a.templ.c_str(), &st_t);
    if (!a.data.empty()) stat(a.data.c_str(), &st_d);
    auto t_t = st_t.st_mtime, t_d = st_d.st_mtime;

    std::cerr << "[watch] waiting for changes (Ctrl+C to exit)\n";
    while (g_running) {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        bool need_regen = false;

        struct stat s{};
        if (stat(a.templ.c_str(), &s) == 0 && s.st_mtime != t_t) { t_t = s.st_mtime; need_regen = true; }
        if (!a.data.empty() && stat(a.data.c_str(), &s) == 0 && s.st_mtime != t_d) { t_d = s.st_mtime; need_regen = true; }

        auto log = loader.refresh();
        for (auto& m : log) std::cerr << m << "\n";
        if (!log.empty()) need_regen = true;

        if (need_regen) {
            std::cerr << "[watch] regenerating...\n";
            generate();
        }
    }
    return 0;
}
