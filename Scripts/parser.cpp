#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <format>
#include <filesystem>
#include "lexer.h"
#include "ast.h"
#include "parser.h"
#include "otools.h"


std::string tokenTypeName(TokenType type) {
    switch (type) {
        case TokenType::Identifier: return "Identifier";
        case TokenType::Keyword: return "Keyword";
        case TokenType::Let: return "Let";
        case TokenType::If: return "If";
        case TokenType::Else: return "Else";
        case TokenType::While: return "While";
        case TokenType::Out: return "Out";
        case TokenType::Create: return "Create";
        case TokenType::Call: return "Call";
        case TokenType::Open: return "Open";
        case TokenType::Class: return "Class";
        case TokenType::Insert: return "Insert";
        case TokenType::From: return "From";
        case TokenType::As: return "As";
        case TokenType::Map: return "Map";
        case TokenType::Inline: return "Inline";
        case TokenType::Push: return "Push";
        case TokenType::Remove: return "Remove";
        case TokenType::Request: return "Request";
        case TokenType::Send: return "Send";
        case TokenType::Decouple: return "Decouple";
        case TokenType::Enum: return "Enum";
        case TokenType::Number: return "Number";
        case TokenType::String: return "String";
        case TokenType::Str: return "Str";
        case TokenType::Plus: return "Plus";
        case TokenType::Minus: return "Minus";
        case TokenType::Star: return "Star";
        case TokenType::FSlash: return "FSlash";
        case TokenType::BSlash: return "BSlash";
        case TokenType::Colon: return "Colon";
        case TokenType::Semicolon: return "Semicolon";
        case TokenType::Comma: return "Comma";
        case TokenType::LBracket: return "LBracket";
        case TokenType::RBracket: return "RBracket";
        case TokenType::LParen: return "LParen";
        case TokenType::RParen: return "RParen";
        case TokenType::LBrace: return "LBrace";
        case TokenType::RBrace: return "RBrace";
        case TokenType::Tilde: return "Tilde";
        case TokenType::Dot: return "Dot";
        case TokenType::Backtick: return "Backtick";
        case TokenType::Equal: return "Equal";
        case TokenType::EqualEqual: return "EqualEqual";
        case TokenType::NotEqual: return "NotEqual";
        case TokenType::LessThan: return "LessThan";
        case TokenType::GreaterThan: return "GreaterThan";
        case TokenType::LessThanOrEqual: return "LessThanOrEqual";
        case TokenType::GreaterThanOrEqual: return "GreaterThanOrEqual";
        case TokenType::L_AND: return "L_AND";
        case TokenType::L_OR: return "L_OR";
        case TokenType::L_NOT: return "L_NOT";
        case TokenType::L_XOR: return "L_XOR";
        case TokenType::Punctuation: return "Punctuation";
        case TokenType::Int: return "Int";
        case TokenType::Bool: return "Bool";
        case TokenType::Float: return "Float";
        case TokenType::Array: return "Array";
        case TokenType::Hash: return "Hash";
        case TokenType::Question: return "Question";
        case TokenType::Null: return "Null";
        case TokenType::UnsignedInt: return "UnsignedInt";
        case TokenType::UnsignedFloat: return "UnsignedFloat";
        case TokenType::EndOfFile: return "EndOfFile";
    }

    return "Unknown"; 
}

Token Parser::peek() const {
    if (pos < tokens.size()) {
        return tokens[pos];
    }
    return {TokenType::EndOfFile, "", 0, 0};
}

Token Parser::advance() {
    if (pos < tokens.size()) {
        return tokens[pos++];
    }
    return {TokenType::EndOfFile, "", 0, 0};
}

Token Parser::expect(TokenType type) {
    Token token = peek();
    if (token.type == type) {
        advance();
        return token;
    }
    
    // Handle error: unexpected token type
    throw std::runtime_error("P: E31 | Unexpected token type " + tokenTypeName(token.type) + " at line " + std::to_string(token.line) + ", column " + std::to_string(token.column)); 
    return {TokenType::EndOfFile, "", 0, 0};
}

Token Parser::expectType() {
    TokenType t = peek().type;
    if (t == TokenType::Int || t == TokenType::Bool || t == TokenType::Float || t == TokenType::Null
        || t == TokenType::UnsignedInt || t == TokenType::UnsignedFloat || t == TokenType::Identifier
        || t == TokenType::String || t == TokenType::Character) {
        return advance();
    }

    throw std::runtime_error("P: E30 | Expected type token at line " + std::to_string(peek().line) + ", column " + std::to_string(peek().column));
    return {TokenType::EndOfFile, "", 0, 0};
}

bool Parser::match(TokenType type) {
    Token token = peek();
    if (token.type == type) {
        advance();
        return true;
    }
    return false;
}

ASTNode* Parser::statement() {
    if (peek().type == TokenType::Let) {
        return varDecl();
    } 

    if (peek().type == TokenType::If) {
        return ifStatement();
    }

    if (peek().type == TokenType::While) {
        return whileStatement();
    }

    if (peek().type == TokenType::Identifier) {
        size_t savedPos = pos;
        ASTNode* target = primary();
        if (peek().type == TokenType::Equal || peek().type == TokenType::Tilde) {
            return assignStatement(target);
        }

        pos = savedPos;
    }

    if (peek().type == TokenType::Create) {
        return funcDecl();
    }

    if (peek().type == TokenType::Open || peek().type == TokenType::Class) {
        return classDecl();
    }

    if (peek().type == TokenType::Push) return pushStatement();
    if (peek().type == TokenType::Remove) return removeStatement();

    if (peek().type == TokenType::Out) {
        return outStatement();
    }

    if (peek().type == TokenType::Backtick) {
        return printStatement();
    }

    if (peek().type == TokenType::Enum) {
        return enumDecl();
    }

    return exprstatement();
}

ASTNode* Parser::varDecl() {
    expect(TokenType::Let);
    Token name = expect(TokenType::Identifier);
    expect(TokenType::Colon);

    if (peek().type == TokenType::LBrace) {
        typeBlockInfo newInfo = typeBlock();
        expect(TokenType::Comma);
        std::vector<ASTNode*> elements = parseList(newInfo.elementType.type);

        return new ArrayDeclNode{name, newInfo.elementType, newInfo.params[0], elements, false};
    }

    else if (peek().type == TokenType::Str) {
        advance();
        expect(TokenType::Comma);
        Token literal = expect(TokenType::String);
        expect(TokenType::Semicolon);
        std::vector<ASTNode*> elements;

        for (const char& c : literal.value) {
            std::string ascii = std::to_string(static_cast<int>(c));
            elements.push_back(new NumberLiteralNode(ascii));
        }

        Token elementType{TokenType::Int, "i", name.line, name.column};

        return new ArrayDeclNode{name, elementType, (int)elements.size(), elements, true}; // Strings are currently ArrayDecl for temporary optimization
    }
    else if (peek().type == TokenType::Char) {
        advance();
        expect(TokenType::Comma);
        int savedPos = peek().line;
        Token literal = expect(TokenType::Character);
        if (literal.value == "") {
            throw std::runtime_error("P: E42-1 | Cannot parse an empty character at line " + std::to_string(savedPos));
        }

        expect(TokenType::Semicolon);

        std::vector<ASTNode*> elements;
        std::string ascii = std::to_string(static_cast<int>(literal.value[0]));
        elements.push_back(new NumberLiteralNode(ascii));

        Token elementType{TokenType::Int, "i", name.line, name.column};

        return new ArrayDeclNode{name, elementType, 1, elements, true};
    }
    else if (peek().type == TokenType::Map) {
        return mapDecl(name);
    }
    
    Token type = expectType();
    expect(TokenType::Comma);
    ASTNode* value = expression(); 
    expect(TokenType::Semicolon);
    return new VarDeclNode{name, type, value};
}

std::vector<ASTNode*> Parser::parseList(TokenType expectedType) {
    std::vector<ASTNode*> elements;
    expect(TokenType::BSlash);

    while (peek().type != TokenType::BSlash) {
        if (peek().type == expectedType) {
            elements.push_back(expression());
        } else {
            throw std::runtime_error("P: E29 | Expected type: " + tokenTypeName(expectedType) + "Got: " +tokenTypeName(peek().type));
        }
        if (peek().type == TokenType::Comma) advance();
    }

    expect(TokenType::BSlash);
    return elements;
}

typeBlockInfo Parser::typeBlock() {
    typeBlockInfo info;
    int size = 0;
    Token foundType;
    expect(TokenType::LBrace);
    expect(TokenType::Array);
    expect(TokenType::LParen);
    size = std::stoi(expect(TokenType::Int).value);
    expect(TokenType::RParen);
    
    expect(TokenType::Comma);

    if (peek().type == TokenType::Int || peek().type == TokenType::Float || peek().type == TokenType::UnsignedInt
    || peek().type == TokenType::UnsignedFloat) {
        foundType = advance();
    } else {
        throw std::runtime_error("P: E28 | Expected valid type, got: " + peek().value);
    }

    expect(TokenType::RBrace);

    info.kind = TokenType::Array;
    info.elementType = foundType;
    info.params.push_back(size);

    return info;
}

ASTNode* Parser::ifStatement() {
    expect(TokenType::If);
    ASTNode* condition = expression();
    ASTNode* thenBranch = block();
    ASTNode* elseBranch = nullptr;
    if (peek().type == TokenType::Else) {
        advance();
        elseBranch = block();
        return new IfNode{condition, thenBranch, elseBranch};
    }
    
    return new IfNode{condition, thenBranch, elseBranch};
}

ASTNode* Parser::whileStatement() {
    expect(TokenType::While);
    ASTNode* condition = expression();
    ASTNode* body = block();
    
    return new WhileNode{condition, body};
}

ASTNode* Parser::funcDecl() {
    expect(TokenType::Create);
    Token returnType = expectType();
    Token name = expect(TokenType::Identifier);
    std::vector<Param> parameters;
    expect(TokenType::LParen);

    while (peek().type != TokenType::RParen) {
        Param tempParam;
        tempParam.type = expectType();
        expect(TokenType::Colon);
        tempParam.name = expect(TokenType::Identifier);
        parameters.push_back(tempParam);
        if (peek().type != TokenType::RParen) {
            expect(TokenType::Comma);
        }       
    }

    expect(TokenType::RParen);

    ASTNode* body = block();

    return new FuncDeclNode{returnType, name, parameters, body};
}

ASTNode* Parser::classDecl() {
    bool isOpen = false;

    if (peek().type == TokenType::Open) {
        advance();
        isOpen = true;
    }

    expect(TokenType::Class);

    if (!isOpen) {
        throw std::runtime_error("P: E-minus(1) | Closed class not implemented yet");
    }

    Token name = expect(TokenType::Identifier);

    expect(TokenType::LBracket);

    std::vector<Param> fields;

    while (peek().type != TokenType::RBracket) {
        Param field;
        field.type = expectType();
        expect(TokenType::Colon);
        field.name = expect(TokenType::Identifier);
        fields.push_back(field);
        expect(TokenType::Semicolon);
    }

    expect(TokenType::RBracket);

    return new StructDeclNode{name, fields};
}

ASTNode* Parser::enumDecl() {
    std::vector<ASTNode*> statements;

    expect(TokenType::Enum);
    expect(TokenType::Identifier);
    expect(TokenType::LBracket);

    int counter = -1;

    while (peek().type != TokenType::RBracket) {
        Token enumName = expect(TokenType::Identifier);
        if (peek().type != TokenType::RBracket) {
            expect(TokenType::Comma);
        }

        counter++;

        Token enumType{TokenType::Int, "i", enumName.line, enumName.column};

        statements.push_back(new VarDeclNode{enumName, enumType, new NumberLiteralNode(std::to_string(counter))});
    }

    expect(TokenType::RBracket);

    return new BlockNode{statements};
}

ASTNode* Parser::mapDecl(Token name) {
    expect(TokenType::Map);
    expect(TokenType::LParen);
    Token keyType = expectType();
    expect(TokenType::Comma);
    Token valueType = expectType();
    expect(TokenType::Comma);
    ASTNode* size = expression();

    if (dynamic_cast<CollectionNode*>(size)) {
        throw std::runtime_error("P: E45 | Map size cannot be a collection or array definition");
    }

    expect(TokenType::RParen);
    expect(TokenType::Comma);
    std::vector<ASTNode*> keysVector = parseList(keyType.type);
    expect(TokenType::Comma);
    std::vector<ASTNode*> valuesVector = parseList(valueType.type);

    int convertedSize = evaluateConstant(size);

    ArrayDeclNode* keys = new ArrayDeclNode{name, keyType, convertedSize, keysVector, false};
    ArrayDeclNode* values = new ArrayDeclNode{name, valueType, convertedSize, valuesVector, false};

    expect(TokenType::Semicolon);

    return new MapDeclNode{name, keyType, valueType, size, keys, values};
}

ASTNode* Parser::outStatement() {
    expect(TokenType::Out);
    ASTNode* output = expression();
    expect(TokenType::Semicolon);

    return new OutNode{output};
}

ASTNode* Parser::assignStatement(ASTNode* target) {
    Token op = advance();
    ASTNode* value = expression();
    expect(TokenType::Semicolon);
    return new AssignNode{target, op, value};
}

ASTNode* Parser::pushStatement() {
    expect(TokenType::Push);
    expect(TokenType::LParen);
    Token name = expect(TokenType::Identifier);
    expect(TokenType::Comma);
    ASTNode* value = expression();
    if (peek().type == TokenType::Comma) {
        advance();
        ASTNode* secondValue = expression();
        expect(TokenType::RParen);
        expect(TokenType::Semicolon);

        return new PushNode{name, value, secondValue}; // pushing the key (value) and value (secondValue) onto a map

    }
    expect(TokenType::RParen);
    expect(TokenType::Semicolon);

    return new PushNode{name, value, nullptr};
}

ASTNode* Parser::removeStatement() {
    expect(TokenType::Remove);
    expect(TokenType::LParen);
    Token arrayName = expect(TokenType::Identifier);
    expect(TokenType::Comma);
    ASTNode* index = expression();
    expect(TokenType::RParen);
    expect(TokenType::Semicolon);
    
    return new RemoveNode{arrayName, index};
}

ASTNode* Parser::printStatement() {
    expect(TokenType::Backtick);
    expect(TokenType::LParen);
    ASTNode* value = expression();
    expect(TokenType::RParen);
    expect(TokenType::Semicolon);

    return new PrintNode{value};
}

ASTNode* Parser::exprstatement() {
    ASTNode* expr = expression();
    expect(TokenType::Semicolon);
    return expr;
}

ASTNode* Parser::expression() {
    return comparison();
}

ASTNode* Parser::comparison() {
    ASTNode* left = term(); // arithmetic first

    while (peek().type == TokenType::LessThan || peek().type == TokenType::LessThanOrEqual ||
           peek().type == TokenType::GreaterThan || peek().type == TokenType::GreaterThanOrEqual ||
           peek().type == TokenType::EqualEqual || peek().type == TokenType::NotEqual) {
        Token op = advance();
        ASTNode* right = term();
        left = new BinaryExprNode(left, op, right);
    }

    return left;
}

ASTNode* Parser::term() {
    ASTNode* left = factor();

    while (peek().type == TokenType::Plus || peek().type == TokenType::Minus) {
        Token op = advance();
        ASTNode* right = factor();
        left = new BinaryExprNode(left, op, right);
    }
    return left;
}

ASTNode* Parser::factor() {
    ASTNode* left = primary();

    while (peek().type == TokenType::Star || peek().type == TokenType::FSlash) {
        Token op = advance();
        ASTNode* right = primary();
        left = new BinaryExprNode(left, op, right);
    }
    return left;
}

ASTNode* Parser::primary() {
    if (peek().type == TokenType::Int || peek().type == TokenType::Float) {
        // check numbers

        Token num = advance();
        return new NumberLiteralNode(num.value);
    }

    if (peek().type == TokenType::Identifier) {
        // if variable declaration

        Token id = advance();
        
        if (peek().type == TokenType::BSlash) {
            // if collection

            advance();
            std::vector<ASTNode*> elements;
            elements.push_back(expression());

            while (peek().type == TokenType::Comma) {
                advance();
                elements.push_back(expression());
            }

            expect(TokenType::BSlash);
            return new CollectionNode(id.value, elements);
        }

        if (peek().type == TokenType::Hash) {

            // if index
            bool isGrouped = false;
            advance();
            if (peek().type == TokenType::LParen) {
                advance();
                isGrouped = true;
            }

            ASTNode* index = expression();

            if (isGrouped) expect(TokenType::RParen);

            return new IndexNode{id, index};
        }

        if (peek().type == TokenType::Dot) {
            // if instance access

            advance();

            Token field = advance();

            return new FieldAccessNode{new IdentifierNode(id.value), field};
        }

        if (peek().type == TokenType::Character) {
            int savedPos = peek().line;
            Token literal = advance();
            if (literal.value == "") {
                throw std::runtime_error("P: E42-2 | Cannot parse an empty character at line: " + std::to_string(savedPos));
            }

            std::string ascii = std::to_string(static_cast<int>(literal.value[0]));

            return new NumberLiteralNode{ascii};
        }
        
        return new IdentifierNode(id.value);
    }

    if (peek().type == TokenType::String) {
            Token literal = advance();
            std::vector<ASTNode*> elements;

            for (const char& c : literal.value) {
                std::string ascii = std::to_string(static_cast<int>(c));
                elements.push_back(new NumberLiteralNode(ascii));
            }

            std::string stringName = std::format("str_{}", p_uniqueCount++);

            Token elementType{TokenType::Int, "i", literal.line, literal.column};
            Token name{TokenType::Str, stringName, literal.line, literal.column};

            return new ArrayDeclNode{name, elementType, (int)elements.size(), elements, true};
        }

    if (peek().type == TokenType::Call) {
        // if calling function

        expect(TokenType::Call);
        Token name = expect(TokenType::Identifier);
        expect(TokenType::LParen);

        std::vector<ASTNode*> arguments;

        while (peek().type != TokenType::RParen) {
            ASTNode* argument = expression();
            arguments.push_back(argument);
            if (peek().type != TokenType::RParen) {
                expect(TokenType::Comma);
            }    
        }

        expect(TokenType::RParen);

        return new CallNode{name, arguments};
    }

    if (peek().type == TokenType::Question) {
        // instance creation

        advance();
        Token structName = expect(TokenType::Identifier);
        expect(TokenType::LParen);

        std::vector<ASTNode*> arguments;

        while (peek().type != TokenType::RParen) {
            ASTNode* argument = expression();
            arguments.push_back(argument);
            if (peek().type != TokenType::RParen) {
                expect(TokenType::Comma);
            }    
        }
        
        expect(TokenType::RParen);

        return new InstanceNode{structName, arguments};
    }


    throw std::runtime_error("P: E27 | Expected expression at line " + std::to_string(peek().line));
}

ASTNode* Parser::block() {
    expect(TokenType::LBracket);

    std::vector<ASTNode*> statements;
    while (peek().type != TokenType::RBracket && peek().type != TokenType::EndOfFile) {
        statements.push_back(statement());
    }

    expect(TokenType::RBracket);

    return new BlockNode{statements};
}

Parser::Parser(const std::vector<Token>& tokens, std::filesystem::path currentDir) : tokens(tokens), currentDir(currentDir) {}

ASTNode* Parser::parse() {
    std::vector<ASTNode*> statements;
    while (peek().type == TokenType::Insert) {
        expect(TokenType::Insert);
        Token StringLiteral = expect(TokenType::String);
        expect(TokenType::Semicolon);

        std::filesystem::path insertedPath = currentDir / StringLiteral.value;
        std::string insertSource = readFile(insertedPath.string()); // we read the inserted file

        Lexer lexer(insertSource);
        std::vector<Token> tokens = lexer.tokenize();

        for (auto token : tokens) {
            std::cout << "[insert] Type: " << tokenTypeName(token.type) << " Lexeme: " << token.value << std::endl;
        }

        Parser parser(tokens, insertedPath.parent_path());

        ASTNode* insertedFile = parser.parse(); // recursively parse: if insertedFile also has an insert, it will return to this code block

        for (auto statement : dynamic_cast<BlockNode*>(insertedFile)->statements) {
            statements.push_back(statement);
        }

    }
    while (peek().type != TokenType::EndOfFile) {
        statements.push_back(statement());
    }

    return new BlockNode(statements);
}

void printAST(ASTNode* node, int depth) {
    std::string indent(depth * 2, ' ');

    if (auto arr = dynamic_cast<ArrayDeclNode*>(node)) {
        std::cout << indent << "Array: " << std::format("'{}', ", arr->name.value) << arr->elementType.value << ", size " << arr->size << "\n";
        for (auto element : arr->elements) {
            printAST(element, depth + 1);
        }
    }

    else if (auto v = dynamic_cast<VarDeclNode*>(node)) {
        std::cout << indent << "VarDecl: " << v->name.value << " : " << v->type.value << "\n";
        printAST(v->value, depth + 1);
    }

    else if (auto b = dynamic_cast<BinaryExprNode*>(node)) {
        std::cout << indent << "BinaryExpr: " << b->op.value << "\n";
        printAST(b->left, depth + 1);
        printAST(b->right, depth + 1);
    }

    else if (auto n = dynamic_cast<NumberLiteralNode*>(node)) {
        std::cout << indent << "Number: " << n->value << "\n";
    }

    else if (auto i = dynamic_cast<IdentifierNode*>(node)) {
        std::cout << indent << "Identifier: " << i->value << "\n";
    }

    else if (auto assign = dynamic_cast<AssignNode*>(node)) {
        std::cout << indent << "Assignment: " << "\n";
        printAST(assign->value, depth + 1);
    }

    else if (auto blk = dynamic_cast<BlockNode*>(node)) {
        std::cout << indent << "Block:\n";
        for (auto stmt : blk->statements) {
            printAST(stmt, depth + 1);
        }
    }

    else if (auto ifNode = dynamic_cast<IfNode*>(node)) {
        std::cout << indent << "If:\n";
        std::cout << indent << "  Condition:\n";
        printAST(ifNode->condition, depth + 2);
        std::cout << indent << "  Then:\n";
        printAST(ifNode->thenBranch, depth + 2);
        if (ifNode->elseBranch != nullptr) {
            std::cout << indent << "  Else:\n";
            printAST(ifNode->elseBranch, depth + 2);
        }
    }

    else if (auto idx = dynamic_cast<IndexNode*>(node)) {
        std::cout << "Index: " << "\n";
        printAST(idx->index, depth + 1);
    }

    else if (auto mapDecl = dynamic_cast<MapDeclNode*>(node)) {
        std::cout << "Map:" << mapDecl->name.value << "\n";
        std::cout << indent << "Keys:" << "\n";
        printAST(mapDecl->keys, depth + 1);
        std::cout << indent << "Values:" << "\n";
        printAST(mapDecl->values, depth + 1);
    }

    else if (auto funcDecl = dynamic_cast<FuncDeclNode*>(node)) {
        std::cout << indent << "FuncDecl: " << funcDecl->name.value << "\n";
        std::cout << indent << "Parameters: " << "\n";
        depth++;
        for (auto param : funcDecl->parameters) {
            std::cout << indent << param.name.value << "\n";
        }
    }
    
    else if (auto callNode = dynamic_cast<CallNode*>(node)) {
        std::cout << indent << "Call: " << callNode->name.value << "\n";
        for (auto arg : callNode->arguments) {
            printAST(arg, depth + 1);
        }

    }

    else if (auto structDecl = dynamic_cast<StructDeclNode*>(node)) {
        std::cout << "Class: " << structDecl->name.value << "\n";
        for (auto& field : structDecl->fields) {
            std::cout << indent << "  " << field.type.value << ": " << field.name.value << "\n";
        }
    }

    else if (auto inst = dynamic_cast<InstanceNode*>(node)) {
        std::cout << indent << "Instance: " << inst->structName.value << "\n";
        for (auto arg : inst->arguments) {
            printAST(arg, depth + 1);
        }
    }

    else if (auto field = dynamic_cast<FieldAccessNode*>(node)) {
        std::cout << indent << "FieldAccess: ." << field->field.value << "\n";
        printAST(field->target, depth + 1);
    }

    else if (auto push = dynamic_cast<PushNode*>(node)) {
        std::cout << indent << "Push:" << "\n";
        printAST(push->value, depth + 1);
    }

    else if (auto remove = dynamic_cast<RemoveNode*>(node)) {
        std::cout << indent << "Remove element at index: " << "\n";
        printAST(remove->index, depth + 1);
    }

    else if (auto printNode = dynamic_cast<PrintNode*>(node)) {
        std::cout << indent << "Print: " << "\n";
        printAST(printNode->value, depth + 1);
    }

    else {
        std::cout << indent << "Unknown node\n";
    }
}