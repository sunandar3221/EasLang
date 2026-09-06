#pragma once

#include "AST.hpp"
#include "Environment.hpp"
#include "StandardLibrary.hpp"
#include <memory>

class Interpreter {
public:
    Interpreter();
    explicit Interpreter(std::shared_ptr<Environment> env);

    Value interpret(BlockStmt* program);
    Value execute(Stmt* stmt);
    Value executeBlock(BlockStmt* block, std::shared_ptr<Environment> env);
    Value evaluate(Expr* expr);

    std::shared_ptr<Environment> getGlobalEnvironment() const;

private:
    std::shared_ptr<Environment> globalEnv_;
    std::shared_ptr<Environment> currentEnv_;

    void registerBuiltins();
};
