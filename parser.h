#ifndef PARSER_H
#define PARSER_H

#include "tokenizer.h"
#include <vector>
#include <string>
#include <unordered_map>

// Generic AST node. `valuetype` is filled in by the parser during type checking.
class ASTNode {
public:
    enum ValueType {
        INT_TYPE,
        BOOL_TYPE,
        VOID_TYPE
    };

    enum NodeType {
        BLOCK, DECLS, DECL, TYPE, BASIC, STMTS,     // structure and declarations
        IF, WHILE, DO, BREAK, PRINT, ASSIGN,        // statements
        LOC, ID, NUM, TRUE, FALSE,                  // locations and constants
        AND, OR, NOT, EQ, NEQ, LT, LEQ, GT, GEQ,    // logical and relational operators
        PLUS, MINUS, TIMES, DIVIDE, UNARY_MINUS     // arithmetic operators
    };
    
    NodeType type;
    std::string value;
    ValueType valuetype;
    std::vector<ASTNode*> children;

    ASTNode(NodeType type, std::string value = "", ValueType valuetype = VOID_TYPE)
        : type(type), value(value), valuetype(valuetype) {}
    ~ASTNode();
};

struct Symbol {
    ASTNode::ValueType baseType;
    std::vector<int> dims;  // empty for scalars, e.g. {2, 3} for int[2][3]

    bool isArray() const { return !dims.empty(); }
};

// Recursive-descent parser. Builds the AST and does all static checks
// (declarations, types, break placement) before anything runs.
class Parser {
public:
    Parser(Tokenizer& tokenizer);

    ASTNode* parseProgram();

private:
    Tokenizer& tokenizer;
    Token currentToken;
    std::unordered_map<std::string, Symbol> symbolTable;
    int loopDepth;  // > 0 while parsing a loop body, so we can reject stray breaks

    void consume(TokenType expectedType);
    ASTNode* parseBlock();
    ASTNode* parseDecls();
    ASTNode* parseDecl();
    ASTNode* parseStmts();
    ASTNode* parseStmt();
    ASTNode* parseLoc();
    ASTNode* parseBool();
    ASTNode* parseJoin();
    ASTNode* parseEquality();
    ASTNode* parseRel();
    ASTNode* parseExpr();
    ASTNode* parseTerm();
    ASTNode* parseUnary();
    ASTNode* parseFactor();
    std::string tokenTypeToString(TokenType type);
    std::string where() const;
};

#endif
