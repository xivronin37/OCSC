#pragma once

#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include "lexer.h"

struct ASTNode {
    virtual ~ASTNode() = default;
};

struct Param {
    Token type;
    Token name;
    bool isReference;
};

struct IdentifierNode : ASTNode {
    std::string value;
    IdentifierNode(const std::string& value) : value(value) {}
};

struct CollectionNode : ASTNode {
    std::string value;
    std::vector<ASTNode*> elements;
    CollectionNode(const std::string& value, const std::vector<ASTNode*>& elements) : value(value), elements(elements) {}
};

struct NumberLiteralNode : ASTNode {
    std::string value;
    NumberLiteralNode(const std::string& value) : value(value) {}
};

struct BlockNode : ASTNode {
    std::vector<ASTNode*> statements;
    BlockNode(const std::vector<ASTNode*>& statements) : statements(statements) {}
};

struct IfNode : ASTNode {
    ASTNode* condition;
    ASTNode* thenBranch;
    ASTNode* elseBranch;

    IfNode(ASTNode* condition, ASTNode*& thenBranch, ASTNode*& elseBranch)
        : condition(condition), thenBranch(thenBranch), elseBranch(elseBranch) {}
};

struct WhileNode : ASTNode {
    ASTNode* condition;
    ASTNode* body;

    WhileNode(ASTNode* condition, ASTNode* body)
        : condition(condition), body(body) {}
};

struct OutNode : ASTNode {
    ASTNode* output;

    OutNode(ASTNode* output) : output(output) {}
};

struct BinaryExprNode : ASTNode {
    ASTNode* left;
    Token op;
    ASTNode* right;

    BinaryExprNode(ASTNode* left, Token op, ASTNode* right) : left(left), op(op), right(right) {}
};

struct VarDeclNode : ASTNode {
    Token name;
    Token type;
    ASTNode* value;

    VarDeclNode(Token name, Token type, ASTNode* value) : name(name), type(type), value(value) {}
};

struct ArrayDeclNode : VarDeclNode {
    Token elementType;
    ArrayDeclNode* nestedType;
    int size;
    std::vector<ASTNode*> elements;
    bool isImmutable;

    ArrayDeclNode(Token name, Token elementType, int size, std::vector<ASTNode*> elements, bool isImmutable) : VarDeclNode(name, elementType, nullptr),
    elementType(elementType), size(size), elements(elements), isImmutable(isImmutable) {}
};

struct StructDeclNode : ASTNode {
    Token name;
    std::unordered_map<std::string, ASTNode*> methods;
    std::vector<Param> fields;

    StructDeclNode(Token name, std::vector<Param> fields, std::unordered_map<std::string, ASTNode*> methods) : name(name), fields(fields), methods(methods) {}
};

struct MapDeclNode : ASTNode {
    ArrayDeclNode* keys;
    ArrayDeclNode* values;
    Token name;
    Token keyType;
    Token valueType;
    ASTNode* size;

    MapDeclNode(Token name, Token keyType, Token valueType, ASTNode* size, ArrayDeclNode* keys, ArrayDeclNode* values)
    : name(name), keyType(keyType), valueType(valueType), size(size), keys(keys), values(values) {}
};

struct InstanceNode : ASTNode {
    Token structName;
    std::vector<ASTNode*> arguments;

    InstanceNode(Token structName, std::vector<ASTNode*> arguments) : structName(structName), arguments(arguments) {}
};

struct FieldAccessNode : ASTNode {
    ASTNode* target;
    Token field;

    FieldAccessNode(ASTNode* target, Token field) : target(target), field(field){}

};

struct MethodNode : ASTNode {
    Token methodName;
    Token targetName;
    std::vector<ASTNode*> arguments;

    MethodNode(Token methodName, Token targetName, std::vector<ASTNode*> arguments)
        : methodName(methodName), targetName(targetName), arguments(arguments) {}
};

struct IndexNode : ASTNode {
    Token name;
    ASTNode* index;

    IndexNode(Token name, ASTNode* index) : name(name), index(index) {}
};

struct AssignNode : ASTNode {
    ASTNode* target;
    Token op;
    ASTNode* value;

    AssignNode(ASTNode* target, Token op, ASTNode* value) : target(target), op(op), value(value) {}
};

struct PushNode : ASTNode {
    Token arrayName;
    ASTNode* value;
    ASTNode* secondValue;
    PushNode(Token arrayName, ASTNode* value, ASTNode* secondValue) : arrayName(arrayName), value(value), secondValue(secondValue) {}
};

struct RemoveNode : ASTNode {
    Token arrayName;
    ASTNode* index;
    RemoveNode(Token arrayName, ASTNode* index) : arrayName(arrayName), index(index) {}
};

struct PrintNode : ASTNode {
    ASTNode* value;

    PrintNode(ASTNode* value) : value(value) {}
};

struct FuncDeclNode : ASTNode {
    Token returnType;
    Token name;
    std::vector<Param> parameters;
    ASTNode* body;

    FuncDeclNode(Token returnType, Token name, std::vector<Param> parameters, ASTNode* body) : returnType(returnType), name(name), parameters(parameters), body(body) {}

};

struct CallNode : ASTNode {
    Token name;
    std::vector<ASTNode*> arguments;
    
    CallNode(Token name, std::vector<ASTNode*> arguments) : name(name), arguments(arguments) {}
};

struct SizeNode : ASTNode {
    Token name;
    SizeNode(Token name) : name(name) {}
};