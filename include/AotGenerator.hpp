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
    int loopCounter_;
    std::unordered_set<std::string> declaredVars_;

    void emitIndent(std::ostringstream& ss);
    void generateStmt(Stmt* stmt, std::ostringstream& ss);
    void generateBlock(BlockStmt* block, std::ostringstream& ss, bool isFunctionBody = false);
    void generateReturnStmt(Stmt* stmt, std::ostringstream& ss);
    std::string generateExpr(Expr* expr);
    void collectVariables(ASTNode* node, std::unordered_set<std::string>& vars);
    void resolveImports(BlockStmt* program, std::vector<std::unique_ptr<FnDeclStmt>>& extraFns, std::vector<std::unique_ptr<Stmt>>& extraStmts, std::unordered_set<std::string>& visited);
};
