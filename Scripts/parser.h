#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <filesystem>
#include "lexer.h"

struct typeBlockInfo {
    TokenType kind;
    std::vector<int> params;
    Token elementType;
};

class Parser {
    public:
        Parser(const std::vector<Token>& tokens, std::filesystem::path currentDir);
        ASTNode* parse();
    private:
        std::filesystem::path currentDir;
        std::vector<Token> tokens;
        size_t pos = 0;

        Token peek() const;
        Token advance();
        Token expect(TokenType type);
        Token expectType();
        bool match(TokenType type);
        ASTNode* statement();
        ASTNode* varDecl();
        ASTNode* ifStatement();
        ASTNode* whileStatement();
        ASTNode* funcDecl();
        ASTNode* classDecl();
        ASTNode* enumDecl();
        ASTNode* expression();
        ASTNode* comparison();
        ASTNode* term();
        ASTNode* factor();
        ASTNode* primary();
        ASTNode* block();
        typeBlockInfo typeBlock();
        ASTNode* exprstatement();
        ASTNode* assignStatement(ASTNode* target);
        ASTNode* outStatement();
        ASTNode* pushStatement();
        ASTNode* removeStatement();
        ASTNode* printStatement();

};

std::string tokenTypeName(TokenType type);

void printAST(ASTNode* node, int depth = 0);