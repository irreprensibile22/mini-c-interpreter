#include "tokenizer.h"
#include <cctype>
#include <stdexcept>
#include <string>

Tokenizer::Tokenizer(std::istream& input) : input(input), currentChar(0), line(1), column(0) {
    readNextChar();
}

static std::string position(int line, int column) {
    return "at line " + std::to_string(line) + ", column " + std::to_string(column);
}

Token Tokenizer::getNextToken() {
    skipWhitespace();

    // Every token is reported at the position of its first character.
    int startLine = line;
    int startColumn = column;

    switch (currentChar) {
        case '+':
            readNextChar();
            return Token(PLUS, "+", startLine, startColumn);
        case '-':
            readNextChar();
            return Token(MINUS, "-", startLine, startColumn);
        case '*':
            readNextChar();
            return Token(TIMES, "*", startLine, startColumn);
        case '/':
            readNextChar();
            if (currentChar == '/') {
                // Line comment: skip to the end of the line.
                while (currentChar != '\n' && currentChar != EOF) {
                    readNextChar();
                }
                return getNextToken();
            }
            return Token(DIVIDE, "/", startLine, startColumn);
        case '=':
            readNextChar();
            if (currentChar == '=') {
                readNextChar();
                return Token(EQ, "==", startLine, startColumn);
            }
            return Token(ASSIGN, "=", startLine, startColumn);
        case '!':
            readNextChar();
            if (currentChar == '=') {
                readNextChar();
                return Token(NEQ, "!=", startLine, startColumn);
            }
            return Token(NOT, "!", startLine, startColumn);
        case '<':
            readNextChar();
            if (currentChar == '=') {
                readNextChar();
                return Token(LEQ, "<=", startLine, startColumn);
            }
            return Token(LT, "<", startLine, startColumn);
        case '>':
            readNextChar();
            if (currentChar == '=') {
                readNextChar();
                return Token(GEQ, ">=", startLine, startColumn);
            }
            return Token(GT, ">", startLine, startColumn);
        case '&':
            readNextChar();
            if (currentChar == '&') {
                readNextChar();
                return Token(AND, "&&", startLine, startColumn);
            }
            throw std::runtime_error("invalid character '&' " + position(startLine, startColumn) + " (did you mean '&&'?)");
        case '|':
            readNextChar();
            if (currentChar == '|') {
                readNextChar();
                return Token(OR, "||", startLine, startColumn);
            }
            throw std::runtime_error("invalid character '|' " + position(startLine, startColumn) + " (did you mean '||'?)");
        case ';':
            readNextChar();
            return Token(SEMICOLON, ";", startLine, startColumn);
        case '(':
            readNextChar();
            return Token(LPAREN, "(", startLine, startColumn);
        case ')':
            readNextChar();
            return Token(RPAREN, ")", startLine, startColumn);
        case '{':
            readNextChar();
            return Token(LBRACE, "{", startLine, startColumn);
        case '}':
            readNextChar();
            return Token(RBRACE, "}", startLine, startColumn);
        case '[':
            readNextChar();
            return Token(LBRACKET, "[", startLine, startColumn);
        case ']':
            readNextChar();
            return Token(RBRACKET, "]", startLine, startColumn);
        default:
            if (std::isalpha(currentChar)) {
                return scanIdentifier(startLine, startColumn);
            } else if (std::isdigit(currentChar)) {
                return scanNumber(startLine, startColumn);
            } else if (currentChar == EOF) {
                return Token(END_OF_FILE, "", startLine, startColumn);
            } else if (!std::isprint(currentChar)) {
                throw std::runtime_error("invalid character " + position(startLine, startColumn));
            } else {
                throw std::runtime_error("invalid character '" + std::string(1, (char)currentChar) + "' " +
                    position(startLine, startColumn));
            }
    }
}

void Tokenizer::readNextChar() {
    // Advance the position past the character we are leaving behind.
    if (currentChar == '\n') {
        line++;
        column = 0;
    }
    currentChar = input.get();
    column++;
}

void Tokenizer::skipWhitespace() {
    while (currentChar != EOF && std::isspace(currentChar)) {
        readNextChar();
    }
}

Token Tokenizer::scanIdentifier(int startLine, int startColumn) {
    std::string lexeme;
    while (currentChar != EOF && std::isalnum(currentChar)) {
        lexeme += (char)currentChar;
        readNextChar();
    }

    if (lexeme == "if")      return Token(IF, lexeme, startLine, startColumn);
    if (lexeme == "else")    return Token(ELSE, lexeme, startLine, startColumn);
    if (lexeme == "while")   return Token(WHILE, lexeme, startLine, startColumn);
    if (lexeme == "do")      return Token(DO, lexeme, startLine, startColumn);
    if (lexeme == "break")   return Token(BREAK, lexeme, startLine, startColumn);
    if (lexeme == "int")     return Token(INT, lexeme, startLine, startColumn);
    if (lexeme == "boolean") return Token(BOOLEAN, lexeme, startLine, startColumn);
    if (lexeme == "true")    return Token(TRUE, lexeme, startLine, startColumn);
    if (lexeme == "false")   return Token(FALSE, lexeme, startLine, startColumn);
    if (lexeme == "print")   return Token(PRINT, lexeme, startLine, startColumn);
    return Token(ID, lexeme, startLine, startColumn);
}

Token Tokenizer::scanNumber(int startLine, int startColumn) {
    std::string lexeme;
    while (currentChar != EOF && std::isdigit(currentChar)) {
        lexeme += (char)currentChar;
        readNextChar();
    }
    // Reject literals that don't fit in an int here, so the error has a position.
    if (lexeme.size() > 10 || std::stoll(lexeme) > 2147483647LL) {
        throw std::runtime_error("integer literal " + lexeme + " is too large " + position(startLine, startColumn));
    }
    return Token(NUM, lexeme, startLine, startColumn);
}
