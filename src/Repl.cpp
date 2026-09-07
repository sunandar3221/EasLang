#include "Repl.hpp"
#include "Lexer.hpp"
#include "Parser.hpp"
#include <iostream>
#include <algorithm>

Repl::Repl() {}

void Repl::run() {
    std::cout << "EasLang Interactive Environment (v1.0)\n";
    std::cout << "Type 'exit' to quit.\n\n";

    std::string line;
    std::string buffer;

    auto countBrackets = [](const std::string& str) {
        int parens = 0, brackets = 0, braces = 0;
        bool inStr = false;
        for (size_t i = 0; i < str.size(); ++i) {
            char c = str[i];
            if (c == '"' && (i == 0 || str[i - 1] != '\\')) {
                inStr = !inStr;
            } else if (!inStr) {
                if (c == '(') parens++;
                else if (c == ')') parens--;
                else if (c == '[') brackets++;
                else if (c == ']') brackets--;
                else if (c == '{') braces++;
                else if (c == '}') braces--;
            }
        }
        return (parens > 0 || brackets > 0 || braces > 0);
    };

    auto startsBlock = [](const std::string& trimmed) {
        if (trimmed.empty()) return false;
        if (trimmed.back() == ':') return true;
        auto startsWithWord = [&](const std::string& prefix) {
            if (trimmed.rfind(prefix, 0) == 0) {
                if (trimmed.size() == prefix.size() || std::isspace(static_cast<unsigned char>(trimmed[prefix.size()])) || trimmed[prefix.size()] == '(' || trimmed[prefix.size()] == ':') {
                    return true;
                }
            }
            return false;
        };
        return startsWithWord("if") || startsWithWord("else") || startsWithWord("elif") ||
               startsWithWord("while") || startsWithWord("loop") || startsWithWord("fn");
    };

    auto isBlockContinuation = [](const std::string& trimmed) {
        if (trimmed.empty()) return false;
        auto startsWithWord = [&](const std::string& prefix) {
            if (trimmed.rfind(prefix, 0) == 0) {
                if (trimmed.size() == prefix.size() || std::isspace(static_cast<unsigned char>(trimmed[prefix.size()])) || trimmed[prefix.size()] == '(' || trimmed[prefix.size()] == ':') {
                    return true;
                }
            }
            return false;
        };
        return startsWithWord("else") || startsWithWord("elif");
    };

    auto executeCode = [this](const std::string& code) {
        try {
            Lexer lexer(code);
            auto tokens = lexer.tokenize();
            if (lexer.hasErrors()) {
                for (const auto& err : lexer.getErrors()) {
                    std::cerr << err << "\n";
                }
                return;
            }

            Parser parser(std::move(tokens));
            auto program = parser.parseProgram();
            if (parser.hasErrors()) {
                for (const auto& err : parser.getErrors()) {
                    std::cerr << err << "\n";
                }
                return;
            }

            bool isExpr = false;
            if (program && program->statements.size() == 1) {
                if (dynamic_cast<ExprStmt*>(program->statements[0].get())) {
                    isExpr = true;
                }
            }

            Value res = interpreter_.interpret(program.get());

            if (isExpr && !res.isNil()) {
                std::cout << res.toString() << "\n";
            }
        } catch (const std::exception& ex) {
            std::string msg = ex.what();
            if (msg.rfind("Runtime Error", 0) != 0 && msg.rfind("Syntax Error", 0) != 0) {
                std::cerr << "Runtime Error: " << msg << "\n";
            } else {
                std::cerr << msg << "\n";
            }
        } catch (...) {
            std::cerr << "Runtime Error: Unknown error occurred.\n";
        }
    };

    while (true) {
        if (buffer.empty()) {
            std::cout << "EasLang> ";
        } else {
            std::cout << "... ";
        }
        std::cout.flush();

        if (!std::getline(std::cin, line)) {
            if (!buffer.empty()) {
                executeCode(buffer);
                buffer.clear();
            }
            break;
        }

        std::string trimmed = line;
        trimmed.erase(trimmed.begin(), std::find_if(trimmed.begin(), trimmed.end(), [](unsigned char ch) {
            return !std::isspace(ch);
        }));
        trimmed.erase(std::find_if(trimmed.rbegin(), trimmed.rend(), [](unsigned char ch) {
            return !std::isspace(ch);
        }).base(), trimmed.end());

        if (buffer.empty() && trimmed == "exit") {
            break;
        }

        if (buffer.empty()) {
            if (trimmed.empty()) {
                continue;
            }
            if (startsBlock(trimmed) || countBrackets(line)) {
                buffer = line + "\n";
            } else {
                executeCode(line);
            }
        } else {
            if (trimmed.empty()) {
                if (!countBrackets(buffer)) {
                    executeCode(buffer);
                    buffer.clear();
                } else {
                    buffer += "\n";
                }
            } else {
                bool isIndented = (!line.empty() && (line[0] == ' ' || line[0] == '\t'));
                if (!isIndented && !countBrackets(buffer) && !isBlockContinuation(trimmed)) {
                    executeCode(buffer);
                    buffer.clear();
                    if (trimmed == "exit") {
                        break;
                    }
                    if (startsBlock(trimmed) || countBrackets(line)) {
                        buffer = line + "\n";
                    } else {
                        executeCode(line);
                    }
                } else {
                    buffer += line + "\n";
                }
            }
        }
    }
}
