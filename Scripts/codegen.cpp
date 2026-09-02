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
        if (auto inst = dynamic_cast<InstanceNode*>(varDecl->value)) {
            counter += countVarDecl(inst);
        }
        else {
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

        counter += size * 2;
        counter++;
    }
    
    if (auto idx = dynamic_cast<IndexNode*>(node)) {
        counter += countVarDecl(idx->index);
    }
    
    if (auto inst = dynamic_cast<InstanceNode*>(node)) {
        for (auto arg : inst->arguments) {
            counter += countVarDecl(arg);
        }
    }

    if (auto call = dynamic_cast<CallNode*>(node)) {
        for (auto arg : call->arguments) {
            counter += countVarDecl(arg);
        }
    }

    return counter;
}

void CodeGen::emit(const std::string& line, bool indent) {
    std::string current = (indent ? "\t" : "") + line + "\n";
    output += current;
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
            int baseOffset = currentOffset - 8;

            for (size_t i = 0; i < inst->arguments.size(); i++) {
                genNode(inst->arguments[i]);

                currentOffset -= 8;
                emit(std::format("movq %rax, {}(%rbp)", currentOffset));
            }

            symbolTable[name] = baseOffset;
        }
        else {
            genNode(varDecl->value);
            currentOffset -= 8;
            symbolTable[name] = currentOffset;
            emit(std::format("movq %rax, {}(%rbp)", currentOffset));
        }
    }
    
    if (auto idx = dynamic_cast<IndexNode*>(node)) {
        if (typeCheck.symbols.mapExists(idx->name.value)) {
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
            std::string doneLabel = std::format("DONE{}", id);

            emit(startLabel + ":", false);

            emit(std::format("movq {}(%rbp), %rcx", counterOffset));
            emit(std::format("movq {}(%rbp), %rdx", countOffset));
            emit("cmpq %rdx, %rcx");
            emit(std::format("jge {}", endLabel));

            emit("negq %rcx");
            emit(std::format("movq {}(%rbp, %rcx, 8), %rax", keyBaseOffset)); // load index rcx to rax
            emit("negq %rcx");
            emit(std::format("cmpq {}(%rbp), %rax", keyOffset));
            emit(std::format("je {}", foundLabel));
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
            emit(std::format("movq {}(%rbp, %rax, 8), %rax", baseOffset));
        }
    }

    if (auto field = dynamic_cast<FieldAccessNode*>(node)) {
        auto target = dynamic_cast<IdentifierNode*>(field->target);

        if (!target) {
            throw std::runtime_error("CG: E36-1 | No identifier found for field access");
        }

        int baseOffset = symbolTable[target->value];
        std::vector<Param> fields = typeCheck.structTable[typeCheck.instances[target->value]];

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

        int finalOffset = baseOffset - count*8;

        emit(std::format("movq {}(%rbp), %rax", finalOffset));
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
            throw std::runtime_error("Undefined variable: " + id->value);
        }

        int offset = it->second;

        emit(std::format("movq {}(%rbp), %rax", offset));
    }

    if (auto assign = dynamic_cast<AssignNode*>(node)) {
        
        if (auto id = dynamic_cast<IdentifierNode*>(assign->target)) {
            genNode(assign->value);
            auto it = symbolTable.find(id->value);

            if (it == symbolTable.end()) {
                throw std::runtime_error("CG: E37 | Cannot assign to an undefined variable: " + id->value);
            }

            emit(std::format("movq %rax, {}(%rbp)", it->second));
        }

        else if (auto field = dynamic_cast<FieldAccessNode*>(assign->target)) {
            genNode(assign->value);

            auto target = dynamic_cast<IdentifierNode*>(field->target);

            if (!target) {
                throw std::runtime_error("CG: E36-2 | No identifier found for field access");
            }

            int baseOffset = symbolTable[target->value];
            std::vector<Param> fields = typeCheck.structTable[typeCheck.instances[target->value]];

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

            int finalOffset = baseOffset - count*8;

            emit(std::format("movq %rax, {}(%rbp)", finalOffset));
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
            switch(i) {
                default: {
                    throw std::runtime_error("Cannot have more than four parameters: " + funcDecl->name.value);
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


        genNode(funcDecl->body);

        funcOffset = 0;

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

    if (auto callNode = dynamic_cast<CallNode*>(node)) {
        for (int i = 0; i < callNode->arguments.size(); i++) {
            genNode(callNode->arguments[i]);
            switch(i + 1) {
                default:
                    throw std::runtime_error("Cannot have more than six arguments: " + callNode->name.value);
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

    if (auto printNode = dynamic_cast<PrintNode*>(node)) {
        if (auto arr = dynamic_cast<ArrayDeclNode*>(printNode->value)) {
            if (arr->isImmutable) {
                std::string fullString = "";
                for (auto element :arr->elements) {
                    NumberLiteralNode* ascii = dynamic_cast<NumberLiteralNode*>(element);
                    int asciiValue = std::stoi(ascii->value);
                    fullString += static_cast<char>(asciiValue);
                }
                std::string strName = std::format("printStr{}", uniqueCount++);
                emit(".data", false);
                emit(std::format("{}: .string \"{}\"", strName, fullString), false);
                emit(".text", false);

                emit(std::format("leaq {}(%rip), %rdx", strName));
                emit("leaq strFmt(%rip), %rcx");
                emit("subq $32, %rsp");
                emit("call printf");
                emit("addq $32, %rsp");
            }
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