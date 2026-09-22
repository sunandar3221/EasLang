#include "Parser.hpp"
#include "Lexer.hpp"
#include "StandardLibrary.hpp"

Parser::Parser(std::vector<Token> tokens)
    : tokens_(std::move(tokens)), cursor_(0), anonFnCounter_(0) {
    functionArity_["read"] = 1;
    functionArity_["write"] = 2;
    functionArity_["app"] = 1;
    functionArity_["window"] = 2;
    functionArity_["run"] = 0;
    functionArity_["send"] = 2;
    functionArity_["new"] = 0;
    functionArity_["input"] = 1;
    functionArity_["ask"] = 1;
    functionArity_["io.input"] = 1;
    functionArity_["io.ask"] = 1;
    functionArity_["io.read"] = 1;
    functionArity_["io.write"] = 2;
    functionArity_["io.print"] = 1;
    functionArity_["math.sqrt"] = 1;
    functionArity_["math.abs"] = 1;
    functionArity_["math.pow"] = 2;
    functionArity_["math.floor"] = 1;
    functionArity_["math.ceil"] = 1;
    functionArity_["math.round"] = 1;
    functionArity_["math.min"] = 2;
    functionArity_["math.max"] = 2;
    functionArity_["math.random"] = 0;
    functionArity_["random"] = 0;
    functionArity_["math.random_seed"] = 1;
    functionArity_["math.randomSeed"] = 1;
    functionArity_["math.seed"] = 1;
    functionArity_["random_seed"] = 1;
    functionArity_["seed"] = 1;
    functionArity_["math.sin"] = 1;
    functionArity_["math.cos"] = 1;
    functionArity_["math.tan"] = 1;
    functionArity_["time.sleep"] = 1;
    functionArity_["time.now"] = 0;
    functionArity_["net.get"] = 1;
    functionArity_["net.send"] = 2;
    functionArity_["http.get"] = 1;
    functionArity_["http.send"] = 2;
    functionArity_["gui.app"] = 1;
    functionArity_["gui.window"] = 2;
    functionArity_["gui.run"] = 0;
    functionArity_["len"] = 1;
    functionArity_["push"] = 2;
    functionArity_["pop"] = 1;
    functionArity_["str"] = 1;
    functionArity_["int"] = 1;
    functionArity_["float"] = 1;
    functionArity_["lower"] = 1;
    functionArity_["to_lower"] = 1;
    functionArity_["lowercase"] = 1;
    functionArity_["kecil"] = 1;
    functionArity_["upper"] = 1;
    functionArity_["to_upper"] = 1;
    functionArity_["uppercase"] = 1;
    functionArity_["kapital"] = 1;
    functionArity_["case_sensitive"] = 2;
    functionArity_["caseSensitive"] = 2;
    functionArity_["case_sensitif"] = 2;
    functionArity_["caseSensitif"] = 2;
    functionArity_["casesensitive"] = 2;
    functionArity_["casesensitif"] = 2;
    functionArity_["case"] = 2;
    functionArity_["incase_sensitive"] = 2;
    functionArity_["incaseSensitive"] = 2;
    functionArity_["incase_sensitif"] = 2;
    functionArity_["incaseSensitif"] = 2;
    functionArity_["incasesensitive"] = 2;
    functionArity_["incasesensitif"] = 2;
    functionArity_["incase"] = 2;
    functionArity_["icase"] = 2;
    functionArity_["iequals"] = 2;
    functionArity_["iequal"] = 2;
    functionArity_["str.lower"] = 1;
    functionArity_["str.upper"] = 1;
    functionArity_["str.case_sensitive"] = 2;
    functionArity_["str.incase_sensitive"] = 2;
}

bool Parser::isAtEnd() const {
    return cursor_ >= tokens_.size() || tokens_[cursor_].type == TokenType::END_OF_FILE;
}

const Token& Parser::peek() const {
    if (cursor_ >= tokens_.size()) return tokens_.back();
    return tokens_[cursor_];
}

const Token& Parser::peekNext() const {
    if (cursor_ + 1 >= tokens_.size()) return tokens_.back();
    return tokens_[cursor_ + 1];
}

const Token& Parser::previous() const {
    if (cursor_ == 0) return tokens_[0];
    return tokens_[cursor_ - 1];
}

Token Parser::advance() {
    if (!isAtEnd()) cursor_++;
    return previous();
}

bool Parser::check(TokenType type) const {
    if (isAtEnd()) return type == TokenType::END_OF_FILE;
    return peek().type == type;
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

void Parser::skipNewlines() {
    while (check(TokenType::NEWLINE)) {
        advance();
    }
}

bool Parser::isIndexOrPropAssign() const {
    if (cursor_ >= tokens_.size() || tokens_[cursor_].type != TokenType::IDENTIFIER) return false;
    size_t i = cursor_ + 1;
    bool hasAccess = false;
    while (i < tokens_.size()) {
        if (tokens_[i].type == TokenType::LBRACKET) {
            hasAccess = true;
            int depth = 1;
            i++;
            while (i < tokens_.size() && depth > 0) {
                if (tokens_[i].type == TokenType::LBRACKET) depth++;
                else if (tokens_[i].type == TokenType::RBRACKET) depth--;
                i++;
            }
        } else if (tokens_[i].type == TokenType::DOT) {
            hasAccess = true;
            i++;
            if (i < tokens_.size() && tokens_[i].type == TokenType::IDENTIFIER) {
                i++;
            } else {
                return false;
            }
        } else {
            break;
        }
    }
    return hasAccess && (i < tokens_.size() && tokens_[i].type == TokenType::ASSIGN);
}

void Parser::reportError(const std::string& message, const Token& token) {
    std::string loc = "[Line " + std::to_string(token.line) + ", Col " + std::to_string(token.column) + "]";
    if (token.type == TokenType::END_OF_FILE) {
        errors_.push_back("Syntax Error " + loc + ": " + message + " at end of file.");
    } else if (token.lexeme.empty()) {
        errors_.push_back("Syntax Error " + loc + ": " + message + ".");
    } else {
        errors_.push_back("Syntax Error " + loc + ": " + message + " near '" + token.lexeme + "'.");
    }
}

std::unique_ptr<BlockStmt> Parser::parseProgram() {
    auto block = std::make_unique<BlockStmt>();
    while (!isAtEnd()) {
        skipNewlines();
        if (isAtEnd()) break;
        size_t prevCursor = cursor_;
        auto stmt = parseStatement();
        if (stmt) {
            block->statements.push_back(std::move(stmt));
        }
        if (cursor_ == prevCursor && !isAtEnd()) {
            advance();
        }
    }
    for (auto& fn : hoistedAnonFns_) {
        block->statements.insert(block->statements.begin(), std::move(fn));
    }
    hoistedAnonFns_.clear();
    return block;
}

std::unique_ptr<BlockStmt> Parser::parseBlock() {
    auto block = std::make_unique<BlockStmt>();
    skipNewlines();

    if (match(TokenType::INDENT)) {
        while (!isAtEnd() && !check(TokenType::DEDENT)) {
            skipNewlines();
            if (check(TokenType::DEDENT) || isAtEnd()) break;
            if (check(TokenType::END)) {
                advance();
                break;
            }
            size_t prevCursor = cursor_;
            auto stmt = parseStatement();
            if (stmt) {
                block->statements.push_back(std::move(stmt));
            }
            if (cursor_ == prevCursor && !isAtEnd()) {
                advance();
            }
        }
        match(TokenType::DEDENT);
    } else {
        while (!isAtEnd() && !check(TokenType::END) && !check(TokenType::ELSE) && !check(TokenType::ELIF) && !check(TokenType::DEDENT)) {
            skipNewlines();
            if (check(TokenType::END) || check(TokenType::ELSE) || check(TokenType::ELIF) || isAtEnd()) break;
            size_t prevCursor = cursor_;
            auto stmt = parseStatement();
            if (stmt) {
                block->statements.push_back(std::move(stmt));
            }
            if (cursor_ == prevCursor && !isAtEnd()) {
                advance();
            }
        }
        match(TokenType::END);
    }

    return block;
}

std::unique_ptr<Stmt> Parser::parseStatement() {
    skipNewlines();
    if (isAtEnd()) return nullptr;

    switch (peek().type) {
        case TokenType::PRINT: return parsePrint(false);
        case TokenType::SILENT_PRINT: return parsePrint(true);
        case TokenType::WRITE: return parseWrite();
        case TokenType::APP: return parseApp();
        case TokenType::WINDOW: return parseWindow();
        case TokenType::RUN: return parseRun();
        case TokenType::SET: return parseSet();
        case TokenType::USE: return parseUse();
        case TokenType::IF: return parseIf();
        case TokenType::LOOP: return parseLoop();
        case TokenType::WHILE: return parseWhile();
        case TokenType::FN: return parseFnDecl();
        case TokenType::RETURN: return parseReturn();
        case TokenType::BREAK: return parseBreak();
        case TokenType::CONTINUE: return parseContinue();
        case TokenType::ELSE: {
            reportError("Unexpected 'else' without matching 'if'", peek());
            advance();
            return nullptr;
        }
        case TokenType::ELIF: {
            reportError("Unexpected 'elif' without matching 'if'", peek());
            advance();
            return nullptr;
        }
        case TokenType::END: {
            advance();
            return nullptr;
        }
        case TokenType::DEDENT: {
            advance();
            return nullptr;
        }
        default: return parseAssignmentOrExpr();
    }
}

std::unique_ptr<Stmt> Parser::parsePrint(bool silent) {
    int line = advance().line;
    std::vector<std::unique_ptr<Expr>> args;

    if (match(TokenType::LPAREN)) {
        if (!check(TokenType::RPAREN)) {
            do {
                skipNewlines();
                if (check(TokenType::RPAREN) || isAtEnd()) break;
                auto expr = parseExpression();
                if (expr) {
                    args.push_back(std::move(expr));
                }
                skipNewlines();
            } while (match(TokenType::COMMA));
        }
        if (!match(TokenType::RPAREN)) {
            reportError("Expected closing ')' after print arguments", peek());
        }
    } else {
        while (!check(TokenType::NEWLINE) && !check(TokenType::DEDENT) && !check(TokenType::END) && !isAtEnd()) {
            match(TokenType::COMMA);
            if (check(TokenType::NEWLINE) || check(TokenType::DEDENT) || check(TokenType::END) || isAtEnd()) break;
            auto expr = parseExpression();
            if (expr) {
                args.push_back(std::move(expr));
            }
            match(TokenType::COMMA);
        }
    }
    match(TokenType::NEWLINE);
    return std::make_unique<PrintStmt>(std::move(args), line, 0, silent);
}

std::unique_ptr<Stmt> Parser::parseWrite() {
    int line = advance().line;
    std::unique_ptr<Expr> path;
    std::unique_ptr<Expr> content;
    if (match(TokenType::LPAREN)) {
        path = parseExpression();
        match(TokenType::COMMA);
        content = parseExpression();
        if (!match(TokenType::RPAREN)) {
            reportError("Expected closing ')' in write", peek());
        }
    } else {
        path = parseUnary();
        match(TokenType::COMMA);
        content = parseExpression();
    }
    match(TokenType::NEWLINE);
    return std::make_unique<WriteStmt>(std::move(path), std::move(content), line);
}

std::unique_ptr<Stmt> Parser::parseApp() {
    int line = advance().line;
    std::unique_ptr<Expr> title;
    if (match(TokenType::LPAREN)) {
        title = parseExpression();
        if (!match(TokenType::RPAREN)) {
            reportError("Expected closing ')' in app", peek());
        }
    } else {
        title = parseExpression();
    }
    match(TokenType::NEWLINE);
    return std::make_unique<AppStmt>(std::move(title), line);
}

std::unique_ptr<Stmt> Parser::parseWindow() {
    int line = advance().line;
    std::unique_ptr<Expr> width;
    std::unique_ptr<Expr> height;
    if (match(TokenType::LPAREN)) {
        width = parseExpression();
        match(TokenType::COMMA);
        height = parseExpression();
        if (!match(TokenType::RPAREN)) {
            reportError("Expected closing ')' in window", peek());
        }
    } else {
        width = parseUnary();
        match(TokenType::COMMA);
        height = parseUnary();
    }
    match(TokenType::NEWLINE);
    return std::make_unique<WindowStmt>(std::move(width), std::move(height), line);
}

std::unique_ptr<Stmt> Parser::parseRun() {
    int line = advance().line;
    std::unique_ptr<Expr> duration = nullptr;
    if (match(TokenType::LPAREN)) {
        if (!check(TokenType::RPAREN)) {
            duration = parseExpression();
        }
        if (!match(TokenType::RPAREN)) {
            reportError("Expected closing ')' in run", peek());
        }
    } else if (!check(TokenType::NEWLINE) && !check(TokenType::DEDENT) && !check(TokenType::END) && !isAtEnd()) {
        duration = parseExpression();
    }
    match(TokenType::NEWLINE);
    return std::make_unique<RunStmt>(std::move(duration), line);
}

std::unique_ptr<Stmt> Parser::parseSet() {
    int line = advance().line;
    auto target = parseUnary();
    match(TokenType::COMMA);
    auto prop = parseUnary();
    match(TokenType::COMMA);
    auto val = parseExpression();
    match(TokenType::NEWLINE);
    return std::make_unique<SetStmt>(std::move(target), std::move(prop), std::move(val), line);
}

std::unique_ptr<Stmt> Parser::parseUse() {
    int line = advance().line;
    std::string mod;
    if (check(TokenType::STRING) || check(TokenType::IDENTIFIER)) {
        mod = advance().lexeme;
    }
    match(TokenType::NEWLINE);
    if (!mod.empty() && mod != "io" && mod != "math" && mod != "time" && mod != "net" && mod != "http" && mod != "gui" && mod != "str" && mod != "string") {
        std::string filename = mod;
        if (filename.size() < 4 || filename.substr(filename.size() - 4) != ".eas") {
            filename += ".eas";
        }
        Value content = StandardLibrary::readFile(filename);
        if (!content.strVal.empty()) {
            Lexer subLex(content.strVal);
            auto subToks = subLex.tokenize();
            if (!subLex.hasErrors()) {
                Parser subParser(std::move(subToks));
                auto subAst = subParser.parseProgram();
                if (subAst) {
                    for (const auto& s : subAst->statements) {
                        if (auto* fn = dynamic_cast<FnDeclStmt*>(s.get())) {
                            functionArity_[fn->name] = static_cast<int>(fn->params.size());
                        }
                    }
                }
            }
        }
    }
    return std::make_unique<UseStmt>(std::move(mod), line);
}

std::unique_ptr<Stmt> Parser::parseIf() {
    Token keywordTok = advance();
    int line = keywordTok.line;
    auto condition = parseExpression();
    if (!condition) {
        reportError("Expected condition expression after '" + keywordTok.lexeme + "'", peek());
    }
    if (check(TokenType::NEWLINE)) advance();
    auto thenBranch = parseBlock();
    skipNewlines();
    std::unique_ptr<BlockStmt> elseBranch = nullptr;
    if (check(TokenType::ELIF)) {
        auto ifStmt = parseIf();
        auto blk = std::make_unique<BlockStmt>();
        blk->statements.push_back(std::move(ifStmt));
        elseBranch = std::move(blk);
    } else if (match(TokenType::ELSE)) {
        if (check(TokenType::IF) || check(TokenType::ELIF)) {
            auto ifStmt = parseIf();
            auto blk = std::make_unique<BlockStmt>();
            blk->statements.push_back(std::move(ifStmt));
            elseBranch = std::move(blk);
        } else {
            if (check(TokenType::NEWLINE)) advance();
            elseBranch = parseBlock();
        }
    }
    skipNewlines();
    match(TokenType::END);
    return std::make_unique<IfStmt>(std::move(condition), std::move(thenBranch), std::move(elseBranch), line);
}

std::unique_ptr<Stmt> Parser::parseLoop() {
    int line = advance().line;
    auto count = parseExpression();
    match(TokenType::NEWLINE);
    auto body = parseBlock();
    skipNewlines();
    match(TokenType::END);
    return std::make_unique<LoopStmt>(std::move(count), std::move(body), line);
}

std::unique_ptr<Stmt> Parser::parseWhile() {
    int line = advance().line;
    auto condition = parseExpression();
    match(TokenType::NEWLINE);
    auto body = parseBlock();
    skipNewlines();
    match(TokenType::END);
    return std::make_unique<WhileStmt>(std::move(condition), std::move(body), line);
}

std::unique_ptr<Stmt> Parser::parseFnDecl() {
    int line = advance().line;
    std::string name = advance().lexeme;
    std::vector<std::string> params;
    while (check(TokenType::IDENTIFIER)) {
        params.push_back(advance().lexeme);
    }
    functionArity_[name] = static_cast<int>(params.size());
    match(TokenType::NEWLINE);
    auto body = parseBlock();
    skipNewlines();
    match(TokenType::END);
    return std::make_unique<FnDeclStmt>(std::move(name), std::move(params), std::move(body), line);
}

std::unique_ptr<Stmt> Parser::parseReturn() {
    int line = advance().line;
    std::unique_ptr<Expr> value = nullptr;
    if (!check(TokenType::NEWLINE) && !check(TokenType::DEDENT) && !check(TokenType::END) && !isAtEnd()) {
        value = parseExpression();
    }
    if (check(TokenType::NEWLINE)) advance();
    return std::make_unique<ReturnStmt>(std::move(value), line);
}

std::unique_ptr<Stmt> Parser::parseBreak() {
    int line = advance().line;
    if (check(TokenType::NEWLINE)) advance();
    return std::make_unique<BreakStmt>(line);
}

std::unique_ptr<Stmt> Parser::parseContinue() {
    int line = advance().line;
    if (check(TokenType::NEWLINE)) advance();
    return std::make_unique<ContinueStmt>(line);
}

std::unique_ptr<Stmt> Parser::parseAssignmentOrExpr() {
    int line = peek().line;
    if (check(TokenType::IDENTIFIER) && peekNext().type == TokenType::ASSIGN) {
        std::string name = advance().lexeme;
        advance();
        skipNewlines();
        if (check(TokenType::LOOP)) {
            int loopLine = advance().line;
            auto count = parseExpression();
            match(TokenType::NEWLINE);
            auto body = parseBlock();
            return std::make_unique<AssignStmt>(std::move(name), std::make_unique<LoopExpr>(std::move(count), std::move(body), loopLine), line);
        }
        if (check(TokenType::IF)) {
            int ifLine = advance().line;
            auto condition = parseExpression();
            match(TokenType::NEWLINE);
            auto thenBranch = parseBlock();
            skipNewlines();
            std::unique_ptr<BlockStmt> elseBranch = nullptr;
            if (match(TokenType::ELSE)) {
                match(TokenType::NEWLINE);
                elseBranch = parseBlock();
            }
            return std::make_unique<AssignStmt>(std::move(name), std::make_unique<IfExpr>(std::move(condition), std::move(thenBranch), std::move(elseBranch), ifLine), line);
        }
        if (check(TokenType::FN)) {
            int fnLine = advance().line;
            std::vector<std::string> params;
            while (check(TokenType::IDENTIFIER)) {
                params.push_back(advance().lexeme);
            }
            std::string fnName = "$anon_" + std::to_string(anonFnCounter_++);
            functionArity_[fnName] = static_cast<int>(params.size());
            functionArity_[name] = static_cast<int>(params.size());
            match(TokenType::NEWLINE);
            auto body = parseBlock();
            hoistedAnonFns_.push_back(std::make_unique<FnDeclStmt>(fnName, params, std::make_unique<BlockStmt>(std::move(body->statements)), fnLine));
            return std::make_unique<AssignStmt>(std::move(name), std::make_unique<VarExpr>(fnName, fnLine), line);
        }
        auto val = parseExpression();
        if (auto* varExp = dynamic_cast<VarExpr*>(val.get())) {
            auto it = functionArity_.find(varExp->name);
            if (it != functionArity_.end()) {
                functionArity_[name] = it->second;
            }
        }
        match(TokenType::NEWLINE);
        return std::make_unique<AssignStmt>(std::move(name), std::move(val), line);
    }

    if (isIndexOrPropAssign()) {
        auto primary = parseCallOrPrimary();
        primary = parsePostfix(std::move(primary));
        if (auto* idxExpr = dynamic_cast<IndexExpr*>(primary.get())) {
            if (match(TokenType::ASSIGN)) {
                auto val = parseExpression();
                match(TokenType::NEWLINE);
                return std::make_unique<IndexAssignStmt>(std::move(idxExpr->target), std::move(idxExpr->index), std::move(val), line);
            }
        }
        if (auto* getExpr = dynamic_cast<GetExpr*>(primary.get())) {
            if (match(TokenType::ASSIGN)) {
                auto val = parseExpression();
                match(TokenType::NEWLINE);
                return std::make_unique<SetStmt>(std::move(getExpr->target), std::move(getExpr->property), std::move(val), line);
            }
        }
    }

    auto expr = parseExpression();
    if (auto* var = dynamic_cast<VarExpr*>(expr.get())) {
        auto it = functionArity_.find(var->name);
        if (it != functionArity_.end() && it->second == 0) {
            expr = std::make_unique<CallExpr>(var->name, std::vector<std::unique_ptr<Expr>>{}, var->line, var->column);
        }
    }
    match(TokenType::NEWLINE);
    return std::make_unique<ExprStmt>(std::move(expr), line);
}

std::unique_ptr<Expr> Parser::parseExpression() {
    return parseLogicalOr();
}

std::unique_ptr<Expr> Parser::parseLogicalOr() {
    auto expr = parseLogicalAnd();
    while (match(TokenType::OR)) {
        Token opTok = previous();
        auto right = parseLogicalAnd();
        expr = std::make_unique<BinaryExpr>(std::move(expr), opTok.type, std::move(right), opTok.line, opTok.column);
    }
    return expr;
}

std::unique_ptr<Expr> Parser::parseLogicalAnd() {
    auto expr = parseEquality();
    while (match(TokenType::AND)) {
        Token opTok = previous();
        auto right = parseEquality();
        expr = std::make_unique<BinaryExpr>(std::move(expr), opTok.type, std::move(right), opTok.line, opTok.column);
    }
    return expr;
}

std::unique_ptr<Expr> Parser::parseEquality() {
    auto expr = parseRelational();
    while (match(TokenType::EQUAL_EQUAL) || match(TokenType::BANG_EQUAL)) {
        Token opTok = previous();
        auto right = parseRelational();
        expr = std::make_unique<BinaryExpr>(std::move(expr), opTok.type, std::move(right), opTok.line, opTok.column);
    }
    return expr;
}

std::unique_ptr<Expr> Parser::parseRelational() {
    auto expr = parseAdditive();
    while (match(TokenType::LESS) || match(TokenType::GREATER) ||
           match(TokenType::LESS_EQUAL) || match(TokenType::GREATER_EQUAL)) {
        Token opTok = previous();
        auto right = parseAdditive();
        expr = std::make_unique<BinaryExpr>(std::move(expr), opTok.type, std::move(right), opTok.line, opTok.column);
    }
    return expr;
}

std::unique_ptr<Expr> Parser::parseAdditive() {
    auto expr = parseMultiplicative();
    while (match(TokenType::PLUS) || match(TokenType::MINUS)) {
        Token opTok = previous();
        auto right = parseMultiplicative();
        expr = std::make_unique<BinaryExpr>(std::move(expr), opTok.type, std::move(right), opTok.line, opTok.column);
    }
    return expr;
}

std::unique_ptr<Expr> Parser::parseMultiplicative() {
    auto expr = parseUnary();
    while (match(TokenType::STAR) || match(TokenType::SLASH) || match(TokenType::PERCENT)) {
        Token opTok = previous();
        auto right = parseUnary();
        expr = std::make_unique<BinaryExpr>(std::move(expr), opTok.type, std::move(right), opTok.line, opTok.column);
    }
    return expr;
}

std::unique_ptr<Expr> Parser::parseUnary() {
    if (match(TokenType::MINUS) || match(TokenType::NOT)) {
        Token opTok = previous();
        auto right = parseUnary();
        return std::make_unique<UnaryExpr>(opTok.type, std::move(right), opTok.line, opTok.column);
    }
    auto primary = parseCallOrPrimary();
    return parsePostfix(std::move(primary));
}

std::unique_ptr<Expr> Parser::parsePostfix(std::unique_ptr<Expr> expr) {
    while (true) {
        if (match(TokenType::LBRACKET)) {
            auto index = parseExpression();
            match(TokenType::RBRACKET);
            expr = std::make_unique<IndexExpr>(std::move(expr), std::move(index));
        } else if (match(TokenType::DOT)) {
            std::string prop = advance().lexeme;
            if (match(TokenType::LPAREN)) {
                std::vector<std::unique_ptr<Expr>> args;
                if (!check(TokenType::RPAREN)) {
                    do {
                        skipNewlines();
                        if (check(TokenType::RPAREN) || isAtEnd()) break;
                        args.push_back(parseExpression());
                        skipNewlines();
                    } while (match(TokenType::COMMA));
                }
                match(TokenType::RPAREN);
                if (auto* var = dynamic_cast<VarExpr*>(expr.get())) {
                    expr = std::make_unique<CallExpr>(var->name + "." + prop, std::move(args));
                } else {
                    expr = std::make_unique<GetExpr>(std::move(expr), std::make_unique<LiteralExpr>(Value(prop)));
                }
            } else {
                expr = std::make_unique<GetExpr>(std::move(expr), std::make_unique<LiteralExpr>(Value(prop)));
            }
        } else {
            break;
        }
    }
    return expr;
}

std::unique_ptr<Expr> Parser::parseCallOrPrimary() {
    if (match(TokenType::SILENT_PRINT)) {
        int line = previous().line;
        std::vector<std::unique_ptr<Expr>> args;
        while (!check(TokenType::NEWLINE) && !check(TokenType::DEDENT) && !check(TokenType::END) &&
               !check(TokenType::RPAREN) && !check(TokenType::RBRACKET) && !isAtEnd()) {
            match(TokenType::COMMA);
            if (check(TokenType::NEWLINE) || check(TokenType::DEDENT) || check(TokenType::END) ||
                check(TokenType::RPAREN) || check(TokenType::RBRACKET) || isAtEnd()) break;
            auto expr = parseExpression();
            if (expr) {
                args.push_back(std::move(expr));
            }
            match(TokenType::COMMA);
        }
        return std::make_unique<CallExpr>("silent_print", std::move(args), line);
    }

    if (match(TokenType::READ)) {
        auto path = parseUnary();
        return std::make_unique<ReadExpr>(std::move(path));
    }

    if (match(TokenType::NEW)) {
        std::string typeName;
        if (check(TokenType::IDENTIFIER)) {
            typeName = advance().lexeme;
        }
        return std::make_unique<NewExpr>(std::move(typeName));
    }

    if (match(TokenType::GET)) {
        auto target = parseUnary();
        std::unique_ptr<Expr> prop = nullptr;
        if (!check(TokenType::NEWLINE) && !check(TokenType::DEDENT) && !check(TokenType::END) &&
            !check(TokenType::RPAREN) && !check(TokenType::RBRACKET) && !check(TokenType::COMMA) &&
            !check(TokenType::PLUS) && !check(TokenType::MINUS) && !check(TokenType::STAR) &&
            !check(TokenType::SLASH) && !check(TokenType::PERCENT) && !check(TokenType::EQUAL_EQUAL) &&
            !check(TokenType::BANG_EQUAL) && !check(TokenType::LESS) && !check(TokenType::GREATER) &&
            !check(TokenType::LESS_EQUAL) && !check(TokenType::GREATER_EQUAL) && !check(TokenType::AND) &&
            !check(TokenType::OR) && !isAtEnd()) {
            prop = parseUnary();
        }
        return std::make_unique<GetExpr>(std::move(target), std::move(prop));
    }

    if (match(TokenType::SEND)) {
        auto target = parseUnary();
        auto data = parseUnary();
        return std::make_unique<SendExpr>(std::move(target), std::move(data));
    }

    if (check(TokenType::IDENTIFIER)) {
        Token idTok = advance();
        std::string id = idTok.lexeme;
        int line = idTok.line;
        int col = idTok.column;

        if (match(TokenType::DOT)) {
            if (!isAtEnd() && peek().type != TokenType::NEWLINE && peek().type != TokenType::DEDENT &&
                peek().type != TokenType::END && peek().type != TokenType::END_OF_FILE &&
                peek().type != TokenType::COMMA && peek().type != TokenType::RPAREN && peek().type != TokenType::RBRACKET) {
                Token propTok = advance();
                std::string fullName = id + "." + propTok.lexeme;
                auto it = functionArity_.find(fullName);
                int arity = (it != functionArity_.end()) ? it->second : -1;

                if (fullName == "io.print") {
                    if (match(TokenType::LPAREN)) {
                        std::vector<std::unique_ptr<Expr>> args;
                        if (!check(TokenType::RPAREN)) {
                            do {
                                skipNewlines();
                                if (check(TokenType::RPAREN) || isAtEnd()) break;
                                args.push_back(parseExpression());
                                skipNewlines();
                            } while (match(TokenType::COMMA));
                        }
                        match(TokenType::RPAREN);
                        return std::make_unique<CallExpr>(fullName, std::move(args), line, col);
                    }
                    std::vector<std::unique_ptr<Expr>> args;
                    while (!check(TokenType::NEWLINE) && !check(TokenType::DEDENT) && !check(TokenType::END) &&
                           !check(TokenType::RPAREN) && !check(TokenType::RBRACKET) && !isAtEnd()) {
                        match(TokenType::COMMA);
                        if (check(TokenType::NEWLINE) || check(TokenType::DEDENT) || check(TokenType::END) ||
                            check(TokenType::RPAREN) || check(TokenType::RBRACKET) || isAtEnd()) break;
                        auto expr = parseExpression();
                        if (expr) {
                            args.push_back(std::move(expr));
                        }
                        match(TokenType::COMMA);
                    }
                    return std::make_unique<CallExpr>(fullName, std::move(args), line, col);
                }

                if (match(TokenType::LPAREN)) {
                    std::vector<std::unique_ptr<Expr>> args;
                    if (!check(TokenType::RPAREN)) {
                        do {
                            skipNewlines();
                            if (check(TokenType::RPAREN) || isAtEnd()) break;
                            args.push_back(parseExpression());
                            skipNewlines();
                        } while (match(TokenType::COMMA));
                    }
                    match(TokenType::RPAREN);
                    return std::make_unique<CallExpr>(fullName, std::move(args), line, col);
                }

                if (arity >= 0) {
                    if (arity > 0 && (check(TokenType::NEWLINE) || check(TokenType::COMMA) || check(TokenType::RPAREN) ||
                        check(TokenType::RBRACKET) || check(TokenType::DEDENT) || check(TokenType::END) || isAtEnd())) {
                        auto target = std::make_unique<VarExpr>(id, line, col);
                        return std::make_unique<GetExpr>(std::move(target), std::make_unique<LiteralExpr>(Value(propTok.lexeme)));
                    }
                    std::vector<std::unique_ptr<Expr>> args;
                    for (int i = 0; i < arity; ++i) {
                        if (i > 0) match(TokenType::COMMA);
                        args.push_back(parseExpression());
                    }
                    return std::make_unique<CallExpr>(fullName, std::move(args), line, col);
                }

                auto target = std::make_unique<VarExpr>(id, line, col);
                return std::make_unique<GetExpr>(std::move(target), std::make_unique<LiteralExpr>(Value(propTok.lexeme)));
            }
        }

        auto it = functionArity_.find(id);
        if (it != functionArity_.end()) {
            int arity = it->second;
            if (check(TokenType::NEWLINE) || check(TokenType::COMMA) || check(TokenType::RPAREN) ||
                check(TokenType::RBRACKET) || check(TokenType::DEDENT) || check(TokenType::END) || isAtEnd()) {
                return std::make_unique<VarExpr>(id, line, col);
            }
            std::vector<std::unique_ptr<Expr>> args;
            if (match(TokenType::LPAREN)) {
                if (!check(TokenType::RPAREN)) {
                    do {
                        skipNewlines();
                        if (check(TokenType::RPAREN) || isAtEnd()) break;
                        args.push_back(parseExpression());
                        skipNewlines();
                    } while (match(TokenType::COMMA));
                }
                match(TokenType::RPAREN);
            } else {
                for (int i = 0; i < arity; ++i) {
                    if (i > 0) match(TokenType::COMMA);
                    args.push_back(parseExpression());
                }
            }
            return std::make_unique<CallExpr>(id, std::move(args), line, col);
        }

        if (match(TokenType::LPAREN)) {
            std::vector<std::unique_ptr<Expr>> args;
            if (!check(TokenType::RPAREN)) {
                do {
                    skipNewlines();
                    if (check(TokenType::RPAREN) || isAtEnd()) break;
                    args.push_back(parseExpression());
                    skipNewlines();
                } while (match(TokenType::COMMA));
            }
            match(TokenType::RPAREN);
            return std::make_unique<CallExpr>(id, std::move(args), line, col);
        }
        return std::make_unique<VarExpr>(id, line, col);
    }

    return parsePrimary();
}

std::unique_ptr<Expr> Parser::parsePrimary() {
    int line = peek().line;

    if (match(TokenType::NUMBER)) {
        const Token& tok = previous();
        if (tok.isFloat) {
            return std::make_unique<LiteralExpr>(Value(tok.floatValue), line);
        }
        return std::make_unique<LiteralExpr>(Value(tok.intValue), line);
    }

    if (match(TokenType::STRING)) {
        return parseStringInterpolation(previous());
    }

    if (match(TokenType::TRUE)) {
        return std::make_unique<LiteralExpr>(Value(true), line);
    }

    if (match(TokenType::FALSE)) {
        return std::make_unique<LiteralExpr>(Value(false), line);
    }

    if (match(TokenType::NIL)) {
        return std::make_unique<LiteralExpr>(Value(), line);
    }

    if (match(TokenType::LBRACKET)) {
        std::vector<std::unique_ptr<Expr>> elems;
        skipNewlines();
        if (!check(TokenType::RBRACKET)) {
            do {
                skipNewlines();
                if (check(TokenType::RBRACKET) || isAtEnd()) break;
                elems.push_back(parseExpression());
                skipNewlines();
            } while (match(TokenType::COMMA));
        }
        skipNewlines();
        if (!match(TokenType::RBRACKET)) {
            reportError("Expected closing ']'", peek());
        }
        return std::make_unique<ListLiteralExpr>(std::move(elems), line);
    }

    if (match(TokenType::LPAREN)) {
        auto expr = parseExpression();
        if (!match(TokenType::RPAREN)) {
            reportError("Expected closing ')'", peek());
        }
        return expr;
    }

    if (match(TokenType::IDENTIFIER)) {
        return std::make_unique<VarExpr>(previous().lexeme, line);
    }

    if (check(TokenType::PRINT) || check(TokenType::SILENT_PRINT)) {
        bool silent = (advance().type == TokenType::SILENT_PRINT);
        std::vector<std::unique_ptr<Expr>> args;
        if (match(TokenType::LPAREN)) {
            if (!check(TokenType::RPAREN)) {
                do {
                    skipNewlines();
                    if (check(TokenType::RPAREN) || isAtEnd()) break;
                    args.push_back(parseExpression());
                    skipNewlines();
                } while (match(TokenType::COMMA));
            }
            if (!match(TokenType::RPAREN)) {
                reportError("Expected closing ')'", peek());
            }
        }
        return std::make_unique<CallExpr>(silent ? "silent_print" : "print", std::move(args));
    }

    if (isAtEnd()) {
        reportError("Unexpected end of file in expression", peek());
    } else if (check(TokenType::NEWLINE) || check(TokenType::DEDENT) || check(TokenType::END)) {
        reportError("Expected expression", peek());
    } else {
        reportError("Unexpected token in expression", peek());
        advance();
    }
    return std::make_unique<LiteralExpr>(Value(), line);
}

std::unique_ptr<Expr> Parser::parseStringInterpolation(const Token& strTok) {
    const std::string& text = strTok.lexeme;
    if (text.find('$') == std::string::npos) {
        return std::make_unique<LiteralExpr>(Value(text), strTok.line, strTok.column);
    }

    std::vector<std::unique_ptr<Expr>> parts;
    std::string currentLit;
    size_t i = 0;
    while (i < text.size()) {
        if (text[i] == '\\' && i + 1 < text.size() && text[i + 1] == '$') {
            currentLit += '$';
            i += 2;
        } else if (text[i] == '$' && i + 1 < text.size() && text[i + 1] == '{') {
            size_t startExpr = i + 2;
            int depth = 1;
            size_t j = startExpr;
            bool inSubStr = false;
            while (j < text.size() && depth > 0) {
                if (text[j] == '"' && (j == 0 || text[j - 1] != '\\')) {
                    inSubStr = !inSubStr;
                } else if (!inSubStr) {
                    if (text[j] == '{') depth++;
                    else if (text[j] == '}') depth--;
                }
                if (depth == 0) break;
                j++;
            }

            if (depth == 0) {
                if (!currentLit.empty()) {
                    parts.push_back(std::make_unique<LiteralExpr>(Value(currentLit), strTok.line, strTok.column));
                    currentLit.clear();
                }

                std::string exprStr = text.substr(startExpr, j - startExpr);
                Lexer subLexer(exprStr);
                auto subTokens = subLexer.tokenize();
                if (!subLexer.hasErrors() && !subTokens.empty()) {
                    Parser subParser(std::move(subTokens));
                    subParser.setFunctionArity(functionArity_);
                    auto parsed = subParser.parseExpression();
                    if (parsed) {
                        parts.push_back(std::move(parsed));
                    } else {
                        parts.push_back(std::make_unique<LiteralExpr>(Value("${" + exprStr + "}"), strTok.line, strTok.column));
                    }
                } else {
                    parts.push_back(std::make_unique<LiteralExpr>(Value("${" + exprStr + "}"), strTok.line, strTok.column));
                }
                i = j + 1;
            } else {
                currentLit += "${";
                i += 2;
            }
        } else if (text[i] == '$' && i + 1 < text.size() && (std::isalpha(static_cast<unsigned char>(text[i + 1])) || text[i + 1] == '_')) {
            size_t startId = i + 1;
            size_t j = startId;
            while (j < text.size() && (std::isalnum(static_cast<unsigned char>(text[j])) || text[j] == '_')) {
                j++;
            }
            if (!currentLit.empty()) {
                parts.push_back(std::make_unique<LiteralExpr>(Value(currentLit), strTok.line, strTok.column));
                currentLit.clear();
            }
            std::string varName = text.substr(startId, j - startId);
            parts.push_back(std::make_unique<VarExpr>(varName, strTok.line, strTok.column));
            i = j;
        } else {
            currentLit += text[i];
            i++;
        }
    }

    if (!currentLit.empty()) {
        parts.push_back(std::make_unique<LiteralExpr>(Value(currentLit), strTok.line, strTok.column));
    }

    if (parts.empty()) {
        return std::make_unique<LiteralExpr>(Value(""), strTok.line, strTok.column);
    }

    if (parts.size() == 1) {
        if (auto* lit = dynamic_cast<LiteralExpr*>(parts[0].get())) {
            if (lit->value.isString()) {
                return std::move(parts[0]);
            }
        }
        return std::make_unique<BinaryExpr>(
            std::make_unique<LiteralExpr>(Value(""), strTok.line, strTok.column),
            TokenType::PLUS,
            std::move(parts[0]),
            strTok.line, strTok.column
        );
    }

    std::unique_ptr<Expr> result;
    size_t startIdx = 0;

    bool firstIsString = false;
    if (auto* lit = dynamic_cast<LiteralExpr*>(parts[0].get())) {
        if (lit->value.isString()) {
            firstIsString = true;
        }
    }

    if (firstIsString) {
        result = std::move(parts[0]);
        startIdx = 1;
    } else {
        result = std::make_unique<LiteralExpr>(Value(""), strTok.line, strTok.column);
        startIdx = 0;
    }

    for (size_t k = startIdx; k < parts.size(); ++k) {
        result = std::make_unique<BinaryExpr>(
            std::move(result),
            TokenType::PLUS,
            std::move(parts[k]),
            strTok.line, strTok.column
        );
    }

    return result;
}
