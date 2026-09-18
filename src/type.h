#pragma once
#include <unordered_map>
#include <string>
#include "lexer.h"
#include "parser.h"
#include "otools.h"

struct ArrayInfo {
    TokenType elementType;
    bool isImmutable;
};

struct MapInfo {
    TokenType keyType;
    TokenType valueType;

    bool isImmutable;
};

struct StructInfo {
    std::vector<Param> fields;
    std::unordered_map<std::string, ASTNode*> methods;
};


class SymbolTable  {
    public:
        std::unordered_map<std::string, TokenType> table;
        std::unordered_map<std::string, ArrayInfo> arrays;

        std::unordered_map<std::string, MapInfo> maps;
        
        void declare(const std::string& name, TokenType& type);
        void arrayDeclare(const std::string& name, TokenType& elementType, bool isImmutable);
        TokenType lookup(const std::string& name) const;
        bool exists(const std::string& name) const;
        bool isDeclared(const std::string& name) const;
        bool arrayExists(const std::string& name) const;
        void remove(const std::string& name);
        void arrayRemove(const std::string& name);
        void mapRemove(const std::string& name);
        void mapDeclare(const std::string& name, TokenType& keyType,  TokenType& valueType, bool isImmutable);
        bool mapExists(const std::string& name) const;
        TokenType mapKeyType(const std::string& name) const;
        TokenType mapValueType(const std::string& name) const;
};

struct FuncType {
    TokenType type;
    std::vector<TokenType> paramTypes;
    std::string returnStructName;
    std::vector<bool> isReference;
};


inline std::unordered_map<std::string, FuncType> functions;

class TypeChecker {
    private:
        TokenType returnType = TokenType::Sentinel;
        std::string returnStructName = "";
        std::string lastStructName;

        bool nameTaken(const std::string& name);
    public:
        SymbolTable symbols;
        
        std::unordered_map<std::string, StructInfo> structTable;

        std::unordered_map<std::string, std::string> instances;

        void registerMethodSignature(FuncDeclNode* funcDecl, const std::string& structName);

        void checkMethodBody(FuncDeclNode* funcDecl, const std::string& structName);
        
        TypeChecker() = default;
        TokenType TypeCheck(ASTNode* node);
};
