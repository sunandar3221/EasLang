#pragma once

#include "Token.hpp"
#include "Value.hpp"
#include <memory>
#include <vector>
#include <string>

class ASTNode {
public:
    int line;
    int column;
    virtual ~ASTNode() = default;
    ASTNode(int l = 0, int c = 0) : line(l), column(c) {}
};

class Expr : public ASTNode {
public:
    using ASTNode::ASTNode;
};

class Stmt : public ASTNode {
public:
    using ASTNode::ASTNode;
};

class LiteralExpr : public Expr {
public:
    Value value;
    LiteralExpr(Value val, int l = 0, int c = 0)
        : Expr(l, c), value(std::move(val)) {}
};

class VarExpr : public Expr {
public:
    std::string name;
    VarExpr(std::string n, int l = 0, int c = 0)
        : Expr(l, c), name(std::move(n)) {}
};

class ListLiteralExpr : public Expr {
public:
    std::vector<std::unique_ptr<Expr>> elements;
    ListLiteralExpr(std::vector<std::unique_ptr<Expr>> elems, int l = 0, int c = 0)
        : Expr(l, c), elements(std::move(elems)) {}
};

class IndexExpr : public Expr {
public:
    std::unique_ptr<Expr> target;
    std::unique_ptr<Expr> index;
    IndexExpr(std::unique_ptr<Expr> tgt, std::unique_ptr<Expr> idx, int l = 0, int c = 0)
        : Expr(l, c), target(std::move(tgt)), index(std::move(idx)) {}
};

class BinaryExpr : public Expr {
public:
    std::unique_ptr<Expr> left;
    TokenType op;
    std::unique_ptr<Expr> right;
    BinaryExpr(std::unique_ptr<Expr> l, TokenType o, std::unique_ptr<Expr> r, int ln = 0, int c = 0)
        : Expr(ln, c), left(std::move(l)), op(o), right(std::move(r)) {}
};

class UnaryExpr : public Expr {
public:
    TokenType op;
    std::unique_ptr<Expr> right;
    UnaryExpr(TokenType o, std::unique_ptr<Expr> r, int l = 0, int c = 0)
        : Expr(l, c), op(o), right(std::move(r)) {}
};

class CallExpr : public Expr {
public:
    std::string callee;
    std::vector<std::unique_ptr<Expr>> arguments;
    CallExpr(std::string fnName, std::vector<std::unique_ptr<Expr>> args, int l = 0, int c = 0)
        : Expr(l, c), callee(std::move(fnName)), arguments(std::move(args)) {}
};

class ReadExpr : public Expr {
public:
    std::unique_ptr<Expr> path;
    ReadExpr(std::unique_ptr<Expr> p, int l = 0, int c = 0)
        : Expr(l, c), path(std::move(p)) {}
};

class GetExpr : public Expr {
public:
    std::unique_ptr<Expr> target;
    std::unique_ptr<Expr> property;
    GetExpr(std::unique_ptr<Expr> tgt, std::unique_ptr<Expr> prop = nullptr, int l = 0, int c = 0)
        : Expr(l, c), target(std::move(tgt)), property(std::move(prop)) {}
};

class SendExpr : public Expr {
public:
    std::unique_ptr<Expr> target;
    std::unique_ptr<Expr> data;
    SendExpr(std::unique_ptr<Expr> tgt, std::unique_ptr<Expr> d, int l = 0, int c = 0)
        : Expr(l, c), target(std::move(tgt)), data(std::move(d)) {}
};

class NewExpr : public Expr {
public:
    std::string typeName;
    NewExpr(std::string t = "", int l = 0, int c = 0)
        : Expr(l, c), typeName(std::move(t)) {}
};

class BlockStmt : public Stmt {
public:
    std::vector<std::unique_ptr<Stmt>> statements;
    BlockStmt(std::vector<std::unique_ptr<Stmt>> stmts = {}, int l = 0, int c = 0)
        : Stmt(l, c), statements(std::move(stmts)) {}
};

class LoopExpr : public Expr {
public:
    std::unique_ptr<Expr> count;
    std::unique_ptr<BlockStmt> body;
    LoopExpr(std::unique_ptr<Expr> cnt, std::unique_ptr<BlockStmt> b, int l = 0, int c = 0)
        : Expr(l, c), count(std::move(cnt)), body(std::move(b)) {}
};

class IfExpr : public Expr {
public:
    std::unique_ptr<Expr> condition;
    std::unique_ptr<BlockStmt> thenBranch;
    std::unique_ptr<BlockStmt> elseBranch;
    IfExpr(std::unique_ptr<Expr> cond, std::unique_ptr<BlockStmt> thenB, std::unique_ptr<BlockStmt> elseB = nullptr, int l = 0, int c = 0)
        : Expr(l, c), condition(std::move(cond)), thenBranch(std::move(thenB)), elseBranch(std::move(elseB)) {}
};

class FnExpr : public Expr {
public:
    std::string name;
    std::vector<std::string> params;
    std::unique_ptr<BlockStmt> body;
    FnExpr(std::string n, std::vector<std::string> p, std::unique_ptr<BlockStmt> b, int l = 0, int c = 0)
        : Expr(l, c), name(std::move(n)), params(std::move(p)), body(std::move(b)) {}
};

class ExprStmt : public Stmt {
public:
    std::unique_ptr<Expr> expression;
    ExprStmt(std::unique_ptr<Expr> expr, int l = 0, int c = 0)
        : Stmt(l, c), expression(std::move(expr)) {}
};

class AssignStmt : public Stmt {
public:
    std::string name;
    std::unique_ptr<Expr> value;
    AssignStmt(std::string n, std::unique_ptr<Expr> val, int l = 0, int c = 0)
        : Stmt(l, c), name(std::move(n)), value(std::move(val)) {}
};

class IndexAssignStmt : public Stmt {
public:
    std::unique_ptr<Expr> target;
    std::unique_ptr<Expr> index;
    std::unique_ptr<Expr> value;
    IndexAssignStmt(std::unique_ptr<Expr> tgt, std::unique_ptr<Expr> idx, std::unique_ptr<Expr> val, int l = 0, int c = 0)
        : Stmt(l, c), target(std::move(tgt)), index(std::move(idx)), value(std::move(val)) {}
};

class IfStmt : public Stmt {
public:
    std::unique_ptr<Expr> condition;
    std::unique_ptr<BlockStmt> thenBranch;
    std::unique_ptr<BlockStmt> elseBranch;
    IfStmt(std::unique_ptr<Expr> cond, std::unique_ptr<BlockStmt> thenB, std::unique_ptr<BlockStmt> elseB = nullptr, int l = 0, int c = 0)
        : Stmt(l, c), condition(std::move(cond)), thenBranch(std::move(thenB)), elseBranch(std::move(elseB)) {}
};

class LoopStmt : public Stmt {
public:
    std::unique_ptr<Expr> count;
    std::unique_ptr<BlockStmt> body;
    LoopStmt(std::unique_ptr<Expr> cnt, std::unique_ptr<BlockStmt> b, int l = 0, int c = 0)
        : Stmt(l, c), count(std::move(cnt)), body(std::move(b)) {}
};

class WhileStmt : public Stmt {
public:
    std::unique_ptr<Expr> condition;
    std::unique_ptr<BlockStmt> body;
    WhileStmt(std::unique_ptr<Expr> cond, std::unique_ptr<BlockStmt> b, int l = 0, int c = 0)
        : Stmt(l, c), condition(std::move(cond)), body(std::move(b)) {}
};

class FnDeclStmt : public Stmt {
public:
    std::string name;
    std::vector<std::string> params;
    std::unique_ptr<BlockStmt> body;
    FnDeclStmt(std::string n, std::vector<std::string> p, std::unique_ptr<BlockStmt> b, int l = 0, int c = 0)
        : Stmt(l, c), name(std::move(n)), params(std::move(p)), body(std::move(b)) {}
};

class PrintStmt : public Stmt {
public:
    std::vector<std::unique_ptr<Expr>> arguments;
    bool silent;
    PrintStmt(std::vector<std::unique_ptr<Expr>> args, int l = 0, int c = 0, bool s = false)
        : Stmt(l, c), arguments(std::move(args)), silent(s) {}
};

class WriteStmt : public Stmt {
public:
    std::unique_ptr<Expr> path;
    std::unique_ptr<Expr> content;
    WriteStmt(std::unique_ptr<Expr> p, std::unique_ptr<Expr> c, int l = 0, int col = 0)
        : Stmt(l, col), path(std::move(p)), content(std::move(c)) {}
};

class SetStmt : public Stmt {
public:
    std::unique_ptr<Expr> target;
    std::unique_ptr<Expr> property;
    std::unique_ptr<Expr> value;
    SetStmt(std::unique_ptr<Expr> tgt, std::unique_ptr<Expr> prop, std::unique_ptr<Expr> val, int l = 0, int c = 0)
        : Stmt(l, c), target(std::move(tgt)), property(std::move(prop)), value(std::move(val)) {}
};

class AppStmt : public Stmt {
public:
    std::unique_ptr<Expr> title;
    AppStmt(std::unique_ptr<Expr> t, int l = 0, int c = 0)
        : Stmt(l, c), title(std::move(t)) {}
};

class WindowStmt : public Stmt {
public:
    std::unique_ptr<Expr> width;
    std::unique_ptr<Expr> height;
    WindowStmt(std::unique_ptr<Expr> w, std::unique_ptr<Expr> h, int l = 0, int c = 0)
        : Stmt(l, c), width(std::move(w)), height(std::move(h)) {}
};

class RunStmt : public Stmt {
public:
    std::unique_ptr<Expr> duration;
    RunStmt(std::unique_ptr<Expr> dur = nullptr, int l = 0, int c = 0)
        : Stmt(l, c), duration(std::move(dur)) {}
};

class UseStmt : public Stmt {
public:
    std::string moduleName;
    UseStmt(std::string mod, int l = 0, int c = 0)
        : Stmt(l, c), moduleName(std::move(mod)) {}
};
