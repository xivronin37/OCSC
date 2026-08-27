#include <iostream>
#include <fstream>
#include <filesystem>
#include "lexer.h"
#include "ast.h"
#include "parser.h"
#include "type.h"
#include "codegen.h"

int main() {
    try {
        std::filesystem::path filePath = "C:/Projects/O Compiler/a.ol";
        std::string source = readFile(filePath.string());
        
        Lexer lexer(source);
        std::vector<Token> tokens = lexer.tokenize();

        for (auto token : tokens) {
            std::cout << "Type: " << tokenTypeName(token.type) << " | Lexeme: " << token.value << " | Line:" << token.line << std::endl;
        }

        Parser parser(tokens, filePath.parent_path());

        ASTNode* root = parser.parse();
        
        printAST(root);
        
        TypeChecker checker;
        
        TokenType resultType = checker.TypeCheck(root);

        std::cout << "TypeCheck succeeded. Result type: " << tokenTypeName(resultType) << "\n";

        CodeGen codegen(checker);

        std::string assembly = codegen.generate(root); 

        std::ofstream out("output.s");
        out << assembly;
        out.close();

}
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    
    return 0;
}