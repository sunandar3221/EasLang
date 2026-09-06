#include "Parser.hpp"

Parser::Parser(std::vector<Token> tokens)
    : tokens_(std::move(tokens)), cursor_(0) {
    functionArity_["read"] = 1;
    functionArity_["write"] = 2;
    functionArity_["app"] = 1;
    functionArity_["window"] = 2;
    functionArity_["run"] = 0;
    functionArity_["send"] = 2;
    functionArity_["new"] = 0;
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

std::unique_ptr<BlockStmt> Parser::parseProgram() {
    auto block = std::make_unique<BlockStmt>();
    while (!isAtEnd()) {
        skipNewlines();
        if (isAtEnd()) break;
        auto stmt = parseStatement();
        if (stmt) {
            block->statements.push_back(std::move(stmt));
        }
    }
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
            auto stmt = parseStatement();
            if (stmt) {
                block->statements.push_back(std::move(stmt));
            }
        }
        match(TokenType::DEDENT);
    } else {
        while (!isAtEnd() && !check(TokenType::END) && !check(TokenType::ELSE) && !check(TokenType::DEDENT)) {
            skipNewlines();
            if (check(TokenType::END) || check(TokenType::ELSE) || isAtEnd()) break;
            auto stmt = parseStatement();
            if (stmt) {
                block->statements.push_back(std::move(stmt));
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
        case TokenType::PRINT: return parsePrint();
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
        default: return parseAssignmentOrExpr();
    }
}

std::unique_ptr<Stmt> Parser::parsePrint() {
    int line = advance().line;
    std::vector<std::unique_ptr<Expr>> args;

    while (!check(TokenType::NEWLINE) && !check(TokenType::DEDENT) && !check(TokenType::END) && !isAtEnd()) {
        args.push_back(parseExpression());
    }
    match(TokenType::NEWLINE);
    return std::make_unique<PrintStmt>(std::move(args), line);
}

std::unique_ptr<Stmt> Parser::parseWrite() {
    int line = advance().line;
    auto path = parseUnary();
    auto content = parseExpression();
    match(TokenType::NEWLINE);
    return std::make_unique<WriteStmt>(std::move(path), std::move(content), line);
}

std::unique_ptr<Stmt> Parser::parseApp() {
    int line = advance().line;
    auto title = parseExpression();
    match(TokenType::NEWLINE);
    return std::make_unique<AppStmt>(std::move(title), line);
}

std::unique_ptr<Stmt> Parser::parseWindow() {
    int line = advance().line;
    auto width = parseUnary();
    auto height = parseUnary();
    match(TokenType::NEWLINE);
    return std::make_unique<WindowStmt>(std::move(width), std::move(height), line);
}

std::unique_ptr<Stmt> Parser::parseRun() {
    int line = advance().line;
    std::unique_ptr<Expr> duration = nullptr;
    if (!check(TokenType::NEWLINE) && !check(TokenType::DEDENT) && !check(TokenType::END) && !isAtEnd()) {
        duration = parseExpression();
    }
    match(TokenType::NEWLINE);
    return std::make_unique<RunStmt>(std::move(duration), line);
}

std::unique_ptr<Stmt> Parser::parseSet() {
    int line = advance().line;
    auto target = parseUnary();
    auto prop = parseUnary();
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
    return std::make_unique<UseStmt>(std::move(mod), line);
}

std::unique_ptr<Stmt> Parser::parseIf() {
    int line = advance().line;
    auto condition = parseExpression();
    match(TokenType::NEWLINE);
    auto thenBranch = parseBlock();
    skipNewlines();
    std::unique_ptr<BlockStmt> elseBranch = nullptr;
    if (match(TokenType::ELSE)) {
        match(TokenType::NEWLINE);
        elseBranch = parseBlock();
    }
    return std::make_unique<IfStmt>(std::move(condition), std::move(thenBranch), std::move(elseBranch), line);
}

std::unique_ptr<Stmt> Parser::parseLoop() {
    int line = advance().line;
    auto count = parseExpression();
    match(TokenType::NEWLINE);
    auto body = parseBlock();
    return std::make_unique<LoopStmt>(std::move(count), std::move(body), line);
}

std::unique_ptr<Stmt> Parser::parseWhile() {
    int line = advance().line;
    auto condition = parseExpression();
    match(TokenType::NEWLINE);
    auto body = parseBlock();
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
    return std::make_unique<FnDeclStmt>(std::move(name), std::move(params), std::move(body), line);
}

std::unique_ptr<Stmt> Parser::parseAssignmentOrExpr() {
    int line = peek().line;
    if (check(TokenType::IDENTIFIER) && peekNext().type == TokenType::ASSIGN) {
        std::string name = advance().lexeme;
        advance();
        auto val = parseExpression();
        match(TokenType::NEWLINE);
        return std::make_unique<AssignStmt>(std::move(name), std::move(val), line);
    }

    if (check(TokenType::IDENTIFIER) && peekNext().type == TokenType::LBRACKET) {
        auto primary = parseCallOrPrimary();
        primary = parsePostfix(std::move(primary));
        if (auto* idxExpr = dynamic_cast<IndexExpr*>(primary.get())) {
            if (match(TokenType::ASSIGN)) {
                auto val = parseExpression();
                match(TokenType::NEWLINE);
                return std::make_unique<IndexAssignStmt>(std::move(idxExpr->target), std::move(idxExpr->index), std::move(val), line);
            }
        }
        auto expr = parseLogicalOr();
        match(TokenType::NEWLINE);
        return std::make_unique<ExprStmt>(std::move(primary), line);
    }

    auto expr = parseExpression();
    match(TokenType::NEWLINE);
    return std::make_unique<ExprStmt>(std::move(expr), line);
}

std::unique_ptr<Expr> Parser::parseExpression() {
    return parseLogicalOr();
}

std::unique_ptr<Expr> Parser::parseLogicalOr() {
    auto expr = parseLogicalAnd();
    while (match(TokenType::OR)) {
        TokenType op = previous().type;
        auto right = parseLogicalAnd();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
    }
    return expr;
}

std::unique_ptr<Expr> Parser::parseLogicalAnd() {
    auto expr = parseEquality();
    while (match(TokenType::AND)) {
        TokenType op = previous().type;
        auto right = parseEquality();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
    }
    return expr;
}

std::unique_ptr<Expr> Parser::parseEquality() {
    auto expr = parseRelational();
    while (match(TokenType::EQUAL_EQUAL) || match(TokenType::BANG_EQUAL)) {
        TokenType op = previous().type;
        auto right = parseRelational();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
    }
    return expr;
}

std::unique_ptr<Expr> Parser::parseRelational() {
    auto expr = parseAdditive();
    while (match(TokenType::LESS) || match(TokenType::GREATER) ||
           match(TokenType::LESS_EQUAL) || match(TokenType::GREATER_EQUAL)) {
        TokenType op = previous().type;
        auto right = parseAdditive();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
    }
    return expr;
}

std::unique_ptr<Expr> Parser::parseAdditive() {
    auto expr = parseMultiplicative();
    while (match(TokenType::PLUS) || match(TokenType::MINUS)) {
        TokenType op = previous().type;
        auto right = parseMultiplicative();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
    }
    return expr;
}

std::unique_ptr<Expr> Parser::parseMultiplicative() {
    auto expr = parseUnary();
    while (match(TokenType::STAR) || match(TokenType::SLASH) || match(TokenType::PERCENT)) {
        TokenType op = previous().type;
        auto right = parseUnary();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
    }
    return expr;
}

std::unique_ptr<Expr> Parser::parseUnary() {
    if (match(TokenType::MINUS) || match(TokenType::NOT)) {
        TokenType op = previous().type;
        auto right = parseUnary();
        return std::make_unique<UnaryExpr>(op, std::move(right));
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
            expr = std::make_unique<GetExpr>(std::move(expr), std::make_unique<LiteralExpr>(Value(prop)));
        } else {
            break;
        }
    }
    return expr;
}

std::unique_ptr<Expr> Parser::parseCallOrPrimary() {
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
        std::string id = peek().lexeme;
        auto it = functionArity_.find(id);
        if (it != functionArity_.end()) {
            advance();
            int arity = it->second;
            std::vector<std::unique_ptr<Expr>> args;
            if (match(TokenType::LPAREN)) {
                for (int i = 0; i < arity; ++i) {
                    if (i > 0) match(TokenType::COMMA);
                    args.push_back(parseExpression());
                }
                match(TokenType::RPAREN);
            } else {
                for (int i = 0; i < arity; ++i) {
                    args.push_back(parseUnary());
                }
            }
            return std::make_unique<CallExpr>(id, std::move(args));
        }

        advance();
        if (match(TokenType::LPAREN)) {
            std::vector<std::unique_ptr<Expr>> args;
            if (!check(TokenType::RPAREN)) {
                do {
                    args.push_back(parseExpression());
                } while (match(TokenType::COMMA));
            }
            match(TokenType::RPAREN);
            return std::make_unique<CallExpr>(id, std::move(args));
        }
        return std::make_unique<VarExpr>(id);
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
        return std::make_unique<LiteralExpr>(Value(previous().lexeme), line);
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
                if (check(TokenType::RBRACKET)) break;
                elems.push_back(parseExpression());
                skipNewlines();
            } while (match(TokenType::COMMA));
        }
        skipNewlines();
        match(TokenType::RBRACKET);
        return std::make_unique<ListLiteralExpr>(std::move(elems), line);
    }

    if (match(TokenType::LPAREN)) {
        auto expr = parseExpression();
        match(TokenType::RPAREN);
        return expr;
    }

    if (match(TokenType::IDENTIFIER)) {
        return std::make_unique<VarExpr>(previous().lexeme, line);
    }

    advance();
    return std::make_unique<LiteralExpr>(Value(), line);
}
