#include <unordered_map>
#include "ast.h"
#include "type.h"
#include <stdexcept>
#include <algorithm>


void SymbolTable::declare(const std::string& name, TokenType& type) {
    if (isDeclared(name)) {
        throw std::runtime_error("TC: E1 | '" + name + "' is already declared");
    }

    table[name] = type;
}

bool SymbolTable::isDeclared(const std::string& name) const {
    return exists(name) || arrayExists(name) || mapExists(name);
}

TokenType SymbolTable::lookup(const std::string& name) const {
    auto t_it = table.find(name);
    if (t_it != table.end()) return t_it->second;

    auto a_it = arrays.find(name);
    if (a_it != arrays.end()) return a_it->second.elementType;

    throw std::runtime_error("TC: E2 | '" + name + "' is not declared");
}

TokenType SymbolTable::mapKeyType(const std::string& name) const {
    TokenType key = maps.at(name).keyType;
    if (key == TokenType::Str) {
        return TokenType::String;
    } else {
        return key;
    }
}

TokenType SymbolTable::mapValueType(const std::string& name) const {
    TokenType value = maps.at(name).valueType;
    if (value == TokenType::Str) {
        return TokenType::String;
    } else {
        return value;
    }
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

void SymbolTable::mapDeclare(const std::string& name, TokenType& keyType, TokenType& valueType, bool isImmutable) {
    if (isDeclared(name)) {
        throw std::runtime_error("TC: E52 | '" + name + "' is already declared");
    }

    maps[name].keyType = keyType;
    maps[name].valueType = valueType;
    maps[name].isImmutable = isImmutable;

}

bool SymbolTable::mapExists(const std::string& name) const {
    return maps.find(name) != maps.end();
}

void SymbolTable::remove(const std::string& name) {
    table.erase(name);
}

void SymbolTable::arrayRemove(const std::string& name) {
    arrays.erase(name);
}

void SymbolTable::mapRemove(const std::string& name) {
    maps.erase(name);
}

bool TypeChecker::nameTaken(const std::string& name) {
    if (structTable.find(name) != structTable.end()) return true;
    if (functions.find(name) != functions.end()) return true;
    return symbols.isDeclared(name);
}

void TypeChecker::registerMethodSignature(FuncDeclNode* funcDecl, const std::string& structName) {
     std::string key = structName + "_" + funcDecl->name.value;
    if (nameTaken(key)) {
        throw std::runtime_error("TC: E19-3 | '" + key + "' is already declared");
    }

    FuncType current;
    current.type = funcDecl->returnType.type;
    if (current.type == TokenType::Identifier) {
            current.returnStructName = funcDecl->returnType.value;
            if (structTable.find(current.returnStructName) == structTable.end()) {
                throw std::runtime_error("TC: E73-3 | Unknown return type '" + current.returnStructName + "' for '" + funcDecl->name.value + "'");
            }
            current.type = TokenType::Struct;
        }
    
    if (current.type == TokenType::Char) {
        current.type = TokenType::Int;
    }

    for (auto param : funcDecl->parameters) {
        TokenType paramType = param.type.type;
        if (paramType == TokenType::Char) {
            paramType = TokenType::Int;
        }

        current.paramTypes.push_back(paramType);
        current.isReference.push_back(param.isReference);
    }

    functions[key] = current;
}

void TypeChecker::checkMethodBody(FuncDeclNode* funcDecl, const std::string& structName) {
    std::string key = structName + "_" + funcDecl->name.value;
    FuncType current = functions[key];
    for (auto param : funcDecl->parameters) {
        symbols.declare(param.name.value, param.type.type);
    }

    returnType = current.type;
    returnStructName = current.returnStructName;

    instances["inst"] = structName;

    std::vector<std::string> scalarKeysBefore, arrayKeysBefore, mapKeysBefore;
    for (auto& [k, v] : symbols.table) scalarKeysBefore.push_back(k);
    for (auto& [k, v] : symbols.arrays) arrayKeysBefore.push_back(k);
    for (auto& [k, v] : symbols.maps) mapKeysBefore.push_back(k);

    TypeCheck(funcDecl->body);

    returnType = TokenType::Sentinel;
    returnStructName = "";

    for (auto param : funcDecl->parameters) {
        symbols.remove(param.name.value);
    }
    std::vector<std::string> toRemoveScalars, toRemoveArrays, toRemoveMaps;
    for (auto& [k, v] : symbols.table) {
        if (std::find(scalarKeysBefore.begin(), scalarKeysBefore.end(), k) == scalarKeysBefore.end()) {
            toRemoveScalars.push_back(k);
        }
    }
    for (auto& [k, v] : symbols.arrays) {
        if (std::find(arrayKeysBefore.begin(), arrayKeysBefore.end(), k) == arrayKeysBefore.end()) {
            toRemoveArrays.push_back(k);
        }
    }
    for (auto& [k, v] : symbols.maps) {
        if (std::find(mapKeysBefore.begin(), mapKeysBefore.end(), k) == mapKeysBefore.end()) {
            toRemoveMaps.push_back(k);
        }
    }
    for (auto& k : toRemoveScalars) symbols.remove(k);
    for (auto& k : toRemoveArrays) symbols.arrayRemove(k);
    for (auto& k : toRemoveMaps) symbols.mapRemove(k);
}

TokenType TypeChecker::TypeCheck(ASTNode* node) {
    if (auto num = dynamic_cast<NumberLiteralNode*>(node)) {
        return TokenType::Int; // Placeholder before floats
    }

    if (auto id = dynamic_cast<IdentifierNode*>(node)) {
        if (instances.find(id->value) != instances.end()) {
            return TokenType::Struct;
        }
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

            case TokenType::L_AND:
            case TokenType::L_OR:
                if (leftType != TokenType::Bool) {
                    throw std::runtime_error("TC: E72 | Logical operator requires Bool operands, got: " + tokenTypeName(leftType));
                }
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

        if (arr->isImmutable) {
            return TokenType::String;
        }

        return declaredType;
    }

    if (auto idx = dynamic_cast<IndexNode*>(node)) {

        if (!(symbols.arrayExists(idx->name.value))) {
            if (symbols.mapExists(idx->name.value)) {
                TokenType keyType = symbols.mapKeyType(idx->name.value);

                if (TypeCheck(idx->index) != keyType) {
                    throw std::runtime_error("TC: E53 | Invalid key index");
                }

                return symbols.mapValueType(idx->name.value);
            }

            throw std::runtime_error("TC: E9 | '" + idx->name.value + "' is not an array");
        } 

        if (TokenType::Int != TypeCheck(idx->index)) {
            throw std::runtime_error("TC: E8 | Non-integer index for identifier '" + idx->name.value + "'");
        }

        return symbols.lookup(idx->name.value);
    }

    if (auto varDecl = dynamic_cast<VarDeclNode*>(node)) {
        if (nameTaken(varDecl->name.value)) {
            throw std::runtime_error("TC: E10 | '" + varDecl->name.value + "' is already declared");
        }

        TokenType value = TypeCheck(varDecl->value);
        TokenType declaredType = varDecl->type.type;
        if (declaredType == TokenType::Str) {
            declaredType = TokenType::String;
        }
        if (declaredType == TokenType::Char) {
            declaredType = TokenType::Int;
        }

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
            if (value != declaredType) {
                throw std::runtime_error("TC: E13 | Type mismatch in variable declaration for '" + varDecl->name.value + "'");
            }

            if (declaredType == TokenType::String) {
                TokenType elementType = TokenType::Int;
                symbols.arrayDeclare(varDecl->name.value, elementType, true);
            } else {
                symbols.declare(varDecl->name.value, declaredType);
            }
        }
        

        return varDecl->type.type;
    }

    if (auto structDecl = dynamic_cast<StructDeclNode*>(node)) {
        if (nameTaken(structDecl->name.value)) {
            throw std::runtime_error("TC: E14 | '" + structDecl->name.value +  "' is already declared");
        }

        StructInfo info;
        info.fields = structDecl->fields;
        structTable[structDecl->name.value] = info;
        
        // Pass 1
        for (auto method : structDecl->methods) {
            if (auto func = dynamic_cast<FuncDeclNode*>(method.second)) {
                registerMethodSignature(func, structDecl->name.value);
            } else {
                throw std::runtime_error("TC: E15 | Invalid method declaration for struct '" + structDecl->name.value + "'");
            }

            info.methods[method.first] = method.second;
        }

        // Pass 2
        for (auto method : structDecl->methods) {
            checkMethodBody(dynamic_cast<FuncDeclNode*>(method.second), structDecl->name.value);
        }


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

        auto& structFields = structTable[currentInstance];

        bool found = false;
        TokenType fieldType = TokenType::Sentinel;

        for (auto& checkedField : structFields.fields) {
            if (checkedField.name.value == field->field.value ) {
                found = true;
                fieldType = checkedField.type.type;
                if (fieldType == TokenType::Str) {
                    fieldType = TokenType::String;
                }
                break;
            }
        }

        if (!found) {
            throw std::runtime_error("TC: E33 | No such field: '" + field->field.value + "'");
        }

        return fieldType;
    }

    if (auto method = dynamic_cast<MethodNode*>(node)) {
        if (instances.find(method->targetName.value) == instances.end()) {
            throw std::runtime_error("TC: E61 | No instance found for method call: '" + method->methodName.value + "'");
        }

        std::string structName = instances[method->targetName.value];
        std::string key = structName + "_" + method->methodName.value;

        auto it = functions.find(key);
        if (it == functions.end()) {
            throw std::runtime_error("TC: E62 | Undefined method: '" + method->methodName.value + "'");
        }

        FuncType typeStruct = it->second;

        if (method->arguments.size() != typeStruct.paramTypes.size()) {
            throw std::runtime_error("TC: E63 | Expected " + std::to_string(typeStruct.paramTypes.size()) + " arguments, got " + std::to_string(method->arguments.size()));
        }

        int index = 0;

        for (auto& arg : method->arguments){
            TokenType argType = TypeCheck(arg);
            if (argType != typeStruct.paramTypes[index]) {
                throw std::runtime_error("TC: E64 | Expected type: " + tokenTypeName(typeStruct.paramTypes[index]) + ", got: " + tokenTypeName(argType));
            }
            index++;
        }

        lastStructName = typeStruct.returnStructName;
        return typeStruct.type;
    }

    if (auto inst = dynamic_cast<InstanceNode*>(node)) {
        if (structTable.find(inst->structName.value)  == structTable.end()) {
            throw std::runtime_error("TC: E24 | Unknown struct: '" + inst->structName.value + "'");
        }

        auto& structFields = structTable[inst->structName.value];

        if (inst->arguments.size() != structFields.fields.size()) {
            throw std::runtime_error("TC: E25 | Struct '" + inst->structName.value + "' expected: " + 
                std::to_string(structFields.fields.size()) + " Got: " + std::to_string(inst->arguments.size()));
        }

        for (size_t i = 0; i < inst->arguments.size(); i++) {
            TokenType argType = TypeCheck(inst->arguments[i]);
            TokenType expectedType = structFields.fields[i].type.type;
            if (expectedType == TokenType::Str) {
                expectedType = TokenType::String;
            }

            if (argType != expectedType) {
                throw std::runtime_error("TC: E26 | Field: " + structFields.fields[i].name.value + " Expected: " + tokenTypeName(structFields.fields[i].type.type) + " Got: " + tokenTypeName(argType));
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
            throw std::runtime_error("TC: E19-1 | '" + funcDecl->name.value + "' is already declared");
        }

        FuncType current;
        current.type = funcDecl->returnType.type;
        if (current.type == TokenType::Identifier) {
            current.returnStructName = funcDecl->returnType.value;
            current.returnStructName = funcDecl->returnType.value;
            if (structTable.find(current.returnStructName) == structTable.end()) {
                throw std::runtime_error("TC: E73-1 | Unknown return type '" + current.returnStructName + "' for '" + funcDecl->name.value + "'");
            }
        }

        if (current.type == TokenType::Char) {
            current.type = TokenType::Int;
        }

        for (auto param : funcDecl->parameters) {
            TokenType paramType = param.type.type;
            if (paramType == TokenType::Char) {
                paramType = TokenType::Int;
            }
            if (paramType == TokenType::Identifier) {
                paramType = TokenType::Struct;
            }

            current.paramTypes.push_back(paramType);
            current.isReference.push_back(param.isReference);
        }

        functions[funcDecl->name.value] = current;

        for (auto param : funcDecl->parameters) {
            symbols.declare(param.name.value, param.type.type);
            if (param.type.type == TokenType::Identifier) {
                instances[param.name.value] = param.type.value;
            }
        }

        returnType = current.type;
        returnStructName = current.returnStructName;

        TypeCheck(funcDecl->body);

        returnType = TokenType::Sentinel;
        returnStructName = "";

        for (auto param : funcDecl->parameters) {
            symbols.remove(param.name.value);
        }

        return current.type;
    }

    if (auto mapDecl = dynamic_cast<MapDeclNode*>(node)) {
        if (mapDecl->keyType.type == TokenType::Int || mapDecl->keyType.type == TokenType::Bool
            || mapDecl->keyType.type == TokenType::Float || mapDecl->keyType.type == TokenType::String 
            || mapDecl->keyType.type == TokenType::Character || mapDecl->keyType.type == TokenType::Str) {

        } else {
            throw std::runtime_error("TC: E46-1 | Invalid key type");
        }

        if (mapDecl->valueType.type == TokenType::Int || mapDecl->valueType.type == TokenType::Bool
            || mapDecl->valueType.type == TokenType::Float || mapDecl->valueType.type == TokenType::String 
            || mapDecl->valueType.type == TokenType::Character || mapDecl->valueType.type == TokenType::Struct
            || mapDecl->valueType.type == TokenType::Str) {

        } else {
            throw std::runtime_error("TC: E46-2 | Invalid value type");
        }

        if (mapDecl->keys->elements.size() != mapDecl->values->elements.size()) {
            throw std::runtime_error("TC: E50 | Map keys and values count mismatch");
        }


        if (TypeCheck(mapDecl->size) != TokenType::Int) {
            throw std::runtime_error("TC: E47 | Non-integer map size");
        }

        size_t mapSize = evaluateConstant(mapDecl->size);

        if (mapSize < mapDecl->keys->elements.size() || mapSize < mapDecl->values->elements.size()) {
            throw std::runtime_error("TC: E51 | Map size cannot exceed declared size");
        }

        std::string mapName = mapDecl->name.value;
        TokenType keyType = mapDecl->keyType.type;
        TokenType valueType = mapDecl->valueType.type;

        symbols.mapDeclare(mapName, keyType, valueType, false);

        return TokenType::Map;
    }

    if (auto push = dynamic_cast<PushNode*>(node)) {
        if (symbols.mapExists(push->arrayName.value)) {
            TokenType keyType = symbols.mapKeyType(push->arrayName.value);
            if (TypeCheck(push->value) != keyType) {
                throw std::runtime_error("TC: E55 | Cannot push invalid key type");
            }

            if (push->secondValue == nullptr) {
                throw std::runtime_error("TC: E56 | Map push must take two values");
            }

            TokenType valueType = symbols.mapValueType(push->arrayName.value);
            if (TypeCheck(push->secondValue) != valueType) {
                throw std::runtime_error("TC: E57 | Cannot push invalid value type");
            }

            return TokenType::Sentinel;
        }


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

        if (push->secondValue != nullptr) {
            throw std::runtime_error("TC: E54 | Array push takes only one value");
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

    if (auto size = dynamic_cast<SizeNode*>(node)) {
        if (!(symbols.arrayExists(size->name.value))) {
            if (!(symbols.mapExists(size->name.value))) {
                throw std::runtime_error("TC: E68 | Invalid object for size operation: '" + size->name.value + "'");
            }
        }

        return TokenType::Int;
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
                throw std::runtime_error("TC: E22 | Expected type: " + tokenTypeName(typeStruct.paramTypes[index]) + " Got: " + tokenTypeName(type));
            }
            if (typeStruct.isReference[index]) {
                if (!dynamic_cast<IdentifierNode*>(argument)) {
                    throw std::runtime_error("TC: E67 | Unexpected reference type for argument at index " + std::to_string(index));
                }
            }

            index++;
        }
        return typeStruct.type;
    }
    
    if (auto outNode = dynamic_cast<OutNode*>(node)) {
        TokenType outType = TypeCheck(outNode->output);

        if (outType != returnType) {
            throw std::runtime_error("TC: E23 | Expected out: " + tokenTypeName(returnType) + " Got: " + tokenTypeName(outType));
        }

        if (returnType == TokenType::Struct && lastStructName != returnStructName) {
            throw std::runtime_error("TC: E74 | Expected struct out '" + lastStructName + " Got: '" + returnStructName + "'");
        }

        return outType;
    }

    if (auto printValue = dynamic_cast<PrintNode*>(node)) {
        TokenType printType = TypeCheck(printValue->value);

        if (printType == TokenType::Int || printType == TokenType::String) {
            return printType;
        }
        else {
            throw std::runtime_error("TC: E41 | Expected printable type, got: " + tokenTypeName(printType));
        }
    }

    throw std::runtime_error("TC: E24 | Unhandled node type");
}