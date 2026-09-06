#pragma once

#include "Token.hpp"
#include "AST.hpp"
#include <vector>
#include <memory>
#include <unordered_map>
#include <string>

class Parser {
public:
    explicit Parser(std::vector<Token> tokens);
    std::unique_ptr<BlockStmt> parseProgram();
    std::unique_ptr<Stmt> parseStatement();

private:
    std::vector<Token> tokens_;
    size_t cursor_;
    std::unordered_map<std::string, int> functionArity_;

    bool isAtEnd() const;
    const Token& peek() const;
    const Token& peekNext() const;
    const Token& previous() const;
    Token advance();
    bool check(TokenType type) const;
    bool match(TokenType type);
    void skipNewlines();

    std::unique_ptr<BlockStmt> parseBlock();
    std::unique_ptr<Stmt> parsePrint();
    std::unique_ptr<Stmt> parseWrite();
    std::unique_ptr<Stmt> parseApp();
    std::unique_ptr<Stmt> parseWindow();
    std::unique_ptr<Stmt> parseRun();
    std::unique_ptr<Stmt> parseSet();
    std::unique_ptr<Stmt> parseUse();
    std::unique_ptr<Stmt> parseIf();
    std::unique_ptr<Stmt> parseLoop();
    std::unique_ptr<Stmt> parseWhile();
    std::unique_ptr<Stmt> parseFnDecl();
    std::unique_ptr<Stmt> parseAssignmentOrExpr();

    std::unique_ptr<Expr> parseExpression();
    std::unique_ptr<Expr> parseLogicalOr();
    std::unique_ptr<Expr> parseLogicalAnd();
    std::unique_ptr<Expr> parseEquality();
    std::unique_ptr<Expr> parseRelational();
    std::unique_ptr<Expr> parseAdditive();
    std::unique_ptr<Expr> parseMultiplicative();
    std::unique_ptr<Expr> parseUnary();
    std::unique_ptr<Expr> parsePrimary();
    std::unique_ptr<Expr> parseCallOrPrimary();
    std::unique_ptr<Expr> parsePostfix(std::unique_ptr<Expr> expr);
};
