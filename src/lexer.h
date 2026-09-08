#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <stdexcept>

enum class TokenType {
    Identifier,
    Keyword,
    Let, If, Else, While, Out, Call, Create, Open, Class, Insert, From, As, Inline, Request, Send, Decouple, Enum,
    Number,
    String,
    Character,
    Plus, Minus, Star, FSlash, BSlash, Colon, Semicolon, Comma, LBracket, RBracket, LParen, RParen, LBrace, RBrace, Tilde,
    Equal, EqualEqual, NotEqual, LessThan, GreaterThan, LessThanOrEqual, GreaterThanOrEqual, Question, Dot, Backtick,
    L_AND, L_OR, L_NOT, L_XOR, B_AND, B_OR, B_NOT, B_XOR,
    Punctuation,
    Array, Hash, Struct, Push, Remove, Inst,
    Map,
    Int, Bool, Float, Null, UnsignedInt, UnsignedFloat, Str, Char, Sentinel,
    Unknown,
    EndOfFile
};

inline const std::unordered_map<std::string, TokenType> keywords = {
    {"let", TokenType::Let},
    {"if", TokenType::If},
    {"else", TokenType::Else},
    {"while", TokenType::While},
    {"out", TokenType::Out},
    {"call", TokenType::Call},
    {"create", TokenType::Create},
    {"class", TokenType::Class},
    {"inst", TokenType::Inst},
    {"open", TokenType::Open},
    {"insert", TokenType::Insert},
    {"from", TokenType::From},
    {"as", TokenType::As},
    {"map", TokenType::Map},
    {"enum", TokenType::Enum},
    {"inline", TokenType::Inline},
    {"push", TokenType::Push},
    {"remove", TokenType::Remove},
    {"request", TokenType::Request},
    {"send", TokenType::Send},
    {"decouple", TokenType::Decouple}
};

inline const std::unordered_map<std::string, TokenType> types = {
    {"i", TokenType::Int},
    {"b", TokenType::Bool},
    {"f", TokenType::Float},
    {"n", TokenType::Null},
    {"ui", TokenType::UnsignedInt},
    {"uf", TokenType::UnsignedFloat},
    {"arr", TokenType::Array},
    {"str", TokenType::Str},
    {"c", TokenType::Char}
};


struct Token {
    TokenType type;
    std::string value;
    int line = 1;
    int column = 0;
};

class Lexer {
    public:
        Lexer(const std::string& source);
        std::vector<Token> tokenize();
    private:
        std::string source;
        size_t pos = 0;
        int line = 1;
        int column = 0;

        char peek() const;
        char advance();
        void skipWhitespace();
        Token nextToken();
};

std::string readFile(const std::string& filename);