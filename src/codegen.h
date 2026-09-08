#pragma once

#include <iostream>
#include <string>
#include "ast.h"
#include "type.h"
#include "otools.h"

class CodeGen {
    private:
        TypeChecker& typeCheck;
        std::string output;
        std::unordered_map<std::string, int> symbolTable;
        std::unordered_map<std::string, int> mapCapacities;
        int currentOffset = 0;
        int funcOffset = 0;
        int ifCounter = 0;
        int whileCounter = 0;
        int uniqueCount = 0;

        void emit(const std::string& line, bool indent = true);
        void genNode(ASTNode* node);
        void genMethod(FuncDeclNode* method, const std::string& structName);
    public:
        std::string generate(ASTNode* root);
        CodeGen(TypeChecker& checker) : typeCheck(checker) {}

};

struct SentinelType {
    TokenType type;

    int64_t intValue = INT64_MIN;
    std::string stringValue = "O_SENTINEL//1B7D2ED9B35F4F68//";
    char charValue = '\0';
};

int countVarDecl(ASTNode* node);