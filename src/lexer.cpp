#include <vector>
#include <string>
#include <format>
#include "lexer.h"

char Lexer::peek() const {
    if (pos < source.size()) {
        return source[pos];
    }
    return '\0';
}

char Lexer::advance() {
    char c = source[pos];
    column++;
    pos++;
    return c;
}

void Lexer::skipWhitespace() {
    while (pos < source.size() && std::isspace(source[pos])) {
        if (source[pos] == '\n') {
            line++;
            column = 1;
        } else {
            column++;
        }
        pos++;
    }
}

char Lexer::resolveEscape() {
    char code = advance(); // the character right after $$

    switch (code) {
        case 'n': return '\n';
        case 't': return '\t';
        case '0': return '\0';
        default:
            throw std::runtime_error(std::format("L: E70 | Unknown escape sequence $${} at line {}, column {}", code, line, column));
    }
}

Token Lexer::nextToken() {
    if (pos >= source.size()) {
        return {TokenType::EndOfFile, "EndOfFile", line, column};
    }

    char c = peek();
    if (std::isalpha(c) || c == '_') {
        std::string identifier;
        while (std::isalnum(peek()) || peek() == '_') {
            identifier += advance();
        }

        auto keyword = keywords.find(identifier);

        if (keyword != keywords.end()) {
            return {keyword->second, identifier, line, column};
        }

        auto type = types.find(identifier);

        if (type != types.end()) {
            return {type->second, identifier, line, column};
        }

        return {TokenType::Identifier, identifier, line, column};
    }

    if (std::isdigit(c)) {
        std::string number;
        bool isFloat = false;
        while (std::isdigit(peek()) || (peek() == '.' && !isFloat)) {
            if (peek() == '.') isFloat = true;
            number += advance();
        }

        if (isFloat) return {TokenType::Float, number, line, column};

        return {TokenType::Int, number, line, column};
    }

    if (c == '"') {
        std::string StringLiteral = "";

        advance(); // eat opening quote

        while (peek() != '"' && peek() != '\0') {
              
            if (peek() == '$' && pos + 1 < source.size() && source[pos + 1] == '$') {
                advance(); // eat first $
                advance(); // eat second $
                StringLiteral += resolveEscape();
            } else {
                StringLiteral += advance();
            }

            if (peek() == '\0') {
                throw std::runtime_error(std::format("L: E69 | Unterminated string literal at line {}, column {}", line, column));
            }
        }

        advance(); // eat closing quote

        return {TokenType::String, StringLiteral, line, column};
    }

     if (c == '\'') {
        std::string CharLiteral = "";

        advance(); // eat opening quote
        
        if (peek() == '$' && pos + 1 < source.size() && source[pos + 1] == '$') {
            advance(); // eat first $
            advance(); // eat second $
            CharLiteral += resolveEscape();
        }
        else if (peek() != '\'') {
            CharLiteral += advance();
        }

        advance(); // eat closing quote

        return {TokenType::Character, CharLiteral, line, column};
    }

    char op = advance();
    switch (op) {
        default: return {TokenType::Unknown, std::string(1, op), line, column};
        case('+'): return {TokenType::Plus, "+", line, column};
        case('-'): return {TokenType::Minus, "-", line, column};
        case('*'): return {TokenType::Star, "*", line, column};
        case('/'): return {TokenType::FSlash, "/", line, column};
        case('\\'): return {TokenType::BSlash, "\\", line, column};
        case(':'): return {TokenType::Colon, ":", line, column};
        case(';'): return {TokenType::Semicolon, ";", line, column};
        case(','): return {TokenType::Comma, ",", line, column};
        case('['): return {TokenType::LBracket, "[", line, column};
        case(']'): return {TokenType::RBracket, "]", line, column};
        case('('): return {TokenType::LParen, "(", line, column};
        case(')'): return {TokenType::RParen, ")", line, column};
        case('{'): return {TokenType::LBrace, "{", line, column};
        case('}'): return {TokenType::RBrace, "}", line, column};
        case('~'): return {TokenType::Tilde, "~", line, column};
        case('#'): return {TokenType::Hash, "#", line, column};
        case('?'): return {TokenType::Question, "?", line, column};
        case('.'): return {TokenType::Dot, ".", line, column};
        case('`'): return {TokenType::Backtick, "`", line, column};
        case('@'): return {TokenType::At, "@", line, column};
        case('='): {
            if (peek() == '=') {
                advance();
                return {TokenType::EqualEqual, "==", line, column};
            }
            return {TokenType::Equal, "=", line, column};
        }
        case('!'): {
            if (peek() == '=') {
                advance();
                return {TokenType::NotEqual, "!=", line, column};
            }
            return {TokenType::L_NOT, "!", line, column};
        }
        case('&'): {
            if (peek() == '&') {
                advance();
                return {TokenType::L_AND, "&&", line, column};
            }
            return {TokenType::B_AND, "&", line, column};
        }
        case('|'): {
            if (peek() == '|') {
                advance();
                return {TokenType::L_OR, "||", line, column};
            }
            return {TokenType::B_OR, "|", line, column};
        }
        case('^'): {
            if (peek() == '^') {
                advance();
                return {TokenType::L_XOR, "^^", line, column};
            }
            return {TokenType::B_XOR, "^", line, column};
        }
        case('<'): {
            if (peek() == '=') {
                advance();
                return {TokenType::LessThanOrEqual, "<=", line, column};
            }
            return {TokenType::LessThan, "<", line, column};
        }
        case('>'): {
            if (peek() == '=') {
                advance();
                return {TokenType::GreaterThanOrEqual, ">=", line, column};
            }
            return {TokenType::GreaterThan, ">", line, column};
        }
    }

}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    while (pos < source.size()) {
        skipWhitespace();
        if (pos >= source.size()) {
            break;
        }
        tokens.push_back(nextToken());
    }
    return tokens;
}

Lexer::Lexer(const std::string& source) : source(source), pos(0), line(1), column(1) {
    
}

std::string readFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("L: E71 | Could not open file: " + filename);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}