#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <iostream>
#include "token.h"

// Turns the raw character stream into tokens, one at a time.
class Tokenizer {
public:
    Tokenizer(std::istream& input);

    Token getNextToken();

private:
    std::istream& input;
    int currentChar;
    int line;    // position of currentChar
    int column;

    void readNextChar();
    void skipWhitespace();
    Token scanIdentifier(int startLine, int startColumn);
    Token scanNumber(int startLine, int startColumn);
};

#endif
