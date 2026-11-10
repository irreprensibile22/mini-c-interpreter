#ifndef TOKEN_H
#define TOKEN_H

#include <string>

enum TokenType {
    // keywords
    IF, ELSE, WHILE, DO, BREAK, INT, BOOLEAN, TRUE, FALSE, PRINT,
    ID,
    NUM,
    PLUS, MINUS, TIMES, DIVIDE,
    EQ, NEQ, LT, LEQ, GT, GEQ,
    AND, OR, NOT,

    ASSIGN, SEMICOLON,
    LPAREN, RPAREN, LBRACE, RBRACE,
    LBRACKET, RBRACKET,

    END_OF_FILE
};

// A lexical unit plus where it starts in the source (for error messages).
class Token {
public:
    TokenType type = END_OF_FILE;
    std::string value;
    int line = 0;
    int column = 0;

    Token(TokenType type, std::string value, int line, int column);
    Token() = default;
};

#endif
