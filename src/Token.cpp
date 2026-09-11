#include "Token.hpp"

std::string tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::END_OF_FILE: return "END_OF_FILE";
        case TokenType::NEWLINE: return "NEWLINE";
        case TokenType::INDENT: return "INDENT";
        case TokenType::DEDENT: return "DEDENT";
        case TokenType::NUMBER: return "NUMBER";
        case TokenType::STRING: return "STRING";
        case TokenType::IDENTIFIER: return "IDENTIFIER";
        case TokenType::TRUE: return "TRUE";
        case TokenType::FALSE: return "FALSE";
        case TokenType::NIL: return "NIL";
        case TokenType::PRINT: return "PRINT";
        case TokenType::IF: return "IF";
        case TokenType::ELSE: return "ELSE";
        case TokenType::ELIF: return "ELIF";
        case TokenType::LOOP: return "LOOP";
        case TokenType::WHILE: return "WHILE";
        case TokenType::FN: return "FN";
        case TokenType::USE: return "USE";
        case TokenType::NEW: return "NEW";
        case TokenType::GET: return "GET";
        case TokenType::SET: return "SET";
        case TokenType::READ: return "READ";
        case TokenType::WRITE: return "WRITE";
        case TokenType::SEND: return "SEND";
        case TokenType::APP: return "APP";
        case TokenType::WINDOW: return "WINDOW";
        case TokenType::RUN: return "RUN";
        case TokenType::END: return "END";
        case TokenType::AND: return "AND";
        case TokenType::OR: return "OR";
        case TokenType::NOT: return "NOT";
        case TokenType::ASSIGN: return "ASSIGN";
        case TokenType::PLUS: return "PLUS";
        case TokenType::MINUS: return "MINUS";
        case TokenType::STAR: return "STAR";
        case TokenType::SLASH: return "SLASH";
        case TokenType::PERCENT: return "PERCENT";
        case TokenType::EQUAL_EQUAL: return "EQUAL_EQUAL";
        case TokenType::BANG_EQUAL: return "BANG_EQUAL";
        case TokenType::LESS: return "LESS";
        case TokenType::GREATER: return "GREATER";
        case TokenType::LESS_EQUAL: return "LESS_EQUAL";
        case TokenType::GREATER_EQUAL: return "GREATER_EQUAL";
        case TokenType::LPAREN: return "LPAREN";
        case TokenType::RPAREN: return "RPAREN";
        case TokenType::LBRACKET: return "LBRACKET";
        case TokenType::RBRACKET: return "RBRACKET";
        case TokenType::COMMA: return "COMMA";
        case TokenType::DOT: return "DOT";
        default: return "UNKNOWN";
    }
}
