#include <iostream>
#include <fstream>
#include <filesystem>
#include "lexer.h"
#include "ast.h"
#include "parser.h"
#include "type.h"
#include "codegen.h"
#include "winpath.h"


int main(int argc, char* argv[]) {
    try {
        std::filesystem::path filePath = "C:/Projects/O Compiler/a.ol";
        std::string source = readFile(filePath.string());

        bool showTokens = false;
        bool showAst = false;
        for (int i = 1; i < argc; i++) {
            std::string arg = argv[i];
            if (arg == "-d" && i + 1 < argc) {
                std::string next = argv[i + 1];
                if (next == "tokens") showTokens = true;
                if (next == "ast") showAst = true;
            }
        }

        
        Lexer lexer(source);
        std::vector<Token> tokens = lexer.tokenize();

        if (showTokens) {
            for (const auto& token : tokens) {
                std::cout << "Type: " << tokenTypeName(token.type) << " | Lexeme: \"" << token.value << "\" | Line: " << token.line << std::endl;
            }
        }

        Parser parser(tokens, filePath.parent_path());

        ASTNode* root = parser.parse();
        if (showAst) {
            printAST(root);
        }
        
        TypeChecker checker;
        
        TokenType resultType = checker.TypeCheck(root);

        CodeGen codegen(checker);

        std::string assembly = codegen.generate(root); 

        std::ofstream out("output.s");
        out << assembly;
        out.close();

        std::cout << "Successfully compiled." << std::endl;

        std::string toolchainDir = getExecutableDirectory() + "\\toolchain";

        std::string asCmd = "\"" + toolchainDir + "\\as.exe\" -o temp.o output.s";
        std::string wrappedasCmd = "\"" + asCmd + "\"";
        int asResult = std::system(wrappedasCmd.c_str());

        if (asResult != 0) {
            std::cerr << "M: E59 | Assembler failed." << std::endl;
            return 1;
        }

        std::string linkCmd = "\"" + toolchainDir + "\\collect2.exe\" -m i386pep -Bdynamic -o output.exe "
            "-L\"" + toolchainDir + "\" "
            "\"" + toolchainDir + "\\crt2.o\" "
            "\"" + toolchainDir + "\\crtbegin.o\" "
            "temp.o -lmingw32 -lgcc -lgcc_eh -lmingwex -lmsvcrt -lkernel32 "
            "\"" + toolchainDir + "\\default-manifest.o\" "
            "\"" + toolchainDir + "\\crtend.o\"";

        std::string wrappedLinkCmd = "\"" + linkCmd + "\"";
        int linkResult = std::system(wrappedLinkCmd.c_str());

        if (linkResult != 0) {
            std::cerr << "M: E60 | Linker failed." << std::endl;
            return 1;
        }
        
        std::string prompt;

        while (true) {
            std::cout << "> " << std::flush;
            std::getline(std::cin, prompt);

            if (prompt == "-d assembly") {
                std::cout << assembly << std::endl;
            }
            else if (prompt == "-o exit") {
                break;
            } else {
                std::cout << "Unknown command: " << prompt << std::endl;
            }
        }
}
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}