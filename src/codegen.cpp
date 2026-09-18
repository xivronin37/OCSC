#include <iostream>
#include <string>
#include <format>
#include "lexer.h"
#include "ast.h"
#include "type.h"
#include "codegen.h"

int countVarDecl(ASTNode* node) {
    int counter = 0;

    if (auto arrDecl = dynamic_cast<ArrayDeclNode*>(node)) {
        counter += arrDecl->elements.size();
        counter++;
    }

    if (auto varDecl = dynamic_cast<VarDeclNode*>(node)) {
       counter += countVarDecl(varDecl->value);
        if (!dynamic_cast<InstanceNode*>(varDecl->value) && !dynamic_cast<IndexNode*>(varDecl->value)) {
            counter++; 
        }
    }

    if (auto block = dynamic_cast<BlockNode*>(node)) {
        for (auto statement : block->statements) {
            counter += countVarDecl(statement);
        }
    }

    if (auto ifNode = dynamic_cast<IfNode*>(node)) {
        counter += countVarDecl(ifNode->thenBranch);
        if (ifNode->elseBranch != nullptr) {
            counter += countVarDecl(ifNode->elseBranch);
        }
    }

    if (auto whileNode = dynamic_cast<WhileNode*>(node)) {
        counter += countVarDecl(whileNode->body);
    }

    if (auto push = dynamic_cast<PushNode*>(node)) {
        counter += countVarDecl(push->value);
        if (push->secondValue != nullptr) {
            counter += countVarDecl(push->secondValue);
        }
    }

    if (auto remove = dynamic_cast<RemoveNode*>(node)) {
        counter++;
    }

    if (auto mapDecl = dynamic_cast<MapDeclNode*>(node)) {
        int size = evaluateConstant(mapDecl->size);

        if (size < 0) {
            throw std::runtime_error("CG: E50-1 | Map size cannot be negative");
        }

        for (auto key : mapDecl->keys->elements) {
            counter += countVarDecl(key);
        }

        for (auto value : mapDecl->values->elements) {
            counter += countVarDecl(value);
        }

        counter += size * 2;
        counter++;
    }
    
    if (auto idx = dynamic_cast<IndexNode*>(node)) {
        counter += countVarDecl(idx->index);
        counter += 2;
    }
    
    if (auto inst = dynamic_cast<InstanceNode*>(node)) {
        if (inst->arguments.size() == 0) {
        counter += 1;
        } else {
            counter += inst->arguments.size();
        }
        for (auto arg : inst->arguments) {
            counter += countVarDecl(arg);
        }
    }

    if (auto call = dynamic_cast<CallNode*>(node)) {
        for (auto arg : call->arguments) {
            counter += countVarDecl(arg);
        }
    }

    if (auto printNode = dynamic_cast<PrintNode*>(node)) {
        counter += 3;
    }

    if (auto outNode = dynamic_cast<OutNode*>(node)) {
        counter += countVarDecl(outNode->output);
    }

    return counter;
}

void CodeGen::emit(const std::string& line, bool indent) {
    std::string current = (indent ? "\t" : "") + line + "\n";
    output += current;
}

void CodeGen::genMethod(FuncDeclNode* method, const std::string& structName) {
    typeCheck.instances["inst"] = structName;
    std::string name = structName + "_" + method->name.value;
    emit(std::format("jmp .L_skip_{}", name));
    emit(name + ":", false);
    emit("pushq %rbp");
    emit("movq %rsp, %rbp");

    int allocatedBytes = (countVarDecl(method->body) + method->parameters.size() + 1) * 8;

    emit(std::format("subq ${}, %rsp", allocatedBytes));
    funcOffset -= 8;
    symbolTable["inst"] = funcOffset;
    emit(std::format("movq %rcx, {}(%rbp)", funcOffset));

    for (int i = 1; i <= method->parameters.size(); i++) {
        funcOffset-=8;
        symbolTable[method->parameters[i-1].name.value] = funcOffset;
        isReferenceSlot[method->parameters[i-1].name.value] = method->parameters[i-1].isReference;

        switch(i) {
            default: {
                throw std::runtime_error("CG: E66-1 | Cannot have more than three parameters: " + name);
            }
            case 1: {
                emit(std::format("movq %rdx, {}(%rbp)", funcOffset));
                break;
            }
            case 2: {
                emit(std::format("movq %r8, {}(%rbp)", funcOffset));
                break;
            }
            case 3: {
                emit(std::format("movq %r9, {}(%rbp)", funcOffset));
                break;
            }
            }
        }

    int savedOffset = currentOffset;
    currentOffset = funcOffset;

    genNode(method->body);

    funcOffset = 0;
    currentOffset = savedOffset;

    emit("leave");
    emit("ret");
    emit(std::format(".L_skip_{}:", name), false);
}

void CodeGen::genNode(ASTNode* node) {
    if (auto num = dynamic_cast<NumberLiteralNode*>(node)) {
        std::string number = num->value;
        emit(std::format("movq ${}, %rax", number));
    }

    if (auto bin = dynamic_cast<BinaryExprNode*>(node)) {
        genNode(bin->left);
        emit("pushq %rax");
        genNode(bin->right);
        emit("movq %rax, %rbx");
        emit("popq %rax");


        switch(bin->op.type) {
            case TokenType::Plus: {
                emit("addq %rbx, %rax");
                break;
                }
            case TokenType::Minus: {
                emit("subq %rbx, %rax");
                break;
            }
            case TokenType::Star:{
                emit("imulq %rbx, %rax");
                break;
            }
            case TokenType::FSlash: {
                emit("cqto");
                emit("idivq %rbx");
                break;
            }
            case TokenType::EqualEqual: {
                emit("cmpq %rbx, %rax");
                emit("sete %al");
                emit("movzbq %al, %rax");
                break;
            }
            case TokenType::NotEqual: {
                emit("cmpq %rbx, %rax");
                emit("setne %al");
                emit("movzbq %al, %rax");
                break;
            }
            case TokenType::LessThan: {
                emit("cmpq %rbx, %rax");
                emit("setl %al");
                emit("movzbq %al, %rax");
                break;
            }
            case TokenType::LessThanOrEqual: {
                emit("cmpq %rbx, %rax");
                emit("setle %al");
                emit("movzbq %al, %rax");
                break;
            }
            case TokenType::GreaterThan: {
                emit("cmpq %rbx, %rax");
                emit("setg %al");
                emit("movzbq %al, %rax");
                break;
            }
            case TokenType::GreaterThanOrEqual: {
                emit("cmpq %rbx, %rax");
                emit("setge %al");
                emit("movzbq %al, %rax");
                break;
            }
            case TokenType::L_AND: {
                emit("andq %rbx, %rax");
                emit("movzbq %al, %rax");
                break;
            }
            case TokenType::L_OR: {
                emit("orq %rbx, %rax");
                emit("movzbq %al, %rax");
                break;
            }
        }   
    }

    if (auto block = dynamic_cast<BlockNode*>(node)) {
        for (auto statement : block->statements) {
            genNode(statement);
        }
    }

    if (auto arrDecl = dynamic_cast<ArrayDeclNode*>(node)) {
        std::string name = arrDecl->name.value;

        currentOffset -= 8;
        emit(std::format("movq ${}, {}(%rbp)", arrDecl->elements.size(), currentOffset));

        int baseOffset = currentOffset - 8;

        for (size_t i = 0; i < arrDecl->elements.size(); i++) {
            genNode(arrDecl->elements[i]);
            currentOffset -= 8;

            emit(std::format("movq %rax, {}(%rbp)", currentOffset));
        }

        symbolTable[name] = baseOffset;

        emit(std::format("leaq {}(%rbp), %rax", baseOffset)); // load pointer into %rax
    }
    
    else if (auto varDecl = dynamic_cast<VarDeclNode*>(node)) {
        std::string name = varDecl->name.value;

        if (auto inst = dynamic_cast<InstanceNode*>(varDecl->value)) {
            bool first = true;
            int baseOffset = 0;
            if (inst->arguments.size() == 0) {
                    currentOffset -= 8;
                    baseOffset = currentOffset;
            } else {
                for (size_t i = 0; i < inst->arguments.size(); i++) {
                    genNode(inst->arguments[i]);
                    currentOffset -= 8;
                    if (first) {
                        baseOffset = currentOffset;
                        first = false;
                    }
                    emit(std::format("movq %rax, {}(%rbp)", currentOffset));
                }
            }
            symbolTable[name] = baseOffset;
        }
        else {
            genNode(varDecl->value);
            currentOffset -= 8;
            symbolTable[name] = currentOffset;
            emit(std::format("movq %rax, {}(%rbp)", currentOffset));

            if (varDecl->type.type == TokenType::Str || varDecl->type.type == TokenType::Identifier) {
                isReferenceSlot[name] = true;
            }
        }
    }
    
    if (auto idx = dynamic_cast<IndexNode*>(node)) {
        if (typeCheck.symbols.mapExists(idx->name.value)) {
            bool stringKeyed = false;
            if (typeCheck.symbols.mapKeyType(idx->name.value) == TokenType::String) {
                stringKeyed = true;
            }
            genNode(idx->index);
            currentOffset -= 8;
            int keyOffset = currentOffset;
            emit(std::format("movq %rax, {}(%rbp)", keyOffset));
            currentOffset -= 8;
            int counterOffset = currentOffset;
            emit(std::format("movq $0, {}(%rbp)", counterOffset));
            
            int countOffset = symbolTable[idx->name.value + "_count"];
            int keyBaseOffset = symbolTable[idx->name.value + "_keys"];
            int valueBaseOffset = symbolTable[idx->name.value + "_values"];

            int id = uniqueCount++;
            std::string startLabel = std::format("START{}", id);
            std::string endLabel = std::format("END{}", id);
            std::string foundLabel = std::format("FOUND{}", id);
            std::string noMatchLabel = std::format("NOMATCH{}", id);
            std::string doneLabel = std::format("DONE{}", id);

            emit(startLabel + ":", false);

            emit(std::format("movq {}(%rbp), %rcx", counterOffset));
            emit(std::format("movq {}(%rbp), %rdx", countOffset));
            emit("cmpq %rdx, %rcx");
            emit(std::format("jge {}", endLabel));

            if (!stringKeyed) {
                emit("negq %rcx");
                emit(std::format("movq {}(%rbp, %rcx, 8), %rax", keyBaseOffset)); // load index rcx to rax
                emit("negq %rcx");
                emit(std::format("cmpq {}(%rbp), %rax", keyOffset));
                emit(std::format("je {}", foundLabel));
            } else {
                emit("negq %rcx");
                emit(std::format("movq {}(%rbp, %rcx, 8), %rax", keyBaseOffset)); // load index rcx to rax
                emit("negq %rcx");
                emit("movq %rax, %r8"); // save key to r8 for string check
                emit(std::format("movq {}(%rbp), %r9", keyOffset)); // search key address
                emit("movq 8(%r8), %rax");
                emit("cmpq 8(%r9), %rax");
                emit(std::format("jne {}", noMatchLabel));
                emit("movq %rax, %r11"); // load length into r11
                emit("movq $0, %r10"); // initialize string index at 0
                int charId = uniqueCount++;
                std::string charStart = std::format("CHAR_START{}", charId);
                emit(charStart + ":", false);
                emit("cmpq %r11, %r10");
                emit(std::format("jge {}", foundLabel));
                emit("negq %r10");
                emit("movq (%r8, %r10, 8), %rax");
                emit("movq (%r9, %r10, 8), %r13");
                emit("negq %r10");
                emit("cmpq %r13, %rax"); // compare characters
                emit(std::format("jne {}", noMatchLabel));
                emit("incq %r10");
                emit(std::format("jmp {}", charStart));
            }

            emit(noMatchLabel + ":", false);
            emit("incq %rcx");
            emit(std::format("movq %rcx, {}(%rbp)", counterOffset));
            emit(std::format("jmp {}", startLabel));

            emit(foundLabel + ":", false);
            emit("negq %rcx");
            emit(std::format("movq {}(%rbp, %rcx, 8), %rax", valueBaseOffset));
            emit(std::format("jmp {}", doneLabel));

            emit(endLabel + ":", false);
            emit("leaq mapMissMsg(%rip), %rcx");
            emit("jmp runtime_error");

            emit(doneLabel + ":", false);
            
        } else {
            genNode(idx->index);
            emit("negq %rax");
            int baseOffset = symbolTable[idx->name.value];
            if (isReferenceSlot[idx->name.value]){
                emit(std::format("movq {}(%rbp), %rdx", baseOffset));
                emit("movq (%rdx, %rax, 8), %rax");
            } else {
                emit(std::format("movq {}(%rbp, %rax, 8), %rax", baseOffset));
            }
        }
    }

    if (auto field = dynamic_cast<FieldAccessNode*>(node)) {
        auto target = dynamic_cast<IdentifierNode*>(field->target);

        if (!target) {
            throw std::runtime_error("CG: E36-1 | No identifier found for field access");
        }

        int baseOffset = symbolTable[target->value];
        std::vector<Param> fields = typeCheck.structTable[typeCheck.instances[target->value]].fields;

        bool found = false;
        int count = -1;

        for (auto& checkedField : fields) {
            count++;
            if (checkedField.name.value == field->field.value) {
                found = true;
                break;
            }
        }

        if (!found) {
            throw std::runtime_error("CG: E35-1 | Undefined field: " + field->field.value);
        }

        int finalOffset = -count*8;

        if (isReferenceSlot[target->value]) {
            emit(std::format("movq {}(%rbp), %r8", baseOffset));
            emit(std::format("movq {}(%r8), %rax", finalOffset));
        } else {
            emit(std::format("movq {}(%rbp), %rax", baseOffset + finalOffset));
        }
    }

    if (auto push = dynamic_cast<PushNode*>(node)) {
        if (typeCheck.symbols.mapExists(push->arrayName.value)) {
            
            genNode(push->value);
            emit("pushq %rax");
            genNode(push->secondValue);
            emit("pushq %rax");

            int id = uniqueCount++;
            std::string doneLabel = std::format("DONE{}", id);
            std::string mapOverflow = std::format("MAP_OVERFLOW{}", id);

            int countOffset = symbolTable[push->arrayName.value + "_count"];
            int keyBaseOffset = symbolTable[push->arrayName.value + "_keys"];
            int valueBaseOffset = symbolTable[push->arrayName.value + "_values"];

            emit(std::format("movq {}(%rbp), %rdx", countOffset));
            int capacity = mapCapacities[push->arrayName.value];
            emit(std::format("cmpq ${}, %rdx", capacity));
            emit("jge " + mapOverflow);
            
            emit("popq %rax");
            emit("negq %rdx");
            emit(std::format("movq %rax, {}(%rbp, %rdx, 8)", valueBaseOffset));
            emit("popq %rax");
            emit(std::format("movq %rax, {}(%rbp, %rdx, 8)", keyBaseOffset));
            emit("negq %rdx");
            emit("incq %rdx");
            emit(std::format("movq %rdx, {}(%rbp)", countOffset));
            emit(std::format("jmp {}", doneLabel));

            emit(mapOverflow + ":", false);
            emit("leaq mapOverflowMsg(%rip), %rcx");
            emit("jmp runtime_error");
            emit(doneLabel + ":", false);
        } else {
            genNode(push->value);
            emit("pushq %rax");

            int baseOffset = symbolTable[push->arrayName.value];
            int length = baseOffset + 8;

            emit(std::format("movq {}(%rbp), %rax", length));
            emit("negq %rax");
            emit("popq %rbx");
            emit(std::format("movq %rbx, {}(%rbp, %rax, 8) ", baseOffset)); // offset it

            emit(std::format("movq {}(%rbp), %rax", length));
            emit("incq %rax");
            emit(std::format("movq %rax, {}(%rbp)", length)); // write length back to memory
        }
    }

    if (auto remove = dynamic_cast<RemoveNode*>(node)) {
        int lengthOffset = symbolTable[remove->arrayName.value] + 8;
        int baseOffset = lengthOffset - 8;

        genNode(remove->index);

        currentOffset -= 8;
        int counterOffset = currentOffset;

        emit(std::format("movq %rax, {}(%rbp)", currentOffset));

        int id = uniqueCount++;
        
        std::string startLabel = std::format("START{}", id);
        std::string endLabel = std::format("END{}", id);

        emit(startLabel + ":", false);

        emit(std::format("movq {}(%rbp), %rcx", counterOffset));
        emit(std::format("movq {}(%rbp), %rdx", lengthOffset));
        emit("subq $1, %rdx");
        emit("cmpq %rdx, %rcx");

        emit(std::format("jge {}", endLabel));

        emit("negq %rcx");
        emit("movq %rcx, %r10");
        emit("subq $1, %r10");
        emit(std::format("movq {}(%rbp, %r10, 8), %rax", baseOffset)); // read element at index: counter + 1
        emit(std::format("movq %rax, {}(%rbp, %rcx, 8)", baseOffset)); // write current element to index: counter

        emit("negq %rcx");
        emit("incq %rcx"); // counterOffset++
        emit(std::format("movq %rcx, {}(%rbp)", counterOffset)); // write back to memory

        emit(std::format("jmp {}", startLabel));

        emit(endLabel + ":", false);

        emit(std::format("movq {}(%rbp), %rdx", lengthOffset));
        emit("subq $1, %rdx");
        emit(std::format("movq %rdx, {}(%rbp)", lengthOffset));
    }

    if (auto id = dynamic_cast<IdentifierNode*>(node)) {
        auto it = symbolTable.find(id->value);

        if (it == symbolTable.end()) {
            throw std::runtime_error("CG: E58 | Undefined variable: " + id->value);
        }

        int offset = it->second;
        if (isReferenceSlot[id->value]){
            emit(std::format("movq {}(%rbp), %rax", offset));
            emit("movq (%rax), %rax");
        } else {
            emit(std::format("movq {}(%rbp), %rax", offset));
        }
    }

    if (auto assign = dynamic_cast<AssignNode*>(node)) {
        
        if (auto id = dynamic_cast<IdentifierNode*>(assign->target)) {
            genNode(assign->value);
            auto it = symbolTable.find(id->value);

            if (it == symbolTable.end()) {
                throw std::runtime_error("CG: E37 | Cannot assign to an undefined variable: " + id->value);
            }

            if (isReferenceSlot[id->value]){
                emit(std::format("movq {}(%rbp), %r8", it->second));
                emit("movq %rax, (%r8)");
            } else {
                emit(std::format("movq %rax, {}(%rbp)", it->second));
            }
        }

        else if (auto field = dynamic_cast<FieldAccessNode*>(assign->target)) {
            genNode(assign->value);

            auto target = dynamic_cast<IdentifierNode*>(field->target);

            if (!target) {
                throw std::runtime_error("CG: E36-2 | No identifier found for field access");
            }

            int baseOffset = symbolTable[target->value];
            std::vector<Param> fields = typeCheck.structTable[typeCheck.instances[target->value]].fields;

            bool found = false;
            int count = -1;

            for (auto& checkedField : fields) {
                count++;
                if (checkedField.name.value == field->field.value) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                throw std::runtime_error("CG: E35-2 | Undefined field: " + target->value);
            }

            int finalOffset = -count * 8;

            if (isReferenceSlot[target->value]) {
                emit(std::format("movq {}(%rbp), %r8", baseOffset));
                emit(std::format("movq %rax, {}(%r8)", finalOffset));
            } else {
                emit(std::format("movq %rax, {}(%rbp)", baseOffset + finalOffset));
            }
        }

        else if (auto idx = dynamic_cast<IndexNode*>(assign->target)) {
            genNode(assign->value);
            emit("pushq %rax");

            genNode(idx->index);
            emit("negq %rax");
            emit("movq %rax, %rbx");

            emit("popq %rax");

            int baseOffset = symbolTable[idx->name.value];
            emit(std::format("movq %rax, {}(%rbp,%rbx,8)", baseOffset));
        }

    }

    if (auto inst = dynamic_cast<InstanceNode*>(node)) {
        bool first = true;
        int baseOffset = 0;
        
        if (inst->arguments.size() == 0) {
            currentOffset -= 8;
            baseOffset = currentOffset;
        } else {
            for (size_t i = 0; i < inst->arguments.size(); i++) {
                genNode(inst->arguments[i]);
                currentOffset -= 8;
                if (first) {
                    baseOffset = currentOffset;
                    first = false;
                }
                emit(std::format("movq %rax, {}(%rbp)", currentOffset));
            }
        }

        emit(std::format("leaq {}(%rbp), %rax", baseOffset));
    }

    if (auto ifNode = dynamic_cast<IfNode*>(node)) {
        int id = ifCounter++;

        std::string elseLabel = std::format(".L_else{}", id);
        std::string doneLabel = std::format(".L_done{}", id);

        genNode(ifNode->condition);

        emit("cmpq $0, %rax");

        std::string targetLabel = (ifNode->elseBranch != nullptr) ? elseLabel : doneLabel;
        emit(std::format("je {}", targetLabel));

        genNode(ifNode->thenBranch);

        if (ifNode->elseBranch != nullptr) {

            emit(std::format("jmp {}", doneLabel));

            emit(std::format("{}:", elseLabel), false);

            genNode(ifNode->elseBranch);
        }

        emit(std::format("{}:", doneLabel));
    }

    if (auto whileNode = dynamic_cast<WhileNode*>(node)) {
        int id = whileCounter++;
        std::string startLabel = std::format("START{}", id);
        std::string endLabel = std::format("END{}", id);

        emit(startLabel + ":", false);

        genNode(whileNode->condition);

        emit("cmpq $0, %rax");
        emit(std::format("je {}", endLabel));

        genNode(whileNode->body);

        emit(std::format("jmp {}", startLabel));

        emit(endLabel + ":", false);
    }

    if (auto funcDecl = dynamic_cast<FuncDeclNode*>(node)) {
        emit(std::format("jmp .L_skip_{}", funcDecl->name.value));
        emit(funcDecl->name.value + ":", false);
        emit("pushq %rbp");
        emit("movq %rsp, %rbp");

        int allocatedBytes = (countVarDecl(funcDecl->body) + funcDecl->parameters.size()) * 8;

        emit(std::format("subq ${}, %rsp", allocatedBytes));

        for (int i = 1; i <= funcDecl->parameters.size(); i++) {
            funcOffset-=8;
            symbolTable[funcDecl->parameters[i-1].name.value] = funcOffset;
            isReferenceSlot[funcDecl->parameters[i-1].name.value] = funcDecl->parameters[i-1].isReference;
            switch(i) {
                default: {
                    throw std::runtime_error("CG: E65-1 | Cannot have more than four parameters: " + funcDecl->name.value);
                }
                case 1: {
                    emit(std::format("movq %rcx, {}(%rbp)", funcOffset));
                    break;
                }
                case 2: {
                    emit(std::format("movq %rdx, {}(%rbp)", funcOffset));
                    break;
                }
                case 3: {
                    emit(std::format("movq %r8, {}(%rbp)", funcOffset));
                    break;
                }
                case 4: {
                    emit(std::format("movq %r9, {}(%rbp)", funcOffset));
                    break;
                }
                }
            }

        int savedOffset = currentOffset;
        currentOffset = funcOffset;

        genNode(funcDecl->body);

        funcOffset = 0;
        currentOffset = savedOffset;

        emit("leave");
        emit("ret");
        emit(std::format(".L_skip_{}:", funcDecl->name.value), false);
    }

    if (auto mapDecl = dynamic_cast<MapDeclNode*>(node)) {
        int size = evaluateConstant(mapDecl->size);
        mapCapacities[mapDecl->name.value] = size;
        size_t mapSize = mapDecl->keys->elements.size();
        std::string name = mapDecl->name.value;

        currentOffset -= 8;
        emit(std::format("movq ${}, {}(%rbp)", mapSize, currentOffset));
        symbolTable[name + "_count"] = currentOffset;
        
        int keyBaseOffset = currentOffset - 8;
        for (int i = 0; i < size; i++) {
            currentOffset -= 8;
        }

        int valueBaseOffset = currentOffset - 8;
        for (int i = 0; i < size; i++) {
            currentOffset -= 8;
        }

        symbolTable[name + "_keys"] = keyBaseOffset;
        symbolTable[name + "_values"] = valueBaseOffset;

        for (size_t i = 0; i < mapSize; i++) {
            genNode(mapDecl->keys->elements[i]);
            int offset = keyBaseOffset - i * 8;
            emit(std::format("movq %rax, {}(%rbp)", offset));
        }

        for (size_t i = 0; i < mapSize; i++) {
            genNode(mapDecl->values->elements[i]);
            int offset = valueBaseOffset - i * 8;
            emit(std::format("movq %rax, {}(%rbp)", offset));
        }
    }

    if (auto structDecl = dynamic_cast<StructDeclNode*>(node)) {
        for (auto& methodPair : structDecl->methods) {
            if (auto method = dynamic_cast<FuncDeclNode*>(methodPair.second)) {
                genMethod(method, structDecl->name.value);
            }
        }
    }
    
    if (auto method = dynamic_cast<MethodNode*>(node)) {
        int instOffset = symbolTable[method->targetName.value];
        emit(std::format("leaq {}(%rbp), %rcx", instOffset));

        for (int i = 0; i < method->arguments.size(); i++) {
            genNode(method->arguments[i]);
            switch(i + 1) {
                default:
                    throw std::runtime_error("CG: E66-2 | Cannot have more than three arguments: " + method->methodName.value);
                case 1: {
                    emit("movq %rax, %rdx");
                    break;
                }
                case 2: {
                    emit("movq %rax, %r8");
                    break;
                }
                case 3: {
                    emit("movq %rax, %r9");
                    break;
                }
            }
        }
        
        std::string structName = typeCheck.instances[method->targetName.value];
        emit("subq $32, %rsp");
        emit(std::format("call {}_{}", structName, method->methodName.value));
        emit("addq $32, %rsp");
    }

    if (auto callNode = dynamic_cast<CallNode*>(node)) {
        for (int i = 0; i < callNode->arguments.size(); i++) {
            if (functions[callNode->name.value].isReference[i]) {
                auto id = dynamic_cast<IdentifierNode*>(callNode->arguments[i]);
                int offset = symbolTable[id->value];
                emit(std::format("leaq {}(%rbp), %rax", offset));
            } else {
                genNode(callNode->arguments[i]);
            }
            switch(i + 1) {
                default:
                    throw std::runtime_error("CG: E65-2 | Cannot have more than four arguments: " + callNode->name.value);
                case 1: {
                    emit("movq %rax, %rcx");
                    break;
                }
                case 2: {
                    emit("movq %rax, %rdx");
                    break;
                }
                case 3: {
                    emit("movq %rax, %r8");
                    break;
                }
                case 4: {
                    emit("movq %rax, %r9");
                    break;
                }
            }
        }
        
        emit("subq $32, %rsp");
        emit(std::format("call {}", callNode->name.value));
        emit("addq $32, %rsp");
    }

    if (auto outNode = dynamic_cast<OutNode*>(node)) {
        genNode(outNode->output);

        emit("leave");
        emit("ret");
    }

    if (auto size = dynamic_cast<SizeNode*>(node)) {
        if (typeCheck.symbols.mapExists(size->name.value)) {
            int lengthSlot = symbolTable[size->name.value + "_count"];
            emit(std::format("movq {}(%rbp), %rax", lengthSlot));
        } else {
            int offset = symbolTable[size->name.value];
            if (isReferenceSlot[size->name.value]) {
                emit(std::format("movq {}(%rbp), %rax", offset));
                emit("movq 8(%rax), %rax");
            } else {
                emit(std::format("movq {}(%rbp), %rax", offset + 8));
            }
        }
    }

    if (auto printNode = dynamic_cast<PrintNode*>(node)) {
        if (typeCheck.TypeCheck(printNode->value) == TokenType::String) {
            genNode(printNode->value);
            emit("movq %rax, %r8"); // base
            currentOffset -= 8;
            int baseSlot = currentOffset;
            emit(std::format("movq %r8, {}(%rbp)", baseSlot));

            emit("movq 8(%r8), %r9"); // length
            currentOffset -= 8;
            int lengthSlot = currentOffset;
            emit(std::format("movq %r9, {}(%rbp)", lengthSlot));

            currentOffset -= 8;
            int indexSlot = currentOffset;
            emit(std::format("movq $0, {}(%rbp)", indexSlot));

            int id = uniqueCount++;
            std::string printStart= std::format("PRINT_START{}", id);
            std::string printEnd = std::format("PRINT_END{}", id);

            emit(printStart + ":", false);
            emit(std::format("movq {}(%rbp), %r9", lengthSlot));
            emit(std::format("movq {}(%rbp), %r10", indexSlot));
            emit("cmpq %r9, %r10");
            emit(std::format("jge {}", printEnd));

            emit("negq %r10");
            emit(std::format("movq {}(%rbp), %r8", baseSlot));
            emit("movq (%r8, %r10, 8), %rdx");
            emit("negq %r10");

            emit("leaq charFmt(%rip), %rcx");
            emit("subq $32, %rsp");
            emit("call printf");
            emit("addq $32, %rsp");

            emit(std::format("addq $1, {}(%rbp)", indexSlot));
            emit(std::format("jmp {}", printStart));
            emit(printEnd + ":", false);
        } else {
            genNode(printNode->value);
            emit("movq %rax, %rdx");
            emit("leaq fmt(%rip), %rcx");
            emit("subq $32, %rsp");
            emit("call printf");
            emit("addq $32, %rsp");
        }
    }

}

std::string CodeGen::generate(ASTNode* root) {
    emit(".data", false);
    emit("fmt: .string \"%d\\n\"", false);
    emit("charFmt: .string \"%c\"", false);
    emit("strFmt: .string \"%s\\n\"", false);
    emit("mapMissMsg: .string \"Error: key not found in map\\n\"", false);
    emit("mapOverflowMsg: .string \"Error: map overflow\\n\"", false);
    emit(".text", false);
    emit(".global main", false);
    emit(".def main; .scl 2; .type 32; .endef", false);
    emit("main:", false);

    int allocatedBytes = countVarDecl(root) * 8;
    allocatedBytes = (allocatedBytes  + 15) & ~15;
    emit("pushq %rbp");
    emit("movq %rsp, %rbp");
    emit(std::format("subq ${}, %rsp", allocatedBytes));
    genNode(root);
    emit("leave");
    emit("ret");

    emit("runtime_error:", false);
    emit("subq $32, %rsp");
    emit("call printf");
    emit("movq $1, %rcx");
    emit("call exit");

    return output;
}