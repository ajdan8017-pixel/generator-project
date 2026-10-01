#pragma once
#include <string>
#include <vector>

struct Node {
    enum Kind { Literal, Variable, Call } kind;
    std::string text;
    std::string name;
    std::vector<std::string> args;
};

class TemplateParser {
public:
    static std::vector<Node> parse(const std::string& src);
};
