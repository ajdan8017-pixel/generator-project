#pragma once
#include <string>
#include <vector>

class CsvParser {
public:
    static std::vector<std::vector<std::string>> parse(const std::string& text);
    static std::vector<std::vector<std::string>> parse_file(const std::string& path);
};
