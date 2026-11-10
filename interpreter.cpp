#include "interpreter.h"
#include <iostream>
#include <stdexcept>
#include <algorithm>

Evaluator::Evaluator() {}

// Resolves a LOC chain like a[i][j] to the storage cell it refers to,
// checking every index against its dimension.
int& Evaluator::cell(ASTNode* loc) {
    // The parser nests a[i][j] as LOC(LOC(ID a, i), j), so indices come out last-first.
    std::vector<int> indices;
    ASTNode* node = loc;
    while (node->type == ASTNode::LOC) {
        indices.push_back(evaluate(node->children[1]));
        node = node->children[0];
    }
    std::reverse(indices.begin(), indices.end());

    Array& array = arrays.at(node->value);
    size_t offset = 0;
    for (size_t k = 0; k < indices.size(); ++k) {
        if (indices[k] < 0 || indices[k] >= array.dims[k]) {
            throw std::runtime_error("index " + std::to_string(indices[k]) + " out of bounds for '" +
                node->value + "' (size " + std::to_string(array.dims[k]) + ")");
        }
        offset = offset * array.dims[k] + indices[k];
    }
    return array.cells[offset];
}

int Evaluator::evaluate(ASTNode* node) {
    if (!node) {
        return 0;
    }

    switch (node->type) {
        case ASTNode::NUM:
            return std::stoi(node->value);

        case ASTNode::TRUE:
            return 1;

        case ASTNode::FALSE:
            return 0;

        case ASTNode::ID:
            return variables.at(node->value);

        case ASTNode::LOC:
            return cell(node);

        case ASTNode::ASSIGN: {
            // Evaluate the right-hand side first, then the target's indices.
            int value = evaluate(node->children[1]);
            ASTNode* target = node->children[0];
            if (target->type == ASTNode::ID) {
                variables[target->value] = value;
            } else {
                cell(target) = value;
            }
            return 0;
        }

        case ASTNode::PLUS:
            return evaluate(node->children[0]) + evaluate(node->children[1]);

        case ASTNode::MINUS:
            return evaluate(node->children[0]) - evaluate(node->children[1]);

        case ASTNode::UNARY_MINUS:
            return -evaluate(node->children[0]);

        case ASTNode::TIMES:
            return evaluate(node->children[0]) * evaluate(node->children[1]);

        case ASTNode::DIVIDE: {
            int left = evaluate(node->children[0]);
            int right = evaluate(node->children[1]);
            if (right == 0) {
                throw std::runtime_error("division by zero");
            }
            return left / right;
        }

        // && and || short-circuit: the right side is skipped when the left decides the result.
        case ASTNode::AND:
            return evaluate(node->children[0]) && evaluate(node->children[1]) ? 1 : 0;

        case ASTNode::OR:
            return evaluate(node->children[0]) || evaluate(node->children[1]) ? 1 : 0;

        case ASTNode::NOT:
            return evaluate(node->children[0]) ? 0 : 1;

        case ASTNode::EQ:
            return evaluate(node->children[0]) == evaluate(node->children[1]) ? 1 : 0;

        case ASTNode::NEQ:
            return evaluate(node->children[0]) != evaluate(node->children[1]) ? 1 : 0;

        case ASTNode::LT:
            return evaluate(node->children[0]) < evaluate(node->children[1]) ? 1 : 0;

        case ASTNode::LEQ:
            return evaluate(node->children[0]) <= evaluate(node->children[1]) ? 1 : 0;

        case ASTNode::GT:
            return evaluate(node->children[0]) > evaluate(node->children[1]) ? 1 : 0;

        case ASTNode::GEQ:
            return evaluate(node->children[0]) >= evaluate(node->children[1]) ? 1 : 0;

        case ASTNode::IF:
            if (evaluate(node->children[0])) {
                evaluate(node->children[1]);
            } else if (node->children.size() > 2) {
                evaluate(node->children[2]);
            }
            return 0;

        case ASTNode::WHILE:
            while (evaluate(node->children[0])) {
                try {
                    evaluate(node->children[1]);
                } catch (const BreakException&) {
                    break;
                }
            }
            return 0;

        case ASTNode::DO:
            do {
                try {
                    evaluate(node->children[0]);
                } catch (const BreakException&) {
                    break;
                }
            } while (evaluate(node->children[1]));
            return 0;

        case ASTNode::BREAK:
            throw BreakException();

        case ASTNode::PRINT:
            std::cout << evaluate(node->children[0]) << std::endl;
            return 0;

        case ASTNode::BLOCK:
            evaluate(node->children[0]);  // decls
            evaluate(node->children[1]);  // stmts
            return 0;

        case ASTNode::DECLS:
        case ASTNode::STMTS:
            for (ASTNode* child : node->children) {
                evaluate(child);
            }
            return 0;

        case ASTNode::DECL: {
            // A declaration inside a loop runs many times; only the first one allocates,
            // so the variable keeps its value across iterations.
            std::string id = node->children[1]->value;
            ASTNode* type = node->children[0];
            if (type->type == ASTNode::BASIC) {
                variables.emplace(id, 0);
            } else if (!arrays.count(id)) {
                // Walk TYPE(TYPE(BASIC, 2), 3) down to the base, collecting sizes in reverse.
                Array array;
                size_t total = 1;
                while (type->type == ASTNode::TYPE) {
                    int size = std::stoi(type->children[1]->value);
                    array.dims.push_back(size);
                    total *= size;
                    type = type->children[0];
                }
                std::reverse(array.dims.begin(), array.dims.end());
                array.cells.assign(total, 0);
                arrays[id] = array;
            }
            return 0;
        }

        default:
            throw std::runtime_error("internal error: unhandled AST node");
    }
}
