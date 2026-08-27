#include <unordered_map>
#include "ast.h"
#include "type.h"
#include <stdexcept>


void SymbolTable::declare(const std::string& name, TokenType& type) {
    if (isDeclared(name)) {
        throw std::runtime_error("TC: E1 | '" + name + "' is already declared");
    }

    table[name] = type;
}

bool SymbolTable::isDeclared(const std::string& name) const {
    return exists(name) || arrayExists(name);
}

TokenType SymbolTable::lookup(const std::string& name) const {
    auto t_it = table.find(name);
    if (t_it != table.end()) return t_it->second;

    auto a_it = arrays.find(name);
    if (a_it != arrays.end()) return a_it->second.elementType;

    throw std::runtime_error("TC: E2 | '" + name + "' is not declared");
}

bool SymbolTable::exists(const std::string& name) const {
    return table.find(name) != table.end();
}

bool SymbolTable::arrayExists(const std::string& name) const {
    return arrays.find(name) != arrays.end();
}

void SymbolTable::arrayDeclare(const std::string& name, TokenType& elementType, bool isImmutable) {
    if (isDeclared(name)) {
        throw std::runtime_error("TC: E3 | '" + name + "' is already declared");
    }

    arrays[name].elementType = elementType;
    arrays[name].isImmutable = isImmutable;
}

void SymbolTable::remove(const std::string& name) {
    table.erase(name);
}

bool TypeChecker::nameTaken(const std::string& name) {
    if (structTable.find(name) != structTable.end()) return true;
    if (functions.find(name) != functions.end()) return true;
    return symbols.exists(name) || symbols.arrayExists(name);
}

TokenType TypeChecker::TypeCheck(ASTNode* node) {
    if (auto num = dynamic_cast<NumberLiteralNode*>(node)) {
        return TokenType::Int; // Placeholder before floats
    }

    if (auto id = dynamic_cast<IdentifierNode*>(node)) {
        return symbols.lookup(id->value);
    }

    if (auto bin = dynamic_cast<BinaryExprNode*>(node)) {
        TokenType leftType = TypeCheck(bin->left);
        TokenType rightType = TypeCheck(bin->right);

        if (leftType != rightType) {
            throw std::runtime_error("TC: E4 | Type mismatch in binary expression: '" + tokenTypeName(leftType) + "' -> '" + tokenTypeName(rightType) + "'");
        }
        
        switch(bin->op.type) {
            case TokenType::LessThan:
            case TokenType::GreaterThan:
            case TokenType::LessThanOrEqual:
            case TokenType::GreaterThanOrEqual:
            case TokenType::EqualEqual:
            case TokenType::NotEqual:
                return TokenType::Bool;
            
            default: return leftType;
        }
    }

    if (auto arr = dynamic_cast<ArrayDeclNode*>(node)) {
        if (nameTaken(arr->name.value)) {
            throw std::runtime_error("TC: E5 | '" + arr->name.value + "' is already declared");
        }
        
        TokenType declaredType = arr->elementType.type;
        int counter = 0;
        for (auto element : arr->elements) {
            TokenType elementValue = TypeCheck(element);
            if (elementValue != declaredType) {
                throw std::runtime_error("TC: E6 | Type mismatch in array declaration at index " + std::to_string(counter) + ", expected: " + tokenTypeName(arr->elementType.type) + " Got: " + tokenTypeName(elementValue));
            }
            counter++;
        }

        if (arr->elements.size() > arr->size) {
            throw std::runtime_error("TC: E7 | Array '" + arr->name.value + "' declared size " 
                + std::to_string(arr->size) + " but got " + std::to_string(arr->elements.size()) + " elements");
        }

        symbols.arrayDeclare(arr->name.value, declaredType, arr->isImmutable);

        return declaredType;
    }

    if (auto idx = dynamic_cast<IndexNode*>(node)) {
        if (TokenType::Int != TypeCheck(idx->index)) {
            throw std::runtime_error("TC: E8 | Non-integer index for identifier '" + idx->name.value + "'");
        }

        if (!(symbols.arrayExists(idx->name.value))) {
            throw std::runtime_error("TC: E9 | '" + idx->name.value + "' is not an array");
        } 

        return symbols.lookup(idx->name.value);
    }

    if (auto varDecl = dynamic_cast<VarDeclNode*>(node)) {
        if (nameTaken(varDecl->name.value)) {
            throw std::runtime_error("TC: E10 | '" + varDecl->name.value + "' is already declared");
        }

        TokenType value = TypeCheck(varDecl->value);

        if (varDecl->type.type == TokenType::Identifier) {
            // declared type is a struct name

            if (structTable.find(varDecl->type.value) == structTable.end()) {
                throw std::runtime_error("TC: E11 | Unknown type '" + varDecl->type.value + "' for '" + varDecl->name.value + "'");
            }

            if (value != TokenType::Struct || lastStructName != varDecl->type.value) {
                throw std::runtime_error("TC: E12 | Type mismatch in variable declaration for '" + varDecl->name.value + "'");
            }

            instances[varDecl->name.value] = varDecl->type.value;
        } else {
            TokenType declaredType = varDecl->type.type;
            if (value != declaredType) {
                throw std::runtime_error("TC: E13 | Type mismatch in variable declaration for '" + varDecl->name.value + "'");
            }
        }

        symbols.declare(varDecl->name.value, varDecl->type.type);

        return varDecl->type.type;
    }

    if (auto structDecl = dynamic_cast<StructDeclNode*>(node)) {
        if (nameTaken(structDecl->name.value)) {
            throw std::runtime_error("TC: E14 | '" + structDecl->name.value +  "' is already declared");
        }
        structTable[structDecl->name.value] = structDecl->fields;

        return TokenType::Sentinel;
    }

    if (auto field = dynamic_cast<FieldAccessNode*>(node)) {
        auto target = dynamic_cast<IdentifierNode*>(field->target);

        if (!target) {
            throw std::runtime_error("TC: E34 | No identifier found for field access");
        }

        if (instances.find(target->value) == instances.end()) {
            throw std::runtime_error("TC: E32 | No such instance exists: '" + target->value + "'");
        }

        std::string currentInstance = instances[target->value];

        auto& fields = structTable[currentInstance];

        bool found = false;
        TokenType fieldType = TokenType::Sentinel;

        for (auto& checkedField : fields) {
            if (checkedField.name.value == field->field.value ) {
                found = true;
                fieldType = checkedField.type.type;
                break;
            }
        }

        if (!found) {
            throw std::runtime_error("TC: E33 | No such field");
        }

        return fieldType;

    }

    if (auto inst = dynamic_cast<InstanceNode*>(node)) {
        if (structTable.find(inst->structName.value)  == structTable.end()) {
            throw std::runtime_error("TC: E24 | Unknown struct: '" + inst->structName.value + "'");
        }

        auto& fields = structTable[inst->structName.value];

        if (inst->arguments.size() != fields.size()) {
            throw std::runtime_error("TC: E25 | Struct '" + inst->structName.value + "' expected: " + 
                std::to_string(fields.size()) + " Got: " + std::to_string(inst->arguments.size()));
        }

        for (size_t i = 0; i < inst->arguments.size(); i++) {
            TokenType argType = TypeCheck(inst->arguments[i]);

            if (argType != fields[i].type.type) {
                throw std::runtime_error("TC: E26 | Field: " + fields[i].name.value + " Expected: " + tokenTypeName(fields[i].type.type) + " Got: " + tokenTypeName(argType));
            }
        }
        
        lastStructName = inst->structName.value;

        return TokenType::Struct;
    }

    if (auto block = dynamic_cast<BlockNode*>(node)) {
        TokenType last = TokenType::Null;
        for (auto statement: block->statements) {
            last = TypeCheck(statement);
        }
        return last;
    }

    if (auto ifNode = dynamic_cast<IfNode*>(node)) {
        TokenType condition = TypeCheck(ifNode->condition);

        if (condition != TokenType::Bool) {
            throw std::runtime_error("TC: E16 | If condition must be Bool");
        }
        TypeCheck(ifNode->thenBranch);

        if (ifNode->elseBranch != nullptr) {
            TypeCheck(ifNode->elseBranch);
        }

        return TokenType::Null;
    }

    if (auto whileNode = dynamic_cast<WhileNode*>(node)) {
        TokenType condition = TypeCheck(whileNode->condition);

        if (condition != TokenType::Bool) {
            throw std::runtime_error("TC: E17 | If condition must be Bool");
        }

        TypeCheck(whileNode->body);

        return TokenType::Null;
    }

    if (auto assignNode = dynamic_cast<AssignNode*>(node)) {
        TokenType targetType = TypeCheck(assignNode->target);
        TokenType value = TypeCheck(assignNode->value);

        if (targetType != value) {
            throw std::runtime_error("TC: E18 | Cannot assign due to type mismatch");
        }

        return targetType;
    }

    if (auto funcDecl = dynamic_cast<FuncDeclNode*>(node)) {
        if (nameTaken(funcDecl->name.value)) {
            throw std::runtime_error("TC: E19 | '" + funcDecl->name.value + "' is already declared");
        }

        FuncType current;
        current.type = funcDecl->returnType.type;
        for (auto param : funcDecl->parameters) {
            current.paramTypes.push_back(param.type.type);
        }

        functions[funcDecl->name.value] = current;

        for (auto param : funcDecl->parameters) {
            symbols.declare(param.name.value, param.type.type);
        }

        returnType = current.type;

        TypeCheck(funcDecl->body);

        returnType = TokenType::Sentinel;

        for (auto param : funcDecl->parameters) {
            symbols.remove(param.name.value);
        }

        return current.type;

    }

    if (auto push = dynamic_cast<PushNode*>(node)) {
        if (!(symbols.arrayExists(push->arrayName.value))) {
            throw std::runtime_error("TC: E38-1 |'" + push->arrayName.value + "' is not an array");
        }

        TokenType elementType = symbols.lookup(push->arrayName.value);
        TokenType valueType = TypeCheck(push->value);

        if (elementType != valueType) {
            throw std::runtime_error("TC: E39 | Cannot push value due to type mismatch, expecting: " + tokenTypeName(elementType));
        }

        if (symbols.arrays[push->arrayName.value].isImmutable) {
            throw std::runtime_error("TC: E43-1 | Cannot push onto an immutable array: '" + push->arrayName.value + "'");
        }

        return TokenType::Sentinel;
    }

    if (auto remove = dynamic_cast<RemoveNode*>(node)) {
        if (!(symbols.arrayExists(remove->arrayName.value))) {
            throw std::runtime_error("TC: E38-2 |'" + remove->arrayName.value + "' is not an array");
        }

        if (TypeCheck(remove->index) != TokenType::Int) {
            throw std::runtime_error("TC: E40 | Non-integer index cannot be used");
        }

        if (symbols.arrays[remove->arrayName.value].isImmutable) {
            throw std::runtime_error("TC: E43-2 | Cannot push onto an immutable array: '" + remove->arrayName.value + "'");
        }


        return TokenType::Sentinel;
    }

    if (auto callNode = dynamic_cast<CallNode*>(node)) {
        auto it = functions.find(callNode->name.value);
        if (it == functions.end()) {
            throw std::runtime_error("TC: E20 | Undefined function:" + callNode->name.value);
        }

        FuncType typeStruct = it->second;

        if (callNode->arguments.size() != typeStruct.paramTypes.size()) {
            throw std::runtime_error("TC: E21 | Expected " + std::to_string(typeStruct.paramTypes.size()) + " arguments, got " + std::to_string(callNode->arguments.size()));
        }

        int index = 0;
        for (auto argument : callNode->arguments) {
            TokenType type = TypeCheck(argument);
            if (type != typeStruct.paramTypes[index]) {
                throw std::runtime_error("TC: E22 | Expected type: " + tokenTypeName(typeStruct.paramTypes[index]) + "Got: " + tokenTypeName(type));
            }
            index++;
        }
        return typeStruct.type;
    }
    
    if (auto outNode = dynamic_cast<OutNode*>(node)) {
        TokenType outType = TypeCheck(outNode->output);

        if (outType == returnType) {
            return outType;
        } else {
            throw std::runtime_error("TC: E23 | Expected: " + tokenTypeName(returnType) + " Got: " + tokenTypeName(outType));
        }
    }

    if (auto printValue = dynamic_cast<PrintNode*>(node)) {
        TokenType printType = TypeCheck(printValue->value);

        if (printType == TokenType::Int) {
            return printType;
        }
        else {
            throw std::runtime_error("TC: E41 | Expected int type, got: " + tokenTypeName(printType));
        }
    }

    throw std::runtime_error("TC: E24 | Unhandled node type");
}