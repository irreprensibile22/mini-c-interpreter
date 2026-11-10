// Build: g++ -std=c++17 -o interpreter main.cpp token.cpp tokenizer.cpp parser.cpp interpreter.cpp
// Run:   ./interpreter source.txt
#include <iostream>
#include <fstream>
#include <string>
#include "tokenizer.h"
#include "parser.h"
#include "interpreter.h"

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cout << "Error: usage " << argv[0] << " <source_file>" << std::endl;
        return 1;
    }

    std::string fileName = argv[1];
    std::ifstream file(fileName);
    if (!file.is_open()) {
        std::cout << "Error: cannot open file " << fileName << std::endl;
        return 1;
    }

    try {
        Tokenizer tokenizer(file);
        Parser parser(tokenizer);

        // The whole program is parsed and type-checked before it runs,
        // so a syntax error never produces partial output.
        ASTNode* root = parser.parseProgram();

        Evaluator evaluator;
        evaluator.evaluate(root);

        delete root;
    } catch (const std::exception& e) {
        // Errors are reported as a single line starting with "Error: ".
        std::cout << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
