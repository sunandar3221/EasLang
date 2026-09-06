#pragma once

#include "AST.hpp"
#include "Bytecode.hpp"
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>

struct Local {
    std::string name;
    int depth;
};

class BytecodeCompiler {
public:
    BytecodeCompiler();

    std::shared_ptr<Chunk> compile(BlockStmt* program);
    const std::unordered_map<std::string, std::shared_ptr<Chunk>>& getFunctions() const;

private:
    Chunk* currentChunk_;
    std::shared_ptr<Chunk> mainChunk_;
    std::vector<Local> locals_;
    int scopeDepth_;
    int maxLocals_;
    std::unordered_map<std::string, std::shared_ptr<Chunk>> functions_;

    void compileStmt(Stmt* stmt);
    void compileBlock(BlockStmt* block);
    void compileExpr(Expr* expr);
    void compileBlockAsExpr(BlockStmt* block, int line = 1);

    int resolveLocal(const std::string& name);
    void addLocal(const std::string& name);
    size_t emitJump(OpCode op, int line = 1);
    void patchJump(size_t offset);
    void emitLoop(size_t loopStart, int line = 1);
};
