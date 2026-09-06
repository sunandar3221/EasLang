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
    while (true) {
        std::cout << "EasLang> ";
        std::cout.flush();

        if (!std::getline(std::cin, line)) {
            break;
        }

        std::string trimmed = line;
        trimmed.erase(trimmed.begin(), std::find_if(trimmed.begin(), trimmed.end(), [](unsigned char ch) {
            return !std::isspace(ch);
        }));
        trimmed.erase(std::find_if(trimmed.rbegin(), trimmed.rend(), [](unsigned char ch) {
            return !std::isspace(ch);
        }).base(), trimmed.end());

        if (trimmed == "exit") {
            break;
        }

        if (trimmed.empty()) {
            continue;
        }

        try {
            Lexer lexer(line);
            auto tokens = lexer.tokenize();
            Parser parser(std::move(tokens));
            auto program = parser.parseProgram();

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
            std::cout << "Error: " << ex.what() << "\n";
        } catch (...) {
            std::cout << "Unknown error occurred.\n";
        }
    }
}
