#include "parser.h"
#include <stdexcept>
#include "token.h"

ASTNode::~ASTNode() {
    for (ASTNode* child : children) {
        delete child;
    }
}

Parser::Parser(Tokenizer& tokenizer) : tokenizer(tokenizer), loopDepth(0) {
    currentToken = tokenizer.getNextToken();
}

std::string Parser::where() const {
    return "at line " + std::to_string(currentToken.line) + ", column " + std::to_string(currentToken.column);
}

void Parser::consume(TokenType expectedType) {
    if (currentToken.type != expectedType) {
        throw std::runtime_error("unexpected token " + where() + ": expected " +
            tokenTypeToString(expectedType) + ", found " + tokenTypeToString(currentToken.type));
    }
    currentToken = tokenizer.getNextToken();
}

// <program> -> <block>
ASTNode* Parser::parseProgram() {
    ASTNode* node = parseBlock();
    if (currentToken.type != END_OF_FILE) {
        throw std::runtime_error("unexpected " + tokenTypeToString(currentToken.type) +
            " after the end of the program " + where());
    }
    return node;
}

// <block> -> { <decls> <stmts> }
ASTNode* Parser::parseBlock() {
    consume(LBRACE);
    ASTNode* declsNode = parseDecls();
    ASTNode* stmtsNode = parseStmts();
    consume(RBRACE);
    ASTNode* node = new ASTNode(ASTNode::BLOCK);
    node->children.push_back(declsNode);
    node->children.push_back(stmtsNode);
    return node;
}

// <decls> -> <decl> <decls> | epsilon
ASTNode* Parser::parseDecls() {
    ASTNode* node = new ASTNode(ASTNode::DECLS);
    while (currentToken.type == INT || currentToken.type == BOOLEAN) {
        node->children.push_back(parseDecl());
    }
    return node;
}

// <decl>  -> <type> id ;
// <type>  -> <type> [ num ] | <basic>
// <basic> -> int | boolean
ASTNode* Parser::parseDecl() {
    ASTNode::ValueType baseType;
    std::string typeStr = currentToken.value;
    if (currentToken.type == INT) {
        baseType = ASTNode::INT_TYPE;
        consume(INT);
    } else if (currentToken.type == BOOLEAN) {
        baseType = ASTNode::BOOL_TYPE;
        consume(BOOLEAN);
    } else {
        throw std::runtime_error("expected 'int' or 'boolean' " + where());
    }

    ASTNode* typeNode = new ASTNode(ASTNode::BASIC, typeStr, baseType);
    std::vector<int> dims;

    // Each [num] wraps the previous type, so int[2][3] becomes TYPE(TYPE(BASIC, 2), 3).
    while (currentToken.type == LBRACKET) {
        consume(LBRACKET);
        if (currentToken.type != NUM) {
            throw std::runtime_error("array size must be a number " + where());
        }
        int size = std::stoi(currentToken.value);
        if (size <= 0) {
            throw std::runtime_error("array size must be positive " + where());
        }
        dims.push_back(size);

        ASTNode* numNode = new ASTNode(ASTNode::NUM, currentToken.value, ASTNode::INT_TYPE);
        consume(NUM);
        consume(RBRACKET);

        ASTNode* arrayTypeNode = new ASTNode(ASTNode::TYPE, "[]", baseType);
        arrayTypeNode->children.push_back(typeNode);
        arrayTypeNode->children.push_back(numNode);
        typeNode = arrayTypeNode;
    }

    if (currentToken.type != ID) {
        throw std::runtime_error("expected an identifier in declaration " + where());
    }
    std::string id = currentToken.value;
    ASTNode* idNode = new ASTNode(ASTNode::ID, id, baseType);
    consume(ID);
    consume(SEMICOLON);

    // Memory is a single global space, so a name can be declared only once in the whole program.
    if (symbolTable.count(id)) {
        throw std::runtime_error("variable '" + id + "' is already declared");
    }

    Symbol sym;
    sym.baseType = baseType;
    sym.dims = dims;
    symbolTable[id] = sym;

    ASTNode* node = new ASTNode(ASTNode::DECL);
    node->children.push_back(typeNode);
    node->children.push_back(idNode);
    return node;
}

// <stmts> -> <stmt> <stmts> | epsilon
ASTNode* Parser::parseStmts() {
    ASTNode* node = new ASTNode(ASTNode::STMTS);
    while (currentToken.type != RBRACE && currentToken.type != END_OF_FILE) {
        node->children.push_back(parseStmt());
    }
    return node;
}

ASTNode* Parser::parseStmt() {
    ASTNode* node = nullptr;
    switch (currentToken.type) {
        case ID: { // <loc> = <bool> ;
            ASTNode* locNode = parseLoc();
            if (currentToken.type != ASSIGN) {
                throw std::runtime_error("expected '=' after '" + locNode->value + "' " + where());
            }
            consume(ASSIGN);
            ASTNode* exprNode = parseBool();
            if (locNode->valuetype != exprNode->valuetype) {
                throw std::runtime_error("type mismatch in assignment to '" + locNode->value + "'");
            }
            consume(SEMICOLON);
            node = new ASTNode(ASTNode::ASSIGN, "=", locNode->valuetype);
            node->children.push_back(locNode);
            node->children.push_back(exprNode);
            break;
        }
        case IF: { // if ( <bool> ) <stmt> [ else <stmt> ]
            consume(IF);
            consume(LPAREN);
            ASTNode* boolNode = parseBool();
            if (boolNode->valuetype != ASTNode::BOOL_TYPE) {
                throw std::runtime_error("'if' condition must be boolean");
            }
            consume(RPAREN);
            ASTNode* stmtNode = parseStmt();
            node = new ASTNode(ASTNode::IF);
            node->children.push_back(boolNode);
            node->children.push_back(stmtNode);
            // A dangling else binds to the nearest if.
            if (currentToken.type == ELSE) {
                consume(ELSE);
                node->children.push_back(parseStmt());
            }
            break;
        }
        case WHILE: { // while ( <bool> ) <stmt>
            consume(WHILE);
            consume(LPAREN);
            ASTNode* boolNode = parseBool();
            if (boolNode->valuetype != ASTNode::BOOL_TYPE) {
                throw std::runtime_error("'while' condition must be boolean");
            }
            consume(RPAREN);
            loopDepth++;
            ASTNode* stmtNode = parseStmt();
            loopDepth--;
            node = new ASTNode(ASTNode::WHILE);
            node->children.push_back(boolNode);
            node->children.push_back(stmtNode);
            break;
        }
        case DO: { // do <stmt> while ( <bool> ) ;
            consume(DO);
            loopDepth++;
            ASTNode* stmtNode = parseStmt();
            loopDepth--;
            consume(WHILE);
            consume(LPAREN);
            ASTNode* boolNode = parseBool();
            if (boolNode->valuetype != ASTNode::BOOL_TYPE) {
                throw std::runtime_error("'do-while' condition must be boolean");
            }
            consume(RPAREN);
            consume(SEMICOLON);
            node = new ASTNode(ASTNode::DO);
            node->children.push_back(stmtNode);
            node->children.push_back(boolNode);
            break;
        }
        case BREAK: { // break ;
            if (loopDepth <= 0) {
                throw std::runtime_error("'break' outside of a loop " + where());
            }
            consume(BREAK);
            consume(SEMICOLON);
            node = new ASTNode(ASTNode::BREAK);
            break;
        }
        case PRINT: { // print ( <bool> ) ;
            consume(PRINT);
            consume(LPAREN);
            ASTNode* boolNode = parseBool();
            consume(RPAREN);
            consume(SEMICOLON);
            node = new ASTNode(ASTNode::PRINT);
            node->children.push_back(boolNode);
            break;
        }
        case LBRACE:
            node = parseBlock();
            break;
        default:
            throw std::runtime_error("invalid statement " + where() + ": unexpected " +
                tokenTypeToString(currentToken.type));
    }
    return node;
}

// <loc> -> <loc> [ <bool> ] | id
// Only scalar cells hold values, so an array must get exactly one index per dimension.
ASTNode* Parser::parseLoc() {
    if (currentToken.type != ID) {
        throw std::runtime_error("expected an identifier " + where());
    }
    std::string id = currentToken.value;
    auto it = symbolTable.find(id);
    if (it == symbolTable.end()) {
        throw std::runtime_error("undeclared variable '" + id + "' " + where());
    }
    const Symbol& sym = it->second;
    ASTNode* node = new ASTNode(ASTNode::ID, id, sym.baseType);
    consume(ID);

    size_t indices = 0;
    while (currentToken.type == LBRACKET) {
        if (indices == sym.dims.size()) {
            throw std::runtime_error(sym.isArray()
                ? "too many indices for array '" + id + "' " + where()
                : "variable '" + id + "' is not an array " + where());
        }
        consume(LBRACKET);
        ASTNode* indexExpr = parseBool();
        if (indexExpr->valuetype != ASTNode::INT_TYPE) {
            throw std::runtime_error("array index for '" + id + "' must be an integer");
        }
        consume(RBRACKET);
        indices++;

        ASTNode* locNode = new ASTNode(ASTNode::LOC, id, sym.baseType);
        locNode->children.push_back(node);
        locNode->children.push_back(indexExpr);
        node = locNode;
    }

    if (indices < sym.dims.size()) {
        throw std::runtime_error("array '" + id + "' needs " + std::to_string(sym.dims.size()) +
            " index(es), got " + std::to_string(indices) + " " + where());
    }
    return node;
}

// <bool> -> <bool> || <join> | <join>
// The left-recursive rules below are written as loops to keep left associativity.
ASTNode* Parser::parseBool() {
    ASTNode* node = parseJoin();
    while (currentToken.type == OR) {
        consume(OR);
        ASTNode* right = parseJoin();
        if (node->valuetype != ASTNode::BOOL_TYPE || right->valuetype != ASTNode::BOOL_TYPE) {
            throw std::runtime_error("operator '||' needs boolean operands");
        }
        ASTNode* newNode = new ASTNode(ASTNode::OR, "||", ASTNode::BOOL_TYPE);
        newNode->children.push_back(node);
        newNode->children.push_back(right);
        node = newNode;
    }
    return node;
}

// <join> -> <join> && <equality> | <equality>
ASTNode* Parser::parseJoin() {
    ASTNode* node = parseEquality();
    while (currentToken.type == AND) {
        consume(AND);
        ASTNode* right = parseEquality();
        if (node->valuetype != ASTNode::BOOL_TYPE || right->valuetype != ASTNode::BOOL_TYPE) {
            throw std::runtime_error("operator '&&' needs boolean operands");
        }
        ASTNode* newNode = new ASTNode(ASTNode::AND, "&&", ASTNode::BOOL_TYPE);
        newNode->children.push_back(node);
        newNode->children.push_back(right);
        node = newNode;
    }
    return node;
}

// <equality> -> <equality> == <rel> | <equality> != <rel> | <rel>
ASTNode* Parser::parseEquality() {
    ASTNode* node = parseRel();
    while (currentToken.type == EQ || currentToken.type == NEQ) {
        TokenType op = currentToken.type;
        std::string opStr = currentToken.value;
        consume(op);
        ASTNode* right = parseRel();
        if (node->valuetype != right->valuetype) {
            throw std::runtime_error("operands of '" + opStr + "' must have the same type");
        }
        ASTNode* newNode = new ASTNode(op == EQ ? ASTNode::EQ : ASTNode::NEQ, opStr, ASTNode::BOOL_TYPE);
        newNode->children.push_back(node);
        newNode->children.push_back(right);
        node = newNode;
    }
    return node;
}

// <rel> -> <expr> < <expr> | <expr> <= <expr> | <expr> >= <expr> | <expr> > <expr> | <expr>
// The grammar is not recursive here, so a < b < c is a syntax error.
ASTNode* Parser::parseRel() {
    ASTNode* node = parseExpr();
    if (currentToken.type == LT || currentToken.type == LEQ ||
        currentToken.type == GT || currentToken.type == GEQ) {
        TokenType op = currentToken.type;
        std::string opStr = currentToken.value;
        consume(op);
        ASTNode* right = parseExpr();
        if (node->valuetype != ASTNode::INT_TYPE || right->valuetype != ASTNode::INT_TYPE) {
            throw std::runtime_error("operator '" + opStr + "' needs integer operands");
        }

        ASTNode::NodeType nodeType;
        if (op == LT) nodeType = ASTNode::LT;
        else if (op == LEQ) nodeType = ASTNode::LEQ;
        else if (op == GT) nodeType = ASTNode::GT;
        else nodeType = ASTNode::GEQ;

        ASTNode* newNode = new ASTNode(nodeType, opStr, ASTNode::BOOL_TYPE);
        newNode->children.push_back(node);
        newNode->children.push_back(right);
        node = newNode;
    }
    return node;
}

// <expr> -> <expr> + <term> | <expr> - <term> | <term>
ASTNode* Parser::parseExpr() {
    ASTNode* node = parseTerm();
    while (currentToken.type == PLUS || currentToken.type == MINUS) {
        TokenType op = currentToken.type;
        consume(op);
        ASTNode* right = parseTerm();
        if (node->valuetype != ASTNode::INT_TYPE || right->valuetype != ASTNode::INT_TYPE) {
            throw std::runtime_error("operands of '+' and '-' must be integers");
        }
        ASTNode* newNode = new ASTNode(op == PLUS ? ASTNode::PLUS : ASTNode::MINUS, op == PLUS ? "+" : "-", ASTNode::INT_TYPE);
        newNode->children.push_back(node);
        newNode->children.push_back(right);
        node = newNode;
    }
    return node;
}

// <term> -> <term> * <unary> | <term> / <unary> | <unary>
ASTNode* Parser::parseTerm() {
    ASTNode* node = parseUnary();
    while (currentToken.type == TIMES || currentToken.type == DIVIDE) {
        TokenType op = currentToken.type;
        consume(op);
        ASTNode* right = parseUnary();
        if (node->valuetype != ASTNode::INT_TYPE || right->valuetype != ASTNode::INT_TYPE) {
            throw std::runtime_error("operands of '*' and '/' must be integers");
        }
        ASTNode* newNode = new ASTNode(op == TIMES ? ASTNode::TIMES : ASTNode::DIVIDE, op == TIMES ? "*" : "/", ASTNode::INT_TYPE);
        newNode->children.push_back(node);
        newNode->children.push_back(right);
        node = newNode;
    }
    return node;
}

// <unary> -> ! <unary> | - <unary> | <factor>
ASTNode* Parser::parseUnary() {
    if (currentToken.type == NOT) {
        consume(NOT);
        ASTNode* operand = parseUnary();
        if (operand->valuetype != ASTNode::BOOL_TYPE) {
            throw std::runtime_error("operator '!' needs a boolean operand");
        }
        ASTNode* node = new ASTNode(ASTNode::NOT, "!", ASTNode::BOOL_TYPE);
        node->children.push_back(operand);
        return node;
    }
    if (currentToken.type == MINUS) {
        consume(MINUS);
        ASTNode* operand = parseUnary();
        if (operand->valuetype != ASTNode::INT_TYPE) {
            throw std::runtime_error("unary '-' needs an integer operand");
        }
        ASTNode* node = new ASTNode(ASTNode::UNARY_MINUS, "-", ASTNode::INT_TYPE);
        node->children.push_back(operand);
        return node;
    }
    return parseFactor();
}

// <factor> -> ( <bool> ) | <loc> | num | true | false
ASTNode* Parser::parseFactor() {
    switch (currentToken.type) {
        case NUM: {
            ASTNode* node = new ASTNode(ASTNode::NUM, currentToken.value, ASTNode::INT_TYPE);
            consume(NUM);
            return node;
        }
        case ID:
            return parseLoc();
        case TRUE: {
            ASTNode* node = new ASTNode(ASTNode::TRUE, currentToken.value, ASTNode::BOOL_TYPE);
            consume(TRUE);
            return node;
        }
        case FALSE: {
            ASTNode* node = new ASTNode(ASTNode::FALSE, currentToken.value, ASTNode::BOOL_TYPE);
            consume(FALSE);
            return node;
        }
        case LPAREN: {
            consume(LPAREN);
            ASTNode* node = parseBool();
            consume(RPAREN);
            return node;
        }
        default:
            throw std::runtime_error("unexpected " + tokenTypeToString(currentToken.type) +
                " in expression " + where());
    }
}

std::string Parser::tokenTypeToString(TokenType type) {
    switch (type) {
        case IF: return "'if'";
        case ELSE: return "'else'";
        case WHILE: return "'while'";
        case DO: return "'do'";
        case BREAK: return "'break'";
        case INT: return "'int'";
        case BOOLEAN: return "'boolean'";
        case TRUE: return "'true'";
        case FALSE: return "'false'";
        case PRINT: return "'print'";
        case ID: return "identifier";
        case NUM: return "number";
        case PLUS: return "'+'";
        case MINUS: return "'-'";
        case TIMES: return "'*'";
        case DIVIDE: return "'/'";
        case EQ: return "'=='";
        case NEQ: return "'!='";
        case LT: return "'<'";
        case LEQ: return "'<='";
        case GT: return "'>'";
        case GEQ: return "'>='";
        case AND: return "'&&'";
        case OR: return "'||'";
        case NOT: return "'!'";
        case ASSIGN: return "'='";
        case SEMICOLON: return "';'";
        case LPAREN: return "'('";
        case RPAREN: return "')'";
        case LBRACE: return "'{'";
        case RBRACE: return "'}'";
        case LBRACKET: return "'['";
        case RBRACKET: return "']'";
        case END_OF_FILE: return "end of file";
        default: return "unknown token";
    }
}
