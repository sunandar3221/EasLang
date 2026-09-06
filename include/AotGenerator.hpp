#pragma once

#include "AST.hpp"
#include <string>
#include <sstream>
#include <unordered_set>

class AotGenerator {
public:
    AotGenerator();
    std::string generateCpp(BlockStmt* program);
    bool buildBinary(const std::string& sourceFile, const std::string& outputFile);

private:
    int indentLevel_;
    std::unordered_set<std::string> declaredVars_;

    void emitIndent(std::ostringstream& ss);
    void generateStmt(Stmt* stmt, std::ostringstream& ss);
    void generateBlock(BlockStmt* block, std::ostringstream& ss);
    std::string generateExpr(Expr* expr);
};
