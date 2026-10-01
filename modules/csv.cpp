#include "../src/function_registry.hpp"
#include "../src/csv_parser.hpp"
#include <cstdlib>
#include <cstring>
#include <string>

static char* dup_str(const std::string& s) {
    char* p = (char*)std::malloc(s.size() + 1);
    std::memcpy(p, s.c_str(), s.size() + 1);
    return p;
}

static char* fn_csv_row(const char** argv, int argc) {
    if (argc < 2) return dup_str("csv_row: need file and index");
    try {
        auto rows = CsvParser::parse_file(argv[0]);
        int idx = std::atoi(argv[1]);
        if (idx < 0 || (size_t)idx >= rows.size()) return dup_str("");
        char sep = (argc >= 3 && argv[2][0]) ? argv[2][0] : ',';
        std::string out;
        for (size_t i = 0; i < rows[idx].size(); ++i) {
            if (i) out.push_back(sep);
            out += rows[idx][i];
        }
        return dup_str(out);
    } catch (const std::exception& e) {
        return dup_str(std::string("csv_row error: ") + e.what());
    }
}

static const FuncEntry funcs[] = {
    {"csv_row", fn_csv_row},
};

static const ModuleDesc desc = { "csv", funcs, 1 };

extern "C" const ModuleDesc* register_module() { return &desc; }
