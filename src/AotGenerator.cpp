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
            return "Value(static_cast<int64_t>(" + std::to_string(lit->value.intVal) + "LL))";
        }
        if (lit->value.isFloat()) {
            return "Value(" + std::to_string(lit->value.floatVal) + ")";
        }
        if (lit->value.isBool()) {
            return lit->value.boolVal ? "Value(true)" : "Value(false)";
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
            s += generateExpr(listLit->elements[i].get());
        }
        s += "})";
        return s;
    }

    if (auto* idx = dynamic_cast<IndexExpr*>(expr)) {
        return "(" + generateExpr(idx->target.get()) + ").getIndex(" + generateExpr(idx->index.get()) + ")";
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
                return "Value(" + generateExpr(bin->left.get()) + " == " + generateExpr(bin->right.get()) + ")";
            case TokenType::BANG_EQUAL:
                return "Value(" + generateExpr(bin->left.get()) + " != " + generateExpr(bin->right.get()) + ")";
            case TokenType::LESS:
                return "Value(" + generateExpr(bin->left.get()) + " < " + generateExpr(bin->right.get()) + ")";
            case TokenType::GREATER:
                return "Value(" + generateExpr(bin->left.get()) + " > " + generateExpr(bin->right.get()) + ")";
            case TokenType::LESS_EQUAL:
                return "Value(" + generateExpr(bin->left.get()) + " <= " + generateExpr(bin->right.get()) + ")";
            case TokenType::GREATER_EQUAL:
                return "Value(" + generateExpr(bin->left.get()) + " >= " + generateExpr(bin->right.get()) + ")";
            case TokenType::AND:
                return "Value((" + generateExpr(bin->left.get()) + ").isTruthy() && (" + generateExpr(bin->right.get()) + ").isTruthy())";
            case TokenType::OR:
                return "Value((" + generateExpr(bin->left.get()) + ").isTruthy() || (" + generateExpr(bin->right.get()) + ").isTruthy())";
            default:
                return "(" + generateExpr(bin->left.get()) + " + " + generateExpr(bin->right.get()) + ")";
        }
    }

    if (auto* un = dynamic_cast<UnaryExpr*>(expr)) {
        if (un->op == TokenType::MINUS) {
            return "(Value(static_cast<int64_t>(0)) - " + generateExpr(un->right.get()) + ")";
        }
        if (un->op == TokenType::NOT) {
            return "Value(!(" + generateExpr(un->right.get()) + ").isTruthy())";
        }
    }

    if (auto* call = dynamic_cast<CallExpr*>(expr)) {
        const std::string& callee = call->callee;
        if (callee == "read" && !call->arguments.empty()) {
            return "StandardLibrary::readFile((" + generateExpr(call->arguments[0].get()) + ").toString())";
        }
        if (callee == "write" && call->arguments.size() >= 2) {
            return "StandardLibrary::writeFile((" + generateExpr(call->arguments[0].get()) + ").toString(), (" + generateExpr(call->arguments[1].get()) + ").toString())";
        }
        if (callee == "app" && !call->arguments.empty()) {
            return "([&](){ StandardLibrary::setAppTitle((" + generateExpr(call->arguments[0].get()) + ").toString()); return Value(); })()";
        }
        if (callee == "window" && call->arguments.size() >= 2) {
            return "([&](){ StandardLibrary::setWindowSize(static_cast<int>((" + generateExpr(call->arguments[0].get()) + ").asInt()), static_cast<int>((" + generateExpr(call->arguments[1].get()) + ").asInt())); return Value(); })()";
        }
        if (callee == "run") {
            std::string dur = call->arguments.empty() ? "-1" : "static_cast<int>((" + generateExpr(call->arguments[0].get()) + ").asInt())";
            return "StandardLibrary::runApp(" + dur + ")";
        }
        if (callee == "get") {
            if (call->arguments.size() == 1) {
                return "StandardLibrary::httpGet((" + generateExpr(call->arguments[0].get()) + ").toString())";
            } else if (call->arguments.size() >= 2) {
                return "(" + generateExpr(call->arguments[0].get()) + ").getProperty((" + generateExpr(call->arguments[1].get()) + ").toString())";
            }
        }
        if (callee == "send" && call->arguments.size() >= 2) {
            return "StandardLibrary::httpSend((" + generateExpr(call->arguments[0].get()) + ").toString(), (" + generateExpr(call->arguments[1].get()) + ").toString())";
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
        return "StandardLibrary::readFile((" + generateExpr(readExpr->path.get()) + ").toString())";
    }

    if (auto* getExpr = dynamic_cast<GetExpr*>(expr)) {
        if (getExpr->property) {
            return "(" + generateExpr(getExpr->target.get()) + ").getProperty((" + generateExpr(getExpr->property.get()) + ").toString())";
        }
        return "StandardLibrary::httpGet((" + generateExpr(getExpr->target.get()) + ").toString())";
    }

    if (auto* sendExpr = dynamic_cast<SendExpr*>(expr)) {
        return "StandardLibrary::httpSend((" + generateExpr(sendExpr->target.get()) + ").toString(), (" + generateExpr(sendExpr->data.get()) + ").toString())";
    }

    if (dynamic_cast<NewExpr*>(expr)) {
        return "Value::makeObject()";
    }

    return "Value()";
}

void AotGenerator::generateBlock(BlockStmt* block, std::ostringstream& ss) {
    if (!block) return;
    for (const auto& stmt : block->statements) {
        generateStmt(stmt.get(), ss);
    }
}

void AotGenerator::generateStmt(Stmt* stmt, std::ostringstream& ss) {
    if (!stmt) return;

    if (auto* assign = dynamic_cast<AssignStmt*>(stmt)) {
        emitIndent(ss);
        if (declaredVars_.find(assign->name) == declaredVars_.end()) {
            declaredVars_.insert(assign->name);
            ss << "Value var_" << assign->name << " = " << generateExpr(assign->value.get()) << ";\n";
        } else {
            ss << "var_" << assign->name << " = " << generateExpr(assign->value.get()) << ";\n";
        }
        emitIndent(ss);
        ss << "_lastVal = var_" << assign->name << ";\n";
        return;
    }

    if (auto* idxAssign = dynamic_cast<IndexAssignStmt*>(stmt)) {
        emitIndent(ss);
        ss << "(" << generateExpr(idxAssign->target.get()) << ").setIndex("
           << generateExpr(idxAssign->index.get()) << ", "
           << generateExpr(idxAssign->value.get()) << ");\n";
        return;
    }

    if (auto* exprStmt = dynamic_cast<ExprStmt*>(stmt)) {
        emitIndent(ss);
        ss << "_lastVal = " << generateExpr(exprStmt->expression.get()) << ";\n";
        return;
    }

    if (auto* printStmt = dynamic_cast<PrintStmt*>(stmt)) {
        emitIndent(ss);
        ss << "StandardLibrary::print(std::vector<Value>{";
        for (size_t i = 0; i < printStmt->arguments.size(); ++i) {
            if (i > 0) ss << ", ";
            ss << generateExpr(printStmt->arguments[i].get());
        }
        ss << "});\n";
        return;
    }

    if (auto* writeStmt = dynamic_cast<WriteStmt*>(stmt)) {
        emitIndent(ss);
        ss << "StandardLibrary::writeFile((" << generateExpr(writeStmt->path.get()) << ").toString(), ("
           << generateExpr(writeStmt->content.get()) << ").toString());\n";
        return;
    }

    if (auto* appStmt = dynamic_cast<AppStmt*>(stmt)) {
        emitIndent(ss);
        ss << "StandardLibrary::setAppTitle((" << generateExpr(appStmt->title.get()) << ").toString());\n";
        return;
    }

    if (auto* winStmt = dynamic_cast<WindowStmt*>(stmt)) {
        emitIndent(ss);
        ss << "StandardLibrary::setWindowSize(static_cast<int>((" << generateExpr(winStmt->width.get()) << ").asInt()), static_cast<int>(("
           << generateExpr(winStmt->height.get()) << ").asInt()));\n";
        return;
    }

    if (auto* runStmt = dynamic_cast<RunStmt*>(stmt)) {
        emitIndent(ss);
        if (runStmt->duration) {
            ss << "StandardLibrary::runApp(static_cast<int>((" << generateExpr(runStmt->duration.get()) << ").asInt()));\n";
        } else {
            ss << "StandardLibrary::runApp(-1);\n";
        }
        return;
    }

    if (auto* setStmt = dynamic_cast<SetStmt*>(stmt)) {
        emitIndent(ss);
        ss << "(" << generateExpr(setStmt->target.get()) << ").setProperty(("
           << generateExpr(setStmt->property.get()) << ").toString(), "
           << generateExpr(setStmt->value.get()) << ");\n";
        return;
    }

    if (auto* ifStmt = dynamic_cast<IfStmt*>(stmt)) {
        emitIndent(ss);
        ss << "if ((" << generateExpr(ifStmt->condition.get()) << ").isTruthy()) {\n";
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
        ss << "for (int64_t _loopIdx = 0; _loopIdx < (" << generateExpr(loopStmt->count.get()) << ").asInt(); ++_loopIdx) {\n";
        indentLevel_++;
        generateBlock(loopStmt->body.get(), ss);
        indentLevel_--;
        emitIndent(ss);
        ss << "}\n";
        return;
    }

    if (auto* whileStmt = dynamic_cast<WhileStmt*>(stmt)) {
        emitIndent(ss);
        ss << "while ((" << generateExpr(whileStmt->condition.get()) << ").isTruthy()) {\n";
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
    ss << "#include <string>\n\n";

    for (const auto& stmt : program->statements) {
        if (auto* fn = dynamic_cast<FnDeclStmt*>(stmt.get())) {
            ss << "Value fn_" << fn->name << "(";
            for (size_t i = 0; i < fn->params.size(); ++i) {
                if (i > 0) ss << ", ";
                ss << "Value var_" << fn->params[i];
            }
            ss << ");\n";
        }
    }
    ss << "\n";

    for (const auto& stmt : program->statements) {
        if (auto* fn = dynamic_cast<FnDeclStmt*>(stmt.get())) {
            declaredVars_.clear();
            for (const auto& p : fn->params) {
                declaredVars_.insert(p);
            }
            ss << "Value fn_" << fn->name << "(";
            for (size_t i = 0; i < fn->params.size(); ++i) {
                if (i > 0) ss << ", ";
                ss << "Value var_" << fn->params[i];
            }
            ss << ") {\n";
            indentLevel_ = 1;
            emitIndent(ss);
            ss << "Value _lastVal;\n";
            generateBlock(fn->body.get(), ss);
            emitIndent(ss);
            ss << "return _lastVal;\n";
            ss << "}\n\n";
        }
    }

    declaredVars_.clear();
    indentLevel_ = 1;
    ss << "int main(int argc, char** argv) {\n";
    emitIndent(ss);
    ss << "Value _lastVal;\n";

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
    std::string cmd = "g++ -O3 -march=native -flto " + sourceFile + " src/Value.cpp src/StandardLibrary.cpp -Iinclude -lwininet -lgdi32 -luser32 -o " + outputFile;
    int res = std::system(cmd.c_str());
    std::remove(sourceFile.c_str());
    return res == 0;
}
