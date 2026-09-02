#pragma once


inline uint64_t evaluateConstant(ASTNode* node) {
    if (!dynamic_cast<NumberLiteralNode*>(node) && !dynamic_cast<BinaryExprNode*>(node)) {
        throw std::runtime_error("CG: E48 | Invalid expression");
    }
    
    if (auto num = dynamic_cast<NumberLiteralNode*>(node)) {
        return std::stoll(num->value);
    }

    if (auto bin = dynamic_cast<BinaryExprNode*>(node)) {
        int left = evaluateConstant(bin->left);
        int right = evaluateConstant(bin->right);

        switch(bin->op.type) {
            case TokenType::Plus:
                return left + right;
            case TokenType::Minus:
                return left - right;
            case TokenType::Star:
                return left * right;
            case TokenType::FSlash:
                return left / right;
        }
    }

    throw std::runtime_error("CG: E49 | Cannot use comparison or boolean operators");
}