#include "template_parser.hpp"
#include <cctype>
#include <stdexcept>

namespace {

void skip_ws(const std::string& s, size_t& i) {
    while (i < s.size() && std::isspace((unsigned char)s[i])) ++i;
}

std::string read_ident(const std::string& s, size_t& i) {
    size_t start = i;
    while (i < s.size() &&
           (std::isalnum((unsigned char)s[i]) || s[i] == '_')) ++i;
    if (i == start) throw std::runtime_error("expected identifier");
    return s.substr(start, i - start);
}

std::string read_string_literal(const std::string& s, size_t& i) {
    char q = s[i++];
    std::string out;
    while (i < s.size() && s[i] != q) {
        if (s[i] == '\\' && i + 1 < s.size()) {
            ++i;
            switch (s[i]) {
                case 'n': out.push_back('\n'); break;
                case 't': out.push_back('\t'); break;
                case '\\': out.push_back('\\'); break;
                case '\'': out.push_back('\''); break;
                case '"': out.push_back('"'); break;
                default: out.push_back(s[i]);
            }
            ++i;
        } else {
            out.push_back(s[i++]);
        }
    }
    if (i >= s.size()) throw std::runtime_error("unterminated string literal");
    ++i;
    return out;
}

std::string read_arg(const std::string& s, size_t& i) {
    skip_ws(s, i);
    if (i >= s.size()) throw std::runtime_error("expected argument");
    if (s[i] == '\'' || s[i] == '"') return read_string_literal(s, i);
    size_t start = i;
    while (i < s.size() && s[i] != ',' && s[i] != ')') ++i;
    size_t end = i;
    while (end > start && std::isspace((unsigned char)s[end-1])) --end;
    if (end == start) throw std::runtime_error("empty argument");
    return s.substr(start, end - start);
}

} // namespace

std::vector<Node> TemplateParser::parse(const std::string& src) {
    std::vector<Node> nodes;
    std::string literal;
    size_t i = 0;

    auto flush_literal = [&]() {
        if (!literal.empty()) {
            Node n;
            n.kind = Node::Literal;
            n.text = literal;
            nodes.push_back(std::move(n));
            literal.clear();
        }
    };

    while (i < src.size()) {
        if (src[i] == '{' && i + 1 < src.size() && src[i+1] == '{') {
            flush_literal();
            i += 2;
            skip_ws(src, i);
            std::string name = read_ident(src, i);
            skip_ws(src, i);

            Node n;
            n.name = name;

            if (i < src.size() && src[i] == '(') {
                n.kind = Node::Call;
                ++i;
                skip_ws(src, i);
                if (i < src.size() && src[i] == ')') {
                    ++i;
                } else {
                    while (true) {
                        n.args.push_back(read_arg(src, i));
                        skip_ws(src, i);
                        if (i < src.size() && src[i] == ',') { ++i; continue; }
                        if (i < src.size() && src[i] == ')') { ++i; break; }
                        throw std::runtime_error("expected ',' or ')' in call");
                    }
                }
            } else {
                n.kind = Node::Variable;
            }

            skip_ws(src, i);
            if (i + 1 >= src.size() || src[i] != '}' || src[i+1] != '}')
                throw std::runtime_error("expected '}}'");
            i += 2;
            nodes.push_back(std::move(n));
        } else {
            literal.push_back(src[i++]);
        }
    }
    flush_literal();
    return nodes;
}
