#include "token.h"

Token::Token(TokenType type, std::string value, int line, int column) :
    type(type), value(value), line(line), column(column) {
}
