#include "csv_parser.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>

std::vector<std::vector<std::string>> CsvParser::parse(const std::string& text) {
    std::vector<std::vector<std::string>> rows;
    std::vector<std::string> row;
    std::string field;
    bool in_quotes = false;

    for (size_t i = 0; i < text.size(); ++i) {
        char c = text[i];
        if (in_quotes) {
            if (c == '"') {
                if (i + 1 < text.size() && text[i+1] == '"') { field.push_back('"'); ++i; }
                else in_quotes = false;
            } else field.push_back(c);
        } else {
            if (c == '"') in_quotes = true;
            else if (c == ',') { row.push_back(field); field.clear(); }
            else if (c == '\n') {
                row.push_back(field); field.clear();
                rows.push_back(row); row.clear();
            } else if (c == '\r') { }
            else field.push_back(c);
        }
    }
    if (!field.empty() || !row.empty()) { row.push_back(field); rows.push_back(row); }
    return rows;
}

std::vector<std::vector<std::string>> CsvParser::parse_file(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("cannot open CSV: " + path);
    std::ostringstream ss; ss << in.rdbuf();
    return parse(ss.str());
}
