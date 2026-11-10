#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "parser.h"
#include <map>
#include <string>
#include <vector>
#include <exception>

// Thrown by `break` and caught by the innermost enclosing loop.
struct BreakException : public std::exception {
    const char* what() const noexcept override {
        return "break";
    }
};

// Walks the AST and runs it. Booleans are stored as 0/1 ints.
class Evaluator {
public:
    Evaluator();
    int evaluate(ASTNode* node);

private:
    struct Array {
        std::vector<int> dims;
        std::vector<int> cells;  // row-major, size = product of dims
    };

    // One global memory space: a variable is visible anywhere after its declaration.
    std::map<std::string, int> variables;
    std::map<std::string, Array> arrays;

    int& cell(ASTNode* loc);
};

#endif
