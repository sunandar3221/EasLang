#include "AotGenerator.hpp"
#include "Lexer.hpp"
#include "Parser.hpp"
#include "StandardLibrary.hpp"
#include <fstream>
#include <cstdlib>

AotGenerator::AotGenerator() : indentLevel_(0), loopCounter_(0) {}

void AotGenerator::collectVariables(ASTNode* node, std::unordered_set<std::string>& vars) {
    if (!node) return;

    if (auto* assign = dynamic_cast<AssignStmt*>(node)) {
        vars.insert(assign->name);
        collectVariables(assign->value.get(), vars);
    } else if (auto* block = dynamic_cast<BlockStmt*>(node)) {
        for (const auto& s : block->statements) {
            if (!dynamic_cast<FnDeclStmt*>(s.get())) {
                collectVariables(s.get(), vars);
            }
        }
    } else if (auto* ifStmt = dynamic_cast<IfStmt*>(node)) {
        collectVariables(ifStmt->condition.get(), vars);
        collectVariables(ifStmt->thenBranch.get(), vars);
        collectVariables(ifStmt->elseBranch.get(), vars);
    } else if (auto* loopStmt = dynamic_cast<LoopStmt*>(node)) {
        collectVariables(loopStmt->count.get(), vars);
        collectVariables(loopStmt->body.get(), vars);
    } else if (auto* whileStmt = dynamic_cast<WhileStmt*>(node)) {
        collectVariables(whileStmt->condition.get(), vars);
        collectVariables(whileStmt->body.get(), vars);
    } else if (auto* ifExpr = dynamic_cast<IfExpr*>(node)) {
        collectVariables(ifExpr->condition.get(), vars);
        collectVariables(ifExpr->thenBranch.get(), vars);
        collectVariables(ifExpr->elseBranch.get(), vars);
    } else if (auto* loopExpr = dynamic_cast<LoopExpr*>(node)) {
        collectVariables(loopExpr->count.get(), vars);
        collectVariables(loopExpr->body.get(), vars);
    }
}

void AotGenerator::resolveImports(BlockStmt* program,
                                  std::vector<std::unique_ptr<FnDeclStmt>>& extraFns,
                                  std::vector<std::unique_ptr<Stmt>>& extraStmts,
                                  std::unordered_set<std::string>& visited) {
    if (!program) return;
    for (auto& s : program->statements) {
        if (auto* useStmt = dynamic_cast<UseStmt*>(s.get())) {
            const std::string& mod = useStmt->moduleName;
            if (mod.empty() || mod == "io" || mod == "math" || mod == "time" ||
                mod == "net" || mod == "http" || mod == "gui" || mod == "str" || mod == "string") {
                continue;
            }
            std::string filename = mod;
            if (filename.size() < 4 || filename.substr(filename.size() - 4) != ".eas") {
                filename += ".eas";
            }
            if (visited.find(filename) != visited.end()) {
                continue;
            }
            visited.insert(filename);

            Value content = StandardLibrary::readFile(filename);
            if (!content.strVal.empty()) {
                Lexer subLex(content.strVal);
                auto subToks = subLex.tokenize();
                if (!subLex.hasErrors()) {
                    Parser subParser(std::move(subToks));
                    auto subAst = subParser.parseProgram();
                    if (subAst) {
                        resolveImports(subAst.get(), extraFns, extraStmts, visited);
                        for (auto& subStmt : subAst->statements) {
                            if (dynamic_cast<FnDeclStmt*>(subStmt.get())) {
                                extraFns.push_back(std::unique_ptr<FnDeclStmt>(static_cast<FnDeclStmt*>(subStmt.release())));
                            } else {
                                extraStmts.push_back(std::move(subStmt));
                            }
                        }
                    }
                }
            }
        }
    }
}

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
                return "(static_cast<bool>(Value(" + generateExpr(bin->left.get()) + ")) && static_cast<bool>(Value(" + generateExpr(bin->right.get()) + ")))";
            case TokenType::OR:
                return "(static_cast<bool>(Value(" + generateExpr(bin->left.get()) + ")) || static_cast<bool>(Value(" + generateExpr(bin->right.get()) + ")))";
            default:
                return "(" + generateExpr(bin->left.get()) + " + " + generateExpr(bin->right.get()) + ")";
        }
    }

    if (auto* un = dynamic_cast<UnaryExpr*>(expr)) {
        if (un->op == TokenType::MINUS) {
            return "(-(" + generateExpr(un->right.get()) + "))";
        }
        if (un->op == TokenType::NOT) {
            return "(!static_cast<bool>(Value(" + generateExpr(un->right.get()) + ")))";
        }
    }

    if (auto* call = dynamic_cast<CallExpr*>(expr)) {
        const std::string& callee = call->callee;
        if (callee == "input" || callee == "io.input" || callee == "io.ask" || callee == "ask") {
            std::string p = call->arguments.empty() ? "\"\"" : "(Value(" + generateExpr(call->arguments[0].get()) + ")).toString()";
            return "StandardLibrary::input(" + p + ")";
        }
        if ((callee == "read" || callee == "io.read") && !call->arguments.empty()) {
            return "StandardLibrary::readFile((Value(" + generateExpr(call->arguments[0].get()) + ")).toString())";
        }
        if ((callee == "write" || callee == "io.write") && call->arguments.size() >= 2) {
            return "StandardLibrary::writeFile((Value(" + generateExpr(call->arguments[0].get()) + ")).toString(), (Value(" + generateExpr(call->arguments[1].get()) + ")).toString())";
        }
        if (callee == "io.print") {
            std::string p = "StandardLibrary::print(std::vector<Value>{";
            for (size_t i = 0; i < call->arguments.size(); ++i) {
                if (i > 0) p += ", ";
                p += "Value(" + generateExpr(call->arguments[i].get()) + ")";
            }
            p += "})";
            return p;
        }
        if (callee == "app" || callee == "gui.app") {
            if (!call->arguments.empty()) {
                return "([&](){ StandardLibrary::setAppTitle((Value(" + generateExpr(call->arguments[0].get()) + ")).toString()); return Value(); })()";
            }
        }
        if (callee == "window" || callee == "gui.window") {
            if (call->arguments.size() >= 2) {
                return "([&](){ StandardLibrary::setWindowSize(static_cast<int>(Value(" + generateExpr(call->arguments[0].get()) + ").asInt()), static_cast<int>(Value(" + generateExpr(call->arguments[1].get()) + ").asInt())); return Value(); })()";
            }
        }
        if (callee == "run" || callee == "gui.run") {
            std::string dur = call->arguments.empty() ? "-1" : "static_cast<int>(Value(" + generateExpr(call->arguments[0].get()) + ").asInt())";
            return "StandardLibrary::runApp(" + dur + ")";
        }
        if (callee == "get" || callee == "http.get" || callee == "net.get") {
            if (call->arguments.size() == 1) {
                return "StandardLibrary::httpGet((Value(" + generateExpr(call->arguments[0].get()) + ")).toString())";
            } else if (call->arguments.size() >= 2) {
                return "(Value(" + generateExpr(call->arguments[0].get()) + ")).getProperty((Value(" + generateExpr(call->arguments[1].get()) + ")).toString())";
            }
        }
        if (callee == "send" || callee == "http.send" || callee == "net.send") {
            if (call->arguments.size() >= 2) {
                return "StandardLibrary::httpSend((Value(" + generateExpr(call->arguments[0].get()) + ")).toString(), (Value(" + generateExpr(call->arguments[1].get()) + ")).toString())";
            }
        }
        if (callee == "silent_print") {
            std::string sp = "StandardLibrary::silentPrint(std::vector<Value>{";
            for (size_t i = 0; i < call->arguments.size(); ++i) {
                if (i > 0) sp += ", ";
                sp += "Value(" + generateExpr(call->arguments[i].get()) + ")";
            }
            sp += "})";
            return sp;
        }

        if (callee == "len" && !call->arguments.empty()) {
            return "([&](){ Value arg = Value(" + generateExpr(call->arguments[0].get()) + "); if (arg.isList()) return Value(static_cast<int64_t>(arg.listVal ? arg.listVal->size() : 0)); if (arg.isString()) return Value(static_cast<int64_t>(arg.strVal.size())); if (arg.isObject()) return Value(static_cast<int64_t>(arg.objVal ? arg.objVal->size() : 0)); return Value(static_cast<int64_t>(0)); })()";
        }
        if (callee == "push" && call->arguments.size() >= 2) {
            return "([&](){ Value target = Value(" + generateExpr(call->arguments[0].get()) + "); Value val = Value(" + generateExpr(call->arguments[1].get()) + "); if (target.isList()) { if (!target.listVal) target.listVal = std::make_shared<std::vector<Value>>(); target.listVal->push_back(val); } return val; })()";
        }
        if (callee == "pop" && !call->arguments.empty()) {
            return "([&](){ Value target = Value(" + generateExpr(call->arguments[0].get()) + "); if (target.isList() && target.listVal && !target.listVal->empty()) { Value popped = target.listVal->back(); target.listVal->pop_back(); return popped; } return Value(); })()";
        }
        if (callee == "str" && !call->arguments.empty()) {
            return "Value((Value(" + generateExpr(call->arguments[0].get()) + ")).toString())";
        }
        if (callee == "int" && !call->arguments.empty()) {
            return "Value((Value(" + generateExpr(call->arguments[0].get()) + ")).asInt())";
        }
        if (callee == "float" && !call->arguments.empty()) {
            return "Value((Value(" + generateExpr(call->arguments[0].get()) + ")).asFloat())";
        }

        // Math functions
        if (callee == "math.sqrt" || callee == "sqrt") {
            std::string arg = call->arguments.empty() ? "0.0" : "Value(" + generateExpr(call->arguments[0].get()) + ").asFloat()";
            return "StandardLibrary::mathSqrt(" + arg + ")";
        }
        if (callee == "math.abs" || callee == "abs") {
            std::string arg = call->arguments.empty() ? "0.0" : "Value(" + generateExpr(call->arguments[0].get()) + ").asFloat()";
            return "StandardLibrary::mathAbs(" + arg + ")";
        }
        if (callee == "math.pow" || callee == "pow") {
            std::string b = call->arguments.empty() ? "0.0" : "Value(" + generateExpr(call->arguments[0].get()) + ").asFloat()";
            std::string e = call->arguments.size() < 2 ? "0.0" : "Value(" + generateExpr(call->arguments[1].get()) + ").asFloat()";
            return "StandardLibrary::mathPow(" + b + ", " + e + ")";
        }
        if (callee == "math.floor" || callee == "floor") {
            std::string arg = call->arguments.empty() ? "0.0" : "Value(" + generateExpr(call->arguments[0].get()) + ").asFloat()";
            return "StandardLibrary::mathFloor(" + arg + ")";
        }
        if (callee == "math.ceil" || callee == "ceil") {
            std::string arg = call->arguments.empty() ? "0.0" : "Value(" + generateExpr(call->arguments[0].get()) + ").asFloat()";
            return "StandardLibrary::mathCeil(" + arg + ")";
        }
        if (callee == "math.round" || callee == "round") {
            std::string arg = call->arguments.empty() ? "0.0" : "Value(" + generateExpr(call->arguments[0].get()) + ").asFloat()";
            return "StandardLibrary::mathRound(" + arg + ")";
        }
        if (callee == "math.min" || callee == "min") {
            std::string a = call->arguments.empty() ? "0.0" : "Value(" + generateExpr(call->arguments[0].get()) + ").asFloat()";
            std::string b = call->arguments.size() < 2 ? "0.0" : "Value(" + generateExpr(call->arguments[1].get()) + ").asFloat()";
            return "StandardLibrary::mathMin(" + a + ", " + b + ")";
        }
        if (callee == "math.max" || callee == "max") {
            std::string a = call->arguments.empty() ? "0.0" : "Value(" + generateExpr(call->arguments[0].get()) + ").asFloat()";
            std::string b = call->arguments.size() < 2 ? "0.0" : "Value(" + generateExpr(call->arguments[1].get()) + ").asFloat()";
            return "StandardLibrary::mathMax(" + a + ", " + b + ")";
        }
        if (callee == "math.random" || callee == "random") {
            return "StandardLibrary::mathRandom()";
        }
        if (callee == "math.sin" || callee == "sin") {
            std::string arg = call->arguments.empty() ? "0.0" : "Value(" + generateExpr(call->arguments[0].get()) + ").asFloat()";
            return "StandardLibrary::mathSin(" + arg + ")";
        }
        if (callee == "math.cos" || callee == "cos") {
            std::string arg = call->arguments.empty() ? "0.0" : "Value(" + generateExpr(call->arguments[0].get()) + ").asFloat()";
            return "StandardLibrary::mathCos(" + arg + ")";
        }
        if (callee == "math.tan" || callee == "tan") {
            std::string arg = call->arguments.empty() ? "0.0" : "Value(" + generateExpr(call->arguments[0].get()) + ").asFloat()";
            return "StandardLibrary::mathTan(" + arg + ")";
        }

        // Time functions
        if (callee == "time.sleep" || callee == "sleep") {
            std::string ms = call->arguments.empty() ? "0" : "Value(" + generateExpr(call->arguments[0].get()) + ").asInt()";
            return "StandardLibrary::timeSleep(" + ms + ")";
        }
        if (callee == "time.now" || callee == "now") {
            return "StandardLibrary::timeNow()";
        }

        if ((callee == "lower" || callee == "to_lower" || callee == "lowercase" || callee == "kecil" || callee == "str.lower") && !call->arguments.empty()) {
            return "StandardLibrary::toLower((Value(" + generateExpr(call->arguments[0].get()) + ")).toString())";
        }
        if ((callee == "upper" || callee == "to_upper" || callee == "uppercase" || callee == "kapital" || callee == "str.upper") && !call->arguments.empty()) {
            return "StandardLibrary::toUpper((Value(" + generateExpr(call->arguments[0].get()) + ")).toString())";
        }
        if (callee == "incase_sensitive" || callee == "incaseSensitive" || callee == "incase_sensitif" ||
            callee == "incaseSensitif" || callee == "incasesensitive" || callee == "incasesensitif" ||
            callee == "incase" || callee == "icase" || callee == "iequals" || callee == "iequal" || callee == "str.incase_sensitive") {
            if (call->arguments.size() == 1) {
                return "StandardLibrary::toLower((Value(" + generateExpr(call->arguments[0].get()) + ")).toString())";
            } else if (call->arguments.size() >= 2) {
                return "StandardLibrary::incaseSensitive((Value(" + generateExpr(call->arguments[0].get()) + ")).toString(), (Value(" + generateExpr(call->arguments[1].get()) + ")).toString())";
            }
            return "Value(false)";
        }
        if (callee == "case_sensitive" || callee == "caseSensitive" || callee == "case_sensitif" ||
            callee == "caseSensitif" || callee == "casesensitive" || callee == "casesensitif" ||
            callee == "case" || callee == "equals" || callee == "equal" || callee == "str.case_sensitive") {
            if (call->arguments.size() == 1) {
                return "Value(" + generateExpr(call->arguments[0].get()) + ")";
            } else if (call->arguments.size() >= 2) {
                return "StandardLibrary::caseSensitive((Value(" + generateExpr(call->arguments[0].get()) + ")).toString(), (Value(" + generateExpr(call->arguments[1].get()) + ")).toString())";
            }
            return "Value(false)";
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
            if (auto* targetVar = dynamic_cast<VarExpr*>(getExpr->target.get())) {
                if (targetVar->name == "math") {
                    if (auto* propLit = dynamic_cast<LiteralExpr*>(getExpr->property.get())) {
                        if (propLit->value.isString()) {
                            if (propLit->value.strVal == "pi") return "Value(3.141592653589793)";
                            if (propLit->value.strVal == "e") return "Value(2.718281828459045)";
                        }
                    }
                }
            }
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

    if (auto* ifExpr = dynamic_cast<IfExpr*>(expr)) {
        std::string cond = generateExpr(ifExpr->condition.get());
        std::string s = "([&]() -> Value {\n";
        s += "    if (" + cond + ") {\n";
        for (size_t i = 0; ifExpr->thenBranch && i < ifExpr->thenBranch->statements.size(); ++i) {
            auto* st = ifExpr->thenBranch->statements[i].get();
            if (i + 1 == ifExpr->thenBranch->statements.size()) {
                if (auto* es = dynamic_cast<ExprStmt*>(st)) {
                    s += "        return Value(" + generateExpr(es->expression.get()) + ");\n";
                } else {
                    std::ostringstream oss;
                    generateStmt(st, oss);
                    s += oss.str();
                    s += "        return Value();\n";
                }
            } else {
                std::ostringstream oss;
                generateStmt(st, oss);
                s += oss.str();
            }
        }
        if (!ifExpr->thenBranch || ifExpr->thenBranch->statements.empty()) {
            s += "        return Value();\n";
        }
        s += "    } else {\n";
        if (ifExpr->elseBranch) {
            for (size_t i = 0; i < ifExpr->elseBranch->statements.size(); ++i) {
                auto* st = ifExpr->elseBranch->statements[i].get();
                if (i + 1 == ifExpr->elseBranch->statements.size()) {
                    if (auto* es = dynamic_cast<ExprStmt*>(st)) {
                        s += "        return Value(" + generateExpr(es->expression.get()) + ");\n";
                    } else {
                        std::ostringstream oss;
                        generateStmt(st, oss);
                        s += oss.str();
                        s += "        return Value();\n";
                    }
                } else {
                    std::ostringstream oss;
                    generateStmt(st, oss);
                    s += oss.str();
                }
            }
        }
        s += "        return Value();\n";
        s += "    }\n";
        s += "})()";
        return s;
    }

    if (auto* loopExpr = dynamic_cast<LoopExpr*>(expr)) {
        std::string cntVar = "_n_" + std::to_string(loopCounter_++);
        std::string idxVar = "_i_" + std::to_string(loopCounter_++);
        std::string cnt = generateExpr(loopExpr->count.get());
        std::string s = "([&]() -> Value {\n";
        s += "    int64_t " + cntVar + " = Value(" + cnt + ").asInt();\n";
        s += "    std::string _acc = \"\";\n";
        s += "    for (int64_t " + idxVar + " = 0; " + idxVar + " < " + cntVar + "; ++" + idxVar + ") {\n";
        for (const auto& st : loopExpr->body->statements) {
            if (auto* ps = dynamic_cast<PrintStmt*>(st.get())) {
                s += "        _acc += (";
                s += ps->silent ? "StandardLibrary::silentPrint(std::vector<Value>{" : "StandardLibrary::print(std::vector<Value>{";
                for (size_t i = 0; i < ps->arguments.size(); ++i) {
                    if (i > 0) s += ", ";
                    s += "Value(" + generateExpr(ps->arguments[i].get()) + ")";
                }
                s += "})).toString();\n";
            } else if (auto* es = dynamic_cast<ExprStmt*>(st.get())) {
                s += "        _acc += (Value(" + generateExpr(es->expression.get()) + ")).toString();\n";
            } else {
                std::ostringstream oss;
                generateStmt(st.get(), oss);
                s += oss.str();
            }
        }
        s += "    }\n";
        s += "    return Value(_acc);\n";
        s += "})()";
        return s;
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
            ss << "Value var_" << assign->name << " = " << generateExpr(assign->value.get()) << ";\n";
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
            ss << "Value var_" << assign->name << " = " << generateExpr(assign->value.get()) << ";\n";
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
        if (printStmt->silent) {
            ss << "StandardLibrary::silentPrint(std::vector<Value>{";
        } else {
            ss << "StandardLibrary::print(std::vector<Value>{";
        }
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
        std::string idxVar = "_loopIdx_" + std::to_string(loopCounter_++);
        emitIndent(ss);
        ss << "for (int64_t " << idxVar << " = 0; " << idxVar << " < static_cast<int64_t>(Value(" << generateExpr(loopStmt->count.get()) << ").asInt()); ++" << idxVar << ") {\n";
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

    if (auto* useStmt = dynamic_cast<UseStmt*>(stmt)) {
        (void)useStmt;
        return;
    }
}

std::string AotGenerator::generateCpp(BlockStmt* program) {
    loopCounter_ = 0;
    std::vector<std::unique_ptr<FnDeclStmt>> extraFns;
    std::vector<std::unique_ptr<Stmt>> extraStmts;
    std::unordered_set<std::string> visitedImports;

    resolveImports(program, extraFns, extraStmts, visitedImports);

    std::ostringstream ss;
    ss << "#include \"Value.hpp\"\n";
    ss << "#include \"StandardLibrary.hpp\"\n";
    ss << "#include <iostream>\n";
    ss << "#include <vector>\n";
    ss << "#include <string>\n";
    ss << "#include <cstdint>\n\n";

    auto emitFnProto = [&](FnDeclStmt* fn) {
        ss << "Value fn_" << fn->name << "(";
        for (size_t i = 0; i < fn->params.size(); ++i) {
            if (i > 0) ss << ", ";
            ss << "Value var_" << fn->params[i];
        }
        ss << ");\n";
    };

    for (const auto& fn : extraFns) {
        emitFnProto(fn.get());
    }
    for (const auto& stmt : program->statements) {
        if (auto* fn = dynamic_cast<FnDeclStmt*>(stmt.get())) {
            emitFnProto(fn);
        }
    }
    ss << "\n";

    auto emitFnDef = [&](FnDeclStmt* fn) {
        declaredVars_.clear();
        for (const auto& p : fn->params) {
            declaredVars_.insert(p);
        }

        std::unordered_set<std::string> fnVars;
        collectVariables(fn->body.get(), fnVars);
        for (const auto& v : fnVars) {
            declaredVars_.insert(v);
        }

        ss << "Value fn_" << fn->name << "(";
        for (size_t i = 0; i < fn->params.size(); ++i) {
            if (i > 0) ss << ", ";
            ss << "Value var_" << fn->params[i];
        }
        ss << ") {\n";
        indentLevel_ = 1;

        std::unordered_set<std::string> paramSet(fn->params.begin(), fn->params.end());
        for (const auto& v : fnVars) {
            if (paramSet.find(v) == paramSet.end()) {
                emitIndent(ss);
                ss << "Value var_" << v << ";\n";
            }
        }

        generateBlock(fn->body.get(), ss, true);
        ss << "}\n\n";
    };

    for (const auto& fn : extraFns) {
        emitFnDef(fn.get());
    }
    for (const auto& stmt : program->statements) {
        if (auto* fn = dynamic_cast<FnDeclStmt*>(stmt.get())) {
            emitFnDef(fn);
        }
    }

    declaredVars_.clear();
    std::unordered_set<std::string> mainVars;
    for (const auto& s : extraStmts) {
        collectVariables(s.get(), mainVars);
    }
    for (const auto& s : program->statements) {
        if (!dynamic_cast<FnDeclStmt*>(s.get())) {
            collectVariables(s.get(), mainVars);
        }
    }
    for (const auto& v : mainVars) {
        declaredVars_.insert(v);
    }

    indentLevel_ = 1;
    ss << "int main(int argc, char** argv) {\n";
    ss << "    (void)argc;\n";
    ss << "    (void)argv;\n";

    for (const auto& v : mainVars) {
        emitIndent(ss);
        ss << "Value var_" << v << ";\n";
    }

    for (const auto& stmt : extraStmts) {
        generateStmt(stmt.get(), ss);
    }

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
#ifdef _WIN32
    std::string cmd = "g++ -std=c++20 -O3 -march=native -flto -static -static-libgcc -static-libstdc++ " + sourceFile + " src/Value.cpp src/StandardLibrary.cpp -Iinclude -lwininet -lgdi32 -luser32 -o " + outputFile;
#else
    std::string cmd = "g++ -std=c++20 -O3 -march=native -flto " + sourceFile + " src/Value.cpp src/StandardLibrary.cpp -Iinclude -o " + outputFile;
#endif
    int res = std::system(cmd.c_str());
    std::remove(sourceFile.c_str());
    return res == 0;
}
