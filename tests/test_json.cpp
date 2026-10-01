#include "json_parser.hpp"
#include <cassert>
#include <iostream>

int main() {
    auto v = JsonParser::parse(R"({"a": 1, "b": "x", "c": [1,2], "d": {"e": true}})");
    assert(v.is_object());
    assert(v.find("a") && v.find("a")->to_string() == "1");
    assert(v.find("b")->as_string() == "x");
    assert(v.find("d.e") && v.find("d.e")->to_string() == "true");
    assert(v.find("missing") == nullptr);

    std::cout << "json tests OK\n";
    return 0;
}
