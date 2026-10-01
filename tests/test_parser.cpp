#include "template_parser.hpp"
#include <cassert>
#include <iostream>

int main() {
    auto n = TemplateParser::parse("Hi, {{name}}!");
    assert(n.size() == 3);
    assert(n[0].kind == Node::Literal && n[0].text == "Hi, ");
    assert(n[1].kind == Node::Variable && n[1].name == "name");
    assert(n[2].kind == Node::Literal && n[2].text == "!");

    auto m = TemplateParser::parse("{{sum(10, 20)}}");
    assert(m.size() == 1 && m[0].kind == Node::Call && m[0].args.size() == 2);

    auto s = TemplateParser::parse("{{f('a,b', 2)}}");
    assert(s[0].args.size() == 2 && s[0].args[0] == "a,b");

    bool threw = false;
    try { TemplateParser::parse("{{name"); } catch (...) { threw = true; }
    assert(threw);

    std::cout << "parser tests OK\n";
    return 0;
}
