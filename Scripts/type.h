#pragma once
#include <unordered_map>
#include <string>
#include "lexer.h"
#include "parser.h"

struct ArrayInfo {
    TokenType elementType;
    bool isImmutable;
};


class SymbolTable  {
    private:
        std::unordered_map<std::string, TokenType> table;
    public:
        std::unordered_map<std::string, ArrayInfo> arrays;
        
        void declare(const std::string& name, TokenType& type);

        void arrayDeclare(const std::string& name, TokenType& elementType, bool isImmutable);

        TokenType lookup(const std::string& name) const;

        bool exists(const std::string& name) const;

        bool isDeclared(const std::string& name) const;

        bool arrayExists(const std::string& name) const;

        void remove(const std::string& name);
};

struct FuncType {
    TokenType type;
    std::vector<TokenType> paramTypes;
};


inline std::unordered_map<std::string, FuncType> functions;

class TypeChecker {
    private:

        TokenType returnType = TokenType::Sentinel;
        
        std::string lastStructName;

        bool nameTaken(const std::string& name);
    public:
        SymbolTable symbols;
        
        std::unordered_map<std::string, std::vector<Param>> structTable;

        std::unordered_map<std::string, std::string> instances;
        
        TypeChecker() = default;
        TokenType TypeCheck(ASTNode* node);
};
