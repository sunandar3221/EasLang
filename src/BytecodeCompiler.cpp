#include "BytecodeCompiler.hpp"
#include <algorithm>

BytecodeCompiler::BytecodeCompiler()
    : currentChunk_(nullptr), scopeDepth_(0), maxLocals_(0) {}

const std::unordered_map<std::string, std::shared_ptr<Chunk>>& BytecodeCompiler::getFunctions() const {
    return functions_;
}

std::shared_ptr<Chunk> BytecodeCompiler::compile(BlockStmt* program) {
    mainChunk_ = std::make_shared<Chunk>("__main__", 0);
    currentChunk_ = mainChunk_.get();
    locals_.clear();
    scopeDepth_ = 0;
    maxLocals_ = 0;

    compileBlock(program);

    mainChunk_->localsCount = std::max(maxLocals_, static_cast<int>(locals_.size()));
    currentChunk_->emitOp(OpCode::OP_NIL, 1);
    currentChunk_->emitOp(OpCode::OP_RETURN, 1);
    return mainChunk_;
}

int BytecodeCompiler::resolveLocal(const std::string& name) {
    for (int i = static_cast<int>(locals_.size()) - 1; i >= 0; --i) {
        if (locals_[i].name == name) {
            return i;
        }
    }
    return -1;
}

void BytecodeCompiler::addLocal(const std::string& name) {
    locals_.push_back({name, scopeDepth_});
    if (static_cast<int>(locals_.size()) > maxLocals_) {
        maxLocals_ = static_cast<int>(locals_.size());
    }
}

size_t BytecodeCompiler::emitJump(OpCode op, int line) {
    currentChunk_->emitOp(op, line);
    currentChunk_->emit(0xff, line);
    currentChunk_->emit(0xff, line);
    return currentChunk_->code.size() - 2;
}

void BytecodeCompiler::patchJump(size_t offset) {
    size_t jump = currentChunk_->code.size() - offset - 2;
    currentChunk_->code[offset] = static_cast<uint8_t>((jump >> 8) & 0xff);
    currentChunk_->code[offset + 1] = static_cast<uint8_t>(jump & 0xff);
}

void BytecodeCompiler::emitLoop(size_t loopStart, int line) {
    currentChunk_->emitOp(OpCode::OP_LOOP, line);
    size_t offset = currentChunk_->code.size() - loopStart + 2;
    currentChunk_->emitShort(static_cast<uint16_t>(offset), line);
}

void BytecodeCompiler::compileBlock(BlockStmt* block) {
    if (!block) return;
    for (size_t i = 0; i < block->statements.size(); ++i) {
        bool isLast = (i + 1 == block->statements.size());
        Stmt* stmt = block->statements[i].get();
        if (isLast && scopeDepth_ > 0) {
            if (auto* exprStmt = dynamic_cast<ExprStmt*>(stmt)) {
                compileExpr(exprStmt->expression.get());
                continue;
            }
        }
        compileStmt(stmt);
    }
}

void BytecodeCompiler::compileBlockAsExpr(BlockStmt* block, int line) {
    if (!block || block->statements.empty()) {
        currentChunk_->emitOp(OpCode::OP_NIL, line);
        return;
    }
    for (size_t i = 0; i < block->statements.size(); ++i) {
        bool isLast = (i + 1 == block->statements.size());
        Stmt* stmt = block->statements[i].get();
        if (isLast) {
            if (auto* exprStmt = dynamic_cast<ExprStmt*>(stmt)) {
                compileExpr(exprStmt->expression.get());
            } else if (auto* printStmt = dynamic_cast<PrintStmt*>(stmt)) {
                for (const auto& a : printStmt->arguments) {
                    compileExpr(a.get());
                }
                currentChunk_->emitOp(printStmt->silent ? OpCode::OP_SILENT_PRINT : OpCode::OP_PRINT, printStmt->line);
                currentChunk_->emit(static_cast<uint8_t>(printStmt->arguments.size()), printStmt->line);
            } else {
                compileStmt(stmt);
                currentChunk_->emitOp(OpCode::OP_NIL, line);
            }
        } else {
            compileStmt(stmt);
        }
    }
}

void BytecodeCompiler::compileStmt(Stmt* stmt) {
    if (!stmt) return;

    if (auto* assign = dynamic_cast<AssignStmt*>(stmt)) {
        compileExpr(assign->value.get());
        if (scopeDepth_ > 0) {
            int local = resolveLocal(assign->name);
            if (local < 0) {
                addLocal(assign->name);
                local = static_cast<int>(locals_.size() - 1);
            }
            currentChunk_->emitOp(OpCode::OP_SET_LOCAL, assign->line);
            currentChunk_->emitShort(static_cast<uint16_t>(local), assign->line);
        } else {
            size_t idx = currentChunk_->addConstant(Value(assign->name));
            currentChunk_->emitOp(OpCode::OP_SET_GLOBAL, assign->line);
            currentChunk_->emitShort(static_cast<uint16_t>(idx), assign->line);
        }
        currentChunk_->emitOp(OpCode::OP_POP, assign->line);
        return;
    }

    if (auto* idxAssign = dynamic_cast<IndexAssignStmt*>(stmt)) {
        compileExpr(idxAssign->target.get());
        compileExpr(idxAssign->index.get());
        compileExpr(idxAssign->value.get());
        currentChunk_->emitOp(OpCode::OP_SET_INDEX, idxAssign->line);
        return;
    }

    if (auto* exprStmt = dynamic_cast<ExprStmt*>(stmt)) {
        compileExpr(exprStmt->expression.get());
        currentChunk_->emitOp(OpCode::OP_POP, exprStmt->line);
        return;
    }

    if (auto* ifStmt = dynamic_cast<IfStmt*>(stmt)) {
        compileExpr(ifStmt->condition.get());
        size_t thenJump = emitJump(OpCode::OP_JUMP_IF_FALSE, ifStmt->line);
        currentChunk_->emitOp(OpCode::OP_POP, ifStmt->line);
        compileBlock(ifStmt->thenBranch.get());
        size_t elseJump = emitJump(OpCode::OP_JUMP, ifStmt->line);
        patchJump(thenJump);
        currentChunk_->emitOp(OpCode::OP_POP, ifStmt->line);
        if (ifStmt->elseBranch) {
            compileBlock(ifStmt->elseBranch.get());
        }
        patchJump(elseJump);
        return;
    }

    if (auto* whileStmt = dynamic_cast<WhileStmt*>(stmt)) {
        size_t loopStart = currentChunk_->code.size();
        compileExpr(whileStmt->condition.get());
        size_t exitJump = emitJump(OpCode::OP_JUMP_IF_FALSE, whileStmt->line);
        currentChunk_->emitOp(OpCode::OP_POP, whileStmt->line);
        compileBlock(whileStmt->body.get());
        emitLoop(loopStart, whileStmt->line);
        patchJump(exitJump);
        currentChunk_->emitOp(OpCode::OP_POP, whileStmt->line);
        return;
    }

    if (auto* loopStmt = dynamic_cast<LoopStmt*>(stmt)) {
        compileExpr(loopStmt->count.get());
        std::string loopVar = "$loop_" + std::to_string(locals_.size());
        addLocal(loopVar);
        int slot = static_cast<int>(locals_.size() - 1);
        currentChunk_->emitOp(OpCode::OP_SET_LOCAL, loopStmt->line);
        currentChunk_->emitShort(static_cast<uint16_t>(slot), loopStmt->line);
        currentChunk_->emitOp(OpCode::OP_POP, loopStmt->line);

        size_t loopStart = currentChunk_->code.size();
        currentChunk_->emitOp(OpCode::OP_GET_LOCAL, loopStmt->line);
        currentChunk_->emitShort(static_cast<uint16_t>(slot), loopStmt->line);
        currentChunk_->writeConstant(Value(static_cast<int64_t>(0)), loopStmt->line);
        currentChunk_->emitOp(OpCode::OP_GREATER, loopStmt->line);
        size_t exitJump = emitJump(OpCode::OP_JUMP_IF_FALSE, loopStmt->line);
        currentChunk_->emitOp(OpCode::OP_POP, loopStmt->line);

        compileBlock(loopStmt->body.get());

        currentChunk_->emitOp(OpCode::OP_GET_LOCAL, loopStmt->line);
        currentChunk_->emitShort(static_cast<uint16_t>(slot), loopStmt->line);
        currentChunk_->writeConstant(Value(static_cast<int64_t>(1)), loopStmt->line);
        currentChunk_->emitOp(OpCode::OP_SUB, loopStmt->line);
        currentChunk_->emitOp(OpCode::OP_SET_LOCAL, loopStmt->line);
        currentChunk_->emitShort(static_cast<uint16_t>(slot), loopStmt->line);
        currentChunk_->emitOp(OpCode::OP_POP, loopStmt->line);

        emitLoop(loopStart, loopStmt->line);
        patchJump(exitJump);
        currentChunk_->emitOp(OpCode::OP_POP, loopStmt->line);
        return;
    }

    if (auto* fnDecl = dynamic_cast<FnDeclStmt*>(stmt)) {
        auto fnChunk = std::make_shared<Chunk>(fnDecl->name, static_cast<int>(fnDecl->params.size()));
        Chunk* prevChunk = currentChunk_;
        auto prevLocals = locals_;
        int prevDepth = scopeDepth_;
        int prevMax = maxLocals_;

        currentChunk_ = fnChunk.get();
        locals_.clear();
        scopeDepth_ = 1;
        maxLocals_ = 0;

        for (const auto& p : fnDecl->params) {
            addLocal(p);
        }

        compileBlock(fnDecl->body.get());

        fnChunk->localsCount = std::max(maxLocals_, static_cast<int>(locals_.size()));
        currentChunk_->emitOp(OpCode::OP_RETURN, fnDecl->line);

        functions_[fnDecl->name] = fnChunk;

        currentChunk_ = prevChunk;
        locals_ = prevLocals;
        scopeDepth_ = prevDepth;
        maxLocals_ = prevMax;
        return;
    }

    if (auto* printStmt = dynamic_cast<PrintStmt*>(stmt)) {
        for (const auto& a : printStmt->arguments) {
            compileExpr(a.get());
        }
        currentChunk_->emitOp(printStmt->silent ? OpCode::OP_SILENT_PRINT : OpCode::OP_PRINT, printStmt->line);
        currentChunk_->emit(static_cast<uint8_t>(printStmt->arguments.size()), printStmt->line);
        currentChunk_->emitOp(OpCode::OP_POP, printStmt->line);
        return;
    }

    if (auto* writeStmt = dynamic_cast<WriteStmt*>(stmt)) {
        compileExpr(writeStmt->path.get());
        compileExpr(writeStmt->content.get());
        currentChunk_->emitOp(OpCode::OP_WRITE, writeStmt->line);
        return;
    }

    if (auto* appStmt = dynamic_cast<AppStmt*>(stmt)) {
        compileExpr(appStmt->title.get());
        currentChunk_->emitOp(OpCode::OP_APP, appStmt->line);
        return;
    }

    if (auto* winStmt = dynamic_cast<WindowStmt*>(stmt)) {
        compileExpr(winStmt->width.get());
        compileExpr(winStmt->height.get());
        currentChunk_->emitOp(OpCode::OP_WINDOW, winStmt->line);
        return;
    }

    if (auto* runStmt = dynamic_cast<RunStmt*>(stmt)) {
        if (runStmt->duration) {
            compileExpr(runStmt->duration.get());
        } else {
            currentChunk_->emitOp(OpCode::OP_NIL, runStmt->line);
        }
        currentChunk_->emitOp(OpCode::OP_RUN, runStmt->line);
        return;
    }

    if (auto* setStmt = dynamic_cast<SetStmt*>(stmt)) {
        compileExpr(setStmt->target.get());
        compileExpr(setStmt->property.get());
        compileExpr(setStmt->value.get());
        currentChunk_->emitOp(OpCode::OP_SET_PROP, setStmt->line);
        return;
    }

    if (auto* blockStmt = dynamic_cast<BlockStmt*>(stmt)) {
        compileBlock(blockStmt);
        return;
    }

    if (auto* useStmt = dynamic_cast<UseStmt*>(stmt)) {
        size_t idx = currentChunk_->addConstant(Value(useStmt->moduleName));
        currentChunk_->emitOp(OpCode::OP_USE, useStmt->line);
        currentChunk_->emitShort(static_cast<uint16_t>(idx), useStmt->line);
        return;
    }
}

void BytecodeCompiler::compileExpr(Expr* expr) {
    if (!expr) return;

    if (auto* lit = dynamic_cast<LiteralExpr*>(expr)) {
        if (lit->value.isNil()) {
            currentChunk_->emitOp(OpCode::OP_NIL, lit->line);
        } else if (lit->value.isBool()) {
            currentChunk_->emitOp(lit->value.boolVal ? OpCode::OP_TRUE : OpCode::OP_FALSE, lit->line);
        } else {
            currentChunk_->writeConstant(lit->value, lit->line);
        }
        return;
    }

    if (auto* var = dynamic_cast<VarExpr*>(expr)) {
        if (scopeDepth_ > 0) {
            int local = resolveLocal(var->name);
            if (local >= 0) {
                currentChunk_->emitOp(OpCode::OP_GET_LOCAL, var->line);
                currentChunk_->emitShort(static_cast<uint16_t>(local), var->line);
                return;
            }
        }
        auto it = functions_.find(var->name);
        if (it != functions_.end()) {
            size_t idx = currentChunk_->addConstant(Value(ValueType::FUNCTION, var->name));
            currentChunk_->emitOp(OpCode::OP_CONSTANT, var->line);
            currentChunk_->emitShort(static_cast<uint16_t>(idx), var->line);
            return;
        }
        size_t idx = currentChunk_->addConstant(Value(var->name));
        currentChunk_->emitOp(OpCode::OP_GET_GLOBAL, var->line);
        currentChunk_->emitShort(static_cast<uint16_t>(idx), var->line);
        return;
    }

    if (auto* loopExpr = dynamic_cast<LoopExpr*>(expr)) {
        compileExpr(loopExpr->count.get());
        std::string loopVar = "$lexpr_cnt_" + std::to_string(locals_.size());
        addLocal(loopVar);
        int loopSlot = static_cast<int>(locals_.size() - 1);
        currentChunk_->emitOp(OpCode::OP_SET_LOCAL, loopExpr->line);
        currentChunk_->emitShort(static_cast<uint16_t>(loopSlot), loopExpr->line);
        currentChunk_->emitOp(OpCode::OP_POP, loopExpr->line);

        currentChunk_->writeConstant(Value(""), loopExpr->line);
        std::string accumVar = "$lexpr_acc_" + std::to_string(locals_.size());
        addLocal(accumVar);
        int accumSlot = static_cast<int>(locals_.size() - 1);
        currentChunk_->emitOp(OpCode::OP_SET_LOCAL, loopExpr->line);
        currentChunk_->emitShort(static_cast<uint16_t>(accumSlot), loopExpr->line);
        currentChunk_->emitOp(OpCode::OP_POP, loopExpr->line);

        size_t loopStart = currentChunk_->code.size();
        currentChunk_->emitOp(OpCode::OP_GET_LOCAL, loopExpr->line);
        currentChunk_->emitShort(static_cast<uint16_t>(loopSlot), loopExpr->line);
        currentChunk_->writeConstant(Value(static_cast<int64_t>(0)), loopExpr->line);
        currentChunk_->emitOp(OpCode::OP_GREATER, loopExpr->line);
        size_t exitJump = emitJump(OpCode::OP_JUMP_IF_FALSE, loopExpr->line);
        currentChunk_->emitOp(OpCode::OP_POP, loopExpr->line);

        for (size_t si = 0; si < loopExpr->body->statements.size(); ++si) {
            auto* s = loopExpr->body->statements[si].get();
            if (auto* printStmt = dynamic_cast<PrintStmt*>(s)) {
                currentChunk_->emitOp(OpCode::OP_GET_LOCAL, loopExpr->line);
                currentChunk_->emitShort(static_cast<uint16_t>(accumSlot), loopExpr->line);
                for (const auto& a : printStmt->arguments) {
                    compileExpr(a.get());
                }
                currentChunk_->emitOp(printStmt->silent ? OpCode::OP_SILENT_PRINT : OpCode::OP_PRINT, printStmt->line);
                currentChunk_->emit(static_cast<uint8_t>(printStmt->arguments.size()), printStmt->line);
                currentChunk_->emitOp(OpCode::OP_ADD, loopExpr->line);
                currentChunk_->emitOp(OpCode::OP_SET_LOCAL, loopExpr->line);
                currentChunk_->emitShort(static_cast<uint16_t>(accumSlot), loopExpr->line);
                currentChunk_->emitOp(OpCode::OP_POP, loopExpr->line);
            } else if (auto* exprStmt = dynamic_cast<ExprStmt*>(s)) {
                currentChunk_->emitOp(OpCode::OP_GET_LOCAL, loopExpr->line);
                currentChunk_->emitShort(static_cast<uint16_t>(accumSlot), loopExpr->line);
                compileExpr(exprStmt->expression.get());
                currentChunk_->emitOp(OpCode::OP_ADD, loopExpr->line);
                currentChunk_->emitOp(OpCode::OP_SET_LOCAL, loopExpr->line);
                currentChunk_->emitShort(static_cast<uint16_t>(accumSlot), loopExpr->line);
                currentChunk_->emitOp(OpCode::OP_POP, loopExpr->line);
            } else {
                compileStmt(s);
            }
        }

        currentChunk_->emitOp(OpCode::OP_GET_LOCAL, loopExpr->line);
        currentChunk_->emitShort(static_cast<uint16_t>(loopSlot), loopExpr->line);
        currentChunk_->writeConstant(Value(static_cast<int64_t>(1)), loopExpr->line);
        currentChunk_->emitOp(OpCode::OP_SUB, loopExpr->line);
        currentChunk_->emitOp(OpCode::OP_SET_LOCAL, loopExpr->line);
        currentChunk_->emitShort(static_cast<uint16_t>(loopSlot), loopExpr->line);
        currentChunk_->emitOp(OpCode::OP_POP, loopExpr->line);

        emitLoop(loopStart, loopExpr->line);
        patchJump(exitJump);
        currentChunk_->emitOp(OpCode::OP_POP, loopExpr->line);

        currentChunk_->emitOp(OpCode::OP_GET_LOCAL, loopExpr->line);
        currentChunk_->emitShort(static_cast<uint16_t>(accumSlot), loopExpr->line);
        return;
    }

    if (auto* ifExpr = dynamic_cast<IfExpr*>(expr)) {
        compileExpr(ifExpr->condition.get());
        size_t thenJump = emitJump(OpCode::OP_JUMP_IF_FALSE, ifExpr->line);
        currentChunk_->emitOp(OpCode::OP_POP, ifExpr->line);
        compileBlockAsExpr(ifExpr->thenBranch.get(), ifExpr->line);
        size_t elseJump = emitJump(OpCode::OP_JUMP, ifExpr->line);
        patchJump(thenJump);
        currentChunk_->emitOp(OpCode::OP_POP, ifExpr->line);
        if (ifExpr->elseBranch) {
            compileBlockAsExpr(ifExpr->elseBranch.get(), ifExpr->line);
        } else {
            currentChunk_->emitOp(OpCode::OP_NIL, ifExpr->line);
        }
        patchJump(elseJump);
        return;
    }

    if (auto* fnExpr = dynamic_cast<FnExpr*>(expr)) {
        size_t idx = currentChunk_->addConstant(Value(ValueType::FUNCTION, fnExpr->name));
        currentChunk_->emitOp(OpCode::OP_CONSTANT, fnExpr->line);
        currentChunk_->emitShort(static_cast<uint16_t>(idx), fnExpr->line);
        return;
    }

    if (auto* listLit = dynamic_cast<ListLiteralExpr*>(expr)) {
        for (const auto& el : listLit->elements) {
            compileExpr(el.get());
        }
        currentChunk_->emitOp(OpCode::OP_BUILD_LIST, listLit->line);
        currentChunk_->emitShort(static_cast<uint16_t>(listLit->elements.size()), listLit->line);
        return;
    }

    if (auto* idxExpr = dynamic_cast<IndexExpr*>(expr)) {
        compileExpr(idxExpr->target.get());
        compileExpr(idxExpr->index.get());
        currentChunk_->emitOp(OpCode::OP_GET_INDEX, idxExpr->line);
        return;
    }

    if (auto* bin = dynamic_cast<BinaryExpr*>(expr)) {
        compileExpr(bin->left.get());
        compileExpr(bin->right.get());
        switch (bin->op) {
            case TokenType::PLUS: currentChunk_->emitOp(OpCode::OP_ADD, bin->line); break;
            case TokenType::MINUS: currentChunk_->emitOp(OpCode::OP_SUB, bin->line); break;
            case TokenType::STAR: currentChunk_->emitOp(OpCode::OP_MUL, bin->line); break;
            case TokenType::SLASH: currentChunk_->emitOp(OpCode::OP_DIV, bin->line); break;
            case TokenType::PERCENT: currentChunk_->emitOp(OpCode::OP_MOD, bin->line); break;
            case TokenType::EQUAL_EQUAL: currentChunk_->emitOp(OpCode::OP_EQUAL, bin->line); break;
            case TokenType::BANG_EQUAL: currentChunk_->emitOp(OpCode::OP_NOT_EQUAL, bin->line); break;
            case TokenType::LESS: currentChunk_->emitOp(OpCode::OP_LESS, bin->line); break;
            case TokenType::GREATER: currentChunk_->emitOp(OpCode::OP_GREATER, bin->line); break;
            case TokenType::LESS_EQUAL: currentChunk_->emitOp(OpCode::OP_LESS_EQUAL, bin->line); break;
            case TokenType::GREATER_EQUAL: currentChunk_->emitOp(OpCode::OP_GREATER_EQUAL, bin->line); break;
            default: break;
        }
        return;
    }

    if (auto* un = dynamic_cast<UnaryExpr*>(expr)) {
        compileExpr(un->right.get());
        if (un->op == TokenType::MINUS) currentChunk_->emitOp(OpCode::OP_NEGATE, un->line);
        if (un->op == TokenType::NOT) currentChunk_->emitOp(OpCode::OP_NOT, un->line);
        return;
    }

    if (auto* call = dynamic_cast<CallExpr*>(expr)) {
        for (const auto& a : call->arguments) {
            compileExpr(a.get());
        }
        size_t idx = currentChunk_->addConstant(Value(call->callee));
        currentChunk_->emitOp(OpCode::OP_CALL, call->line);
        currentChunk_->emitShort(static_cast<uint16_t>(idx), call->line);
        currentChunk_->emit(static_cast<uint8_t>(call->arguments.size()), call->line);
        return;
    }

    if (auto* readExpr = dynamic_cast<ReadExpr*>(expr)) {
        compileExpr(readExpr->path.get());
        currentChunk_->emitOp(OpCode::OP_READ, readExpr->line);
        return;
    }

    if (auto* getExpr = dynamic_cast<GetExpr*>(expr)) {
        compileExpr(getExpr->target.get());
        if (getExpr->property) {
            compileExpr(getExpr->property.get());
            currentChunk_->emitOp(OpCode::OP_GET_PROP, getExpr->line);
        } else {
            size_t idx = currentChunk_->addConstant(Value("get"));
            currentChunk_->emitOp(OpCode::OP_CALL, getExpr->line);
            currentChunk_->emitShort(static_cast<uint16_t>(idx), getExpr->line);
            currentChunk_->emit(1, getExpr->line);
        }
        return;
    }

    if (auto* sendExpr = dynamic_cast<SendExpr*>(expr)) {
        compileExpr(sendExpr->target.get());
        compileExpr(sendExpr->data.get());
        size_t idx = currentChunk_->addConstant(Value("send"));
        currentChunk_->emitOp(OpCode::OP_CALL, sendExpr->line);
        currentChunk_->emitShort(static_cast<uint16_t>(idx), sendExpr->line);
        currentChunk_->emit(2, sendExpr->line);
        return;
    }

    if (dynamic_cast<NewExpr*>(expr)) {
        currentChunk_->emitOp(OpCode::OP_NEW, expr->line);
        return;
    }
}
