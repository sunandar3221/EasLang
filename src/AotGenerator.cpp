#include "AotGenerator.hpp"
#include <fstream>
#include <cstdlib>

AotGenerator::AotGenerator() : indentLevel_(0) {}

void AotGenerator::emitIndent(std::ostringstream& ss) {
    for (int i = 0; i < indentLevel_; ++i) {
        ss << "    ";
    }
}

std::string AotGenerator::generateExpr(Expr* expr) {
    if (!expr) return "Value()";

    if (auto* lit = dynamic_cast<LiteralExpr*>(expr)) {
        if (lit->value.isInt()) {
            return std::to_string(lit->value.intVal) + "LL";
        }
        if (lit->value.isFloat()) {
            return std::to_string(lit->value.floatVal);
        }
        if (lit->value.isBool()) {
            return lit->value.boolVal ? "true" : "false";
        }
        if (lit->value.isString()) {
            std::string s;
            for (char c : lit->value.strVal) {
                if (c == '"') s += "\\\"";
                else if (c == '\\') s += "\\\\";
                else if (c == '\n') s += "\\n";
                else if (c == '\t') s += "\\t";
                else s += c;
            }
            return "Value(std::string(\"" + s + "\"))";
        }
        return "Value()";
    }

    if (auto* var = dynamic_cast<VarExpr*>(expr)) {
        return "var_" + var->name;
    }

    if (auto* listLit = dynamic_cast<ListLiteralExpr*>(expr)) {
        std::string s = "Value(std::vector<Value>{";
        for (size_t i = 0; i < listLit->elements.size(); ++i) {
            if (i > 0) s += ", ";
            s += "Value(" + generateExpr(listLit->elements[i].get()) + ")";
        }
        s += "})";
        return s;
    }

    if (auto* idx = dynamic_cast<IndexExpr*>(expr)) {
        return "(Value(" + generateExpr(idx->target.get()) + ")).getIndex(Value(" + generateExpr(idx->index.get()) + "))";
    }

    if (auto* bin = dynamic_cast<BinaryExpr*>(expr)) {
        switch (bin->op) {
            case TokenType::PLUS:
                return "(" + generateExpr(bin->left.get()) + " + " + generateExpr(bin->right.get()) + ")";
            case TokenType::MINUS:
                return "(" + generateExpr(bin->left.get()) + " - " + generateExpr(bin->right.get()) + ")";
            case TokenType::STAR:
                return "(" + generateExpr(bin->left.get()) + " * " + generateExpr(bin->right.get()) + ")";
            case TokenType::SLASH:
                return "(" + generateExpr(bin->left.get()) + " / " + generateExpr(bin->right.get()) + ")";
            case TokenType::PERCENT:
                return "(" + generateExpr(bin->left.get()) + " % " + generateExpr(bin->right.get()) + ")";
            case TokenType::EQUAL_EQUAL:
                return "(" + generateExpr(bin->left.get()) + " == " + generateExpr(bin->right.get()) + ")";
            case TokenType::BANG_EQUAL:
                return "(" + generateExpr(bin->left.get()) + " != " + generateExpr(bin->right.get()) + ")";
            case TokenType::LESS:
                return "(" + generateExpr(bin->left.get()) + " < " + generateExpr(bin->right.get()) + ")";
            case TokenType::GREATER:
                return "(" + generateExpr(bin->left.get()) + " > " + generateExpr(bin->right.get()) + ")";
            case TokenType::LESS_EQUAL:
                return "(" + generateExpr(bin->left.get()) + " <= " + generateExpr(bin->right.get()) + ")";
            case TokenType::GREATER_EQUAL:
                return "(" + generateExpr(bin->left.get()) + " >= " + generateExpr(bin->right.get()) + ")";
            case TokenType::AND:
                return "((" + generateExpr(bin->left.get()) + ") && (" + generateExpr(bin->right.get()) + "))";
            case TokenType::OR:
                return "((" + generateExpr(bin->left.get()) + ") || (" + generateExpr(bin->right.get()) + "))";
            default:
                return "(" + generateExpr(bin->left.get()) + " + " + generateExpr(bin->right.get()) + ")";
        }
    }

    if (auto* un = dynamic_cast<UnaryExpr*>(expr)) {
        if (un->op == TokenType::MINUS) {
            return "(-(" + generateExpr(un->right.get()) + "))";
        }
        if (un->op == TokenType::NOT) {
            return "(!(" + generateExpr(un->right.get()) + "))";
        }
    }

    if (auto* call = dynamic_cast<CallExpr*>(expr)) {
        const std::string& callee = call->callee;
        if (callee == "read" && !call->arguments.empty()) {
            return "StandardLibrary::readFile((Value(" + generateExpr(call->arguments[0].get()) + ")).toString())";
        }
        if (callee == "write" && call->arguments.size() >= 2) {
            return "StandardLibrary::writeFile((Value(" + generateExpr(call->arguments[0].get()) + ")).toString(), (Value(" + generateExpr(call->arguments[1].get()) + ")).toString())";
        }
        if (callee == "app" && !call->arguments.empty()) {
            return "([&](){ StandardLibrary::setAppTitle((Value(" + generateExpr(call->arguments[0].get()) + ")).toString()); return Value(); })()";
        }
        if (callee == "window" && call->arguments.size() >= 2) {
            return "([&](){ StandardLibrary::setWindowSize(static_cast<int>(Value(" + generateExpr(call->arguments[0].get()) + ").asInt()), static_cast<int>(Value(" + generateExpr(call->arguments[1].get()) + ").asInt())); return Value(); })()";
        }
        if (callee == "run") {
            std::string dur = call->arguments.empty() ? "-1" : "static_cast<int>(Value(" + generateExpr(call->arguments[0].get()) + ").asInt())";
            return "StandardLibrary::runApp(" + dur + ")";
        }
        if (callee == "get") {
            if (call->arguments.size() == 1) {
                return "StandardLibrary::httpGet((Value(" + generateExpr(call->arguments[0].get()) + ")).toString())";
            } else if (call->arguments.size() >= 2) {
                return "(Value(" + generateExpr(call->arguments[0].get()) + ")).getProperty((Value(" + generateExpr(call->arguments[1].get()) + ")).toString())";
            }
        }
        if (callee == "send" && call->arguments.size() >= 2) {
            return "StandardLibrary::httpSend((Value(" + generateExpr(call->arguments[0].get()) + ")).toString(), (Value(" + generateExpr(call->arguments[1].get()) + ")).toString())";
        }

        std::string s = "fn_" + callee + "(";
        for (size_t i = 0; i < call->arguments.size(); ++i) {
            if (i > 0) s += ", ";
            s += generateExpr(call->arguments[i].get());
        }
        s += ")";
        return s;
    }

    if (auto* readExpr = dynamic_cast<ReadExpr*>(expr)) {
        return "StandardLibrary::readFile((Value(" + generateExpr(readExpr->path.get()) + ")).toString())";
    }

    if (auto* getExpr = dynamic_cast<GetExpr*>(expr)) {
        if (getExpr->property) {
            return "(Value(" + generateExpr(getExpr->target.get()) + ")).getProperty((Value(" + generateExpr(getExpr->property.get()) + ")).toString())";
        }
        return "StandardLibrary::httpGet((Value(" + generateExpr(getExpr->target.get()) + ")).toString())";
    }

    if (auto* sendExpr = dynamic_cast<SendExpr*>(expr)) {
        return "StandardLibrary::httpSend((Value(" + generateExpr(sendExpr->target.get()) + ")).toString(), (Value(" + generateExpr(sendExpr->data.get()) + ")).toString())";
    }

    if (dynamic_cast<NewExpr*>(expr)) {
        return "Value::makeObject()";
    }

    return "Value()";
}

void AotGenerator::generateBlock(BlockStmt* block, std::ostringstream& ss, bool isFunctionBody) {
    if (!block || block->statements.empty()) {
        if (isFunctionBody) {
            emitIndent(ss);
            ss << "return Value();\n";
        }
        return;
    }

    if (!isFunctionBody) {
        for (const auto& stmt : block->statements) {
            generateStmt(stmt.get(), ss);
        }
        return;
    }

    for (size_t i = 0; i + 1 < block->statements.size(); ++i) {
        generateStmt(block->statements[i].get(), ss);
    }
    generateReturnStmt(block->statements.back().get(), ss);
}

void AotGenerator::generateReturnStmt(Stmt* stmt, std::ostringstream& ss) {
    if (!stmt) {
        emitIndent(ss);
        ss << "return Value();\n";
        return;
    }

    if (auto* exprStmt = dynamic_cast<ExprStmt*>(stmt)) {
        emitIndent(ss);
        ss << "return " << generateExpr(exprStmt->expression.get()) << ";\n";
        return;
    }

    if (auto* assign = dynamic_cast<AssignStmt*>(stmt)) {
        emitIndent(ss);
        if (declaredVars_.find(assign->name) == declaredVars_.end()) {
            declaredVars_.insert(assign->name);
            ss << "auto var_" << assign->name << " = " << generateExpr(assign->value.get()) << ";\n";
        } else {
            ss << "var_" << assign->name << " = " << generateExpr(assign->value.get()) << ";\n";
        }
        emitIndent(ss);
        ss << "return var_" << assign->name << ";\n";
        return;
    }

    if (auto* ifStmt = dynamic_cast<IfStmt*>(stmt)) {
        if (ifStmt->elseBranch) {
            emitIndent(ss);
            ss << "if (" << generateExpr(ifStmt->condition.get()) << ") {\n";
            indentLevel_++;
            generateBlock(ifStmt->thenBranch.get(), ss, true);
            indentLevel_--;
            emitIndent(ss);
            ss << "} else {\n";
            indentLevel_++;
            generateBlock(ifStmt->elseBranch.get(), ss, true);
            indentLevel_--;
            emitIndent(ss);
            ss << "}\n";
            return;
        } else {
            emitIndent(ss);
            ss << "if (" << generateExpr(ifStmt->condition.get()) << ") {\n";
            indentLevel_++;
            generateBlock(ifStmt->thenBranch.get(), ss, false);
            indentLevel_--;
            emitIndent(ss);
            ss << "}\n";
            emitIndent(ss);
            ss << "return Value();\n";
            return;
        }
    }

    generateStmt(stmt, ss);
    emitIndent(ss);
    ss << "return Value();\n";
}

void AotGenerator::generateStmt(Stmt* stmt, std::ostringstream& ss) {
    if (!stmt) return;

    if (auto* assign = dynamic_cast<AssignStmt*>(stmt)) {
        emitIndent(ss);
        if (declaredVars_.find(assign->name) == declaredVars_.end()) {
            declaredVars_.insert(assign->name);
            ss << "auto var_" << assign->name << " = " << generateExpr(assign->value.get()) << ";\n";
        } else {
            ss << "var_" << assign->name << " = " << generateExpr(assign->value.get()) << ";\n";
        }
        return;
    }

    if (auto* idxAssign = dynamic_cast<IndexAssignStmt*>(stmt)) {
        emitIndent(ss);
        ss << "(Value(" << generateExpr(idxAssign->target.get()) << ")).setIndex(Value("
           << generateExpr(idxAssign->index.get()) << "), Value("
           << generateExpr(idxAssign->value.get()) << "));\n";
        return;
    }

    if (auto* exprStmt = dynamic_cast<ExprStmt*>(stmt)) {
        emitIndent(ss);
        ss << "(void)(" << generateExpr(exprStmt->expression.get()) << ");\n";
        return;
    }

    if (auto* printStmt = dynamic_cast<PrintStmt*>(stmt)) {
        emitIndent(ss);
        ss << "StandardLibrary::print(std::vector<Value>{";
        for (size_t i = 0; i < printStmt->arguments.size(); ++i) {
            if (i > 0) ss << ", ";
            ss << "Value(" << generateExpr(printStmt->arguments[i].get()) << ")";
        }
        ss << "});\n";
        return;
    }

    if (auto* writeStmt = dynamic_cast<WriteStmt*>(stmt)) {
        emitIndent(ss);
        ss << "StandardLibrary::writeFile((Value(" << generateExpr(writeStmt->path.get()) << ")).toString(), (Value("
           << generateExpr(writeStmt->content.get()) << ")).toString());\n";
        return;
    }

    if (auto* appStmt = dynamic_cast<AppStmt*>(stmt)) {
        emitIndent(ss);
        ss << "StandardLibrary::setAppTitle((Value(" << generateExpr(appStmt->title.get()) << ")).toString());\n";
        return;
    }

    if (auto* winStmt = dynamic_cast<WindowStmt*>(stmt)) {
        emitIndent(ss);
        ss << "StandardLibrary::setWindowSize(static_cast<int>(Value(" << generateExpr(winStmt->width.get()) << ").asInt()), static_cast<int>(Value("
           << generateExpr(winStmt->height.get()) << ").asInt()));\n";
        return;
    }

    if (auto* runStmt = dynamic_cast<RunStmt*>(stmt)) {
        emitIndent(ss);
        if (runStmt->duration) {
            ss << "StandardLibrary::runApp(static_cast<int>(Value(" << generateExpr(runStmt->duration.get()) << ").asInt()));\n";
        } else {
            ss << "StandardLibrary::runApp(-1);\n";
        }
        return;
    }

    if (auto* setStmt = dynamic_cast<SetStmt*>(stmt)) {
        emitIndent(ss);
        ss << "(Value(" << generateExpr(setStmt->target.get()) << ")).setProperty((Value("
           << generateExpr(setStmt->property.get()) << ")).toString(), Value("
           << generateExpr(setStmt->value.get()) << "));\n";
        return;
    }

    if (auto* ifStmt = dynamic_cast<IfStmt*>(stmt)) {
        emitIndent(ss);
        ss << "if (" << generateExpr(ifStmt->condition.get()) << ") {\n";
        indentLevel_++;
        generateBlock(ifStmt->thenBranch.get(), ss);
        indentLevel_--;
        emitIndent(ss);
        ss << "}";
        if (ifStmt->elseBranch) {
            ss << " else {\n";
            indentLevel_++;
            generateBlock(ifStmt->elseBranch.get(), ss);
            indentLevel_--;
            emitIndent(ss);
            ss << "}";
        }
        ss << "\n";
        return;
    }

    if (auto* loopStmt = dynamic_cast<LoopStmt*>(stmt)) {
        emitIndent(ss);
        ss << "for (int64_t _loopIdx = 0; _loopIdx < static_cast<int64_t>(" << generateExpr(loopStmt->count.get()) << "); ++_loopIdx) {\n";
        indentLevel_++;
        generateBlock(loopStmt->body.get(), ss);
        indentLevel_--;
        emitIndent(ss);
        ss << "}\n";
        return;
    }

    if (auto* whileStmt = dynamic_cast<WhileStmt*>(stmt)) {
        emitIndent(ss);
        ss << "while (" << generateExpr(whileStmt->condition.get()) << ") {\n";
        indentLevel_++;
        generateBlock(whileStmt->body.get(), ss);
        indentLevel_--;
        emitIndent(ss);
        ss << "}\n";
        return;
    }
}

std::string AotGenerator::generateCpp(BlockStmt* program) {
    std::ostringstream ss;
    ss << "#include \"Value.hpp\"\n";
    ss << "#include \"StandardLibrary.hpp\"\n";
    ss << "#include <iostream>\n";
    ss << "#include <vector>\n";
    ss << "#include <string>\n";
    ss << "#include <cstdint>\n\n";

    for (const auto& stmt : program->statements) {
        if (auto* fn = dynamic_cast<FnDeclStmt*>(stmt.get())) {
            declaredVars_.clear();
            for (const auto& p : fn->params) {
                declaredVars_.insert(p);
            }
            ss << "auto fn_" << fn->name << "(";
            for (size_t i = 0; i < fn->params.size(); ++i) {
                if (i > 0) ss << ", ";
                ss << "auto var_" << fn->params[i];
            }
            ss << ") {\n";
            indentLevel_ = 1;
            generateBlock(fn->body.get(), ss, true);
            ss << "}\n\n";
        }
    }

    declaredVars_.clear();
    indentLevel_ = 1;
    ss << "int main(int argc, char** argv) {\n";

    for (const auto& stmt : program->statements) {
        if (!dynamic_cast<FnDeclStmt*>(stmt.get())) {
            generateStmt(stmt.get(), ss);
        }
    }

    emitIndent(ss);
    ss << "return 0;\n";
    ss << "}\n";

    return ss.str();
}

bool AotGenerator::buildBinary(const std::string& sourceFile, const std::string& outputFile) {
    std::string cmd = "g++ -std=c++20 -O3 -march=native -flto " + sourceFile + " src/Value.cpp src/StandardLibrary.cpp -Iinclude -lwininet -lgdi32 -luser32 -o " + outputFile;
    int res = std::system(cmd.c_str());
    std::remove(sourceFile.c_str());
    return res == 0;
}
