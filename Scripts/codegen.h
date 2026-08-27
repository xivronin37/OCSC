#pragma once

#include <iostream>
#include <string>
#include "ast.h"
#include "type.h"

class CodeGen {
    private:
        TypeChecker& typeCheck;
        std::string output;
        std::unordered_map<std::string, int> symbolTable;
        int currentOffset = 0;
        int funcOffset = 0;
        int ifCounter = 0;
        int whileCounter = 0;
        int uniqueCount = 0;

        void emit(const std::string& line, bool indent = true);
        void genNode(ASTNode* node);
    public:
        std::string generate(ASTNode* root);
        CodeGen(TypeChecker& checker) : typeCheck(checker) {}

};

int countVarDecl(ASTNode* node);