#include "Lexer.hpp"
#include <cctype>
#include <cstdlib>

Lexer::Lexer(std::string source)
    : source_(std::move(source)),
      cursor_(0),
      line_(1),
      column_(1),
      bracketNesting_(0),
      atLineStart_(true) {
    indentStack_.push_back(0);
    initKeywords();
}

void Lexer::initKeywords() {
    keywords_["print"] = TokenType::PRINT;
    keywords_["if"] = TokenType::IF;
    keywords_["else"] = TokenType::ELSE;
    keywords_["loop"] = TokenType::LOOP;
    keywords_["while"] = TokenType::WHILE;
    keywords_["fn"] = TokenType::FN;
    keywords_["use"] = TokenType::USE;
    keywords_["new"] = TokenType::NEW;
    keywords_["get"] = TokenType::GET;
    keywords_["set"] = TokenType::SET;
    keywords_["read"] = TokenType::READ;
    keywords_["write"] = TokenType::WRITE;
    keywords_["send"] = TokenType::SEND;
    keywords_["app"] = TokenType::APP;
    keywords_["window"] = TokenType::WINDOW;
    keywords_["run"] = TokenType::RUN;
    keywords_["end"] = TokenType::END;
    keywords_["and"] = TokenType::AND;
    keywords_["or"] = TokenType::OR;
    keywords_["not"] = TokenType::NOT;
    keywords_["true"] = TokenType::TRUE;
    keywords_["false"] = TokenType::FALSE;
    keywords_["nil"] = TokenType::NIL;
    keywords_["null"] = TokenType::NIL;
}

bool Lexer::isAtEnd() const {
    return cursor_ >= source_.size();
}

char Lexer::peek() const {
    if (isAtEnd()) return '\0';
    return source_[cursor_];
}

char Lexer::peekNext() const {
    if (cursor_ + 1 >= source_.size()) return '\0';
    return source_[cursor_ + 1];
}

char Lexer::advance() {
    char c = source_[cursor_++];
    column_++;
    return c;
}

bool Lexer::match(char expected) {
    if (isAtEnd() || source_[cursor_] != expected) return false;
    cursor_++;
    column_++;
    return true;
}

void Lexer::handleIndentation(std::vector<Token>& tokens) {
    int indent = 0;
    size_t tempCursor = cursor_;
    int tempCol = column_;
    
    while (tempCursor < source_.size()) {
        char c = source_[tempCursor];
        if (c == ' ') {
            indent++;
            tempCursor++;
            tempCol++;
        } else if (c == '\t') {
            indent += 4;
            tempCursor++;
            tempCol += 4;
        } else if (c == '\r') {
            tempCursor++;
        } else {
            break;
        }
    }

    if (tempCursor >= source_.size() || source_[tempCursor] == '\n') {
        cursor_ = tempCursor;
        column_ = tempCol;
        return;
    }

    cursor_ = tempCursor;
    column_ = tempCol;

    if (bracketNesting_ == 0) {
        int previousIndent = indentStack_.back();
        if (indent > previousIndent) {
            indentStack_.push_back(indent);
            tokens.emplace_back(TokenType::INDENT, "", line_, column_);
        } else if (indent < previousIndent) {
            while (indentStack_.size() > 1 && indentStack_.back() > indent) {
                indentStack_.pop_back();
                tokens.emplace_back(TokenType::DEDENT, "", line_, column_);
            }
        }
    }

    atLineStart_ = false;
}

Token Lexer::readString() {
    int startLine = line_;
    int startCol = column_ - 1;
    std::string val;

    while (!isAtEnd() && peek() != '"') {
        if (peek() == '\n') {
            line_++;
            column_ = 1;
        }
        if (peek() == '\\') {
            advance();
            if (!isAtEnd()) {
                char esc = advance();
                if (esc == 'n') val += '\n';
                else if (esc == 't') val += '\t';
                else if (esc == 'r') val += '\r';
                else if (esc == '"') val += '"';
                else if (esc == '\\') val += '\\';
                else val += esc;
            }
        } else {
            val += advance();
        }
    }

    if (!isAtEnd() && peek() == '"') {
        advance();
    }

    Token tok(TokenType::STRING, val, startLine, startCol);
    return tok;
}

Token Lexer::readNumber() {
    int startLine = line_;
    int startCol = column_ - 1;
    size_t startPos = cursor_ - 1;
    bool isFloat = false;

    while (!isAtEnd() && std::isdigit(static_cast<unsigned char>(peek()))) {
        advance();
    }

    if (peek() == '.' && std::isdigit(static_cast<unsigned char>(peekNext()))) {
        isFloat = true;
        advance();
        while (!isAtEnd() && std::isdigit(static_cast<unsigned char>(peek()))) {
            advance();
        }
    }

    std::string lexeme = source_.substr(startPos, cursor_ - startPos);
    Token tok(TokenType::NUMBER, lexeme, startLine, startCol);
    tok.isFloat = isFloat;
    if (isFloat) {
        tok.floatValue = std::strtod(lexeme.c_str(), nullptr);
    } else {
        tok.intValue = std::strtoll(lexeme.c_str(), nullptr, 10);
    }
    return tok;
}

Token Lexer::readIdentifier() {
    int startLine = line_;
    int startCol = column_ - 1;
    size_t startPos = cursor_ - 1;

    while (!isAtEnd()) {
        char c = peek();
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '_') {
            advance();
        } else {
            break;
        }
    }

    std::string lexeme = source_.substr(startPos, cursor_ - startPos);
    auto it = keywords_.find(lexeme);
    if (it != keywords_.end()) {
        return Token(it->second, lexeme, startLine, startCol);
    }
    return Token(TokenType::IDENTIFIER, lexeme, startLine, startCol);
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;

    while (!isAtEnd()) {
        if (atLineStart_) {
            handleIndentation(tokens);
            if (isAtEnd()) break;
        }

        char c = advance();

        if (c == ' ' || c == '\t' || c == '\r') {
            continue;
        }

        if (c == '\n') {
            if (bracketNesting_ == 0) {
                if (!tokens.empty() &&
                    tokens.back().type != TokenType::NEWLINE &&
                    tokens.back().type != TokenType::INDENT) {
                    tokens.emplace_back(TokenType::NEWLINE, "\n", line_, column_);
                }
            }
            line_++;
            column_ = 1;
            atLineStart_ = true;
            continue;
        }

        if (c == '"') {
            tokens.push_back(readString());
            continue;
        }

        if (std::isdigit(static_cast<unsigned char>(c))) {
            tokens.push_back(readNumber());
            continue;
        }

        if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
            tokens.push_back(readIdentifier());
            continue;
        }

        int curLine = line_;
        int curCol = column_ - 1;

        switch (c) {
            case '=':
                if (match('=')) tokens.emplace_back(TokenType::EQUAL_EQUAL, "==", curLine, curCol);
                else tokens.emplace_back(TokenType::ASSIGN, "=", curLine, curCol);
                break;
            case '!':
                if (match('=')) tokens.emplace_back(TokenType::BANG_EQUAL, "!=", curLine, curCol);
                else tokens.emplace_back(TokenType::NOT, "!", curLine, curCol);
                break;
            case '<':
                if (match('=')) tokens.emplace_back(TokenType::LESS_EQUAL, "<=", curLine, curCol);
                else tokens.emplace_back(TokenType::LESS, "<", curLine, curCol);
                break;
            case '>':
                if (match('=')) tokens.emplace_back(TokenType::GREATER_EQUAL, ">=", curLine, curCol);
                else tokens.emplace_back(TokenType::GREATER, ">", curLine, curCol);
                break;
            case '+': tokens.emplace_back(TokenType::PLUS, "+", curLine, curCol); break;
            case '-': tokens.emplace_back(TokenType::MINUS, "-", curLine, curCol); break;
            case '*': tokens.emplace_back(TokenType::STAR, "*", curLine, curCol); break;
            case '/': tokens.emplace_back(TokenType::SLASH, "/", curLine, curCol); break;
            case '%': tokens.emplace_back(TokenType::PERCENT, "%", curLine, curCol); break;
            case '(':
                bracketNesting_++;
                tokens.emplace_back(TokenType::LPAREN, "(", curLine, curCol);
                break;
            case ')':
                if (bracketNesting_ > 0) bracketNesting_--;
                tokens.emplace_back(TokenType::RPAREN, ")", curLine, curCol);
                break;
            case '[':
                bracketNesting_++;
                tokens.emplace_back(TokenType::LBRACKET, "[", curLine, curCol);
                break;
            case ']':
                if (bracketNesting_ > 0) bracketNesting_--;
                tokens.emplace_back(TokenType::RBRACKET, "]", curLine, curCol);
                break;
            case ',': tokens.emplace_back(TokenType::COMMA, ",", curLine, curCol); break;
            case '.': tokens.emplace_back(TokenType::DOT, ".", curLine, curCol); break;
            default:
                break;
        }
    }

    if (!tokens.empty() &&
        tokens.back().type != TokenType::NEWLINE &&
        tokens.back().type != TokenType::DEDENT) {
        tokens.emplace_back(TokenType::NEWLINE, "\n", line_, column_);
    }

    while (indentStack_.size() > 1) {
        indentStack_.pop_back();
        tokens.emplace_back(TokenType::DEDENT, "", line_, column_);
    }

    tokens.emplace_back(TokenType::END_OF_FILE, "", line_, column_);
    return tokens;
}
