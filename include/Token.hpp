#pragma once

#include <string>
#include <string_view>

enum class TokenType {
    END_OF_FILE,
    NEWLINE,
    INDENT,
    DEDENT,

    NUMBER,
    STRING,
    IDENTIFIER,

    TRUE,
    FALSE,
    NIL,

    PRINT,
    SILENT_PRINT,
    IF,
    ELSE,
    LOOP,
    WHILE,
    FN,
    USE,
    NEW,
    GET,
    SET,
    READ,
    WRITE,
    SEND,
    APP,
    WINDOW,
    RUN,
    END,

    AND,
    OR,
    NOT,

    ASSIGN,
    PLUS,
    MINUS,
    STAR,
    SLASH,
    PERCENT,
    EQUAL_EQUAL,
    BANG_EQUAL,
    LESS,
    GREATER,
    LESS_EQUAL,
    GREATER_EQUAL,

    LPAREN,
    RPAREN,
    LBRACKET,
    RBRACKET,
    COMMA,
    DOT
};

struct Token {
    TokenType type;
    std::string lexeme;
    int64_t intValue;
    double floatValue;
    bool isFloat;
    int line;
    int column;

    Token(TokenType t = TokenType::END_OF_FILE, std::string lex = "", int l = 1, int c = 1)
        : type(t), lexeme(std::move(lex)), intValue(0), floatValue(0.0), isFloat(false), line(l), column(c) {}
};

std::string tokenTypeToString(TokenType type);
