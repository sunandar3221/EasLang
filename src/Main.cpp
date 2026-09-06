#include "Lexer.hpp"
#include "Parser.hpp"
#include "Interpreter.hpp"
#include "BytecodeCompiler.hpp"
#include "VM.hpp"
#include "AotGenerator.hpp"
#include "Repl.hpp"
#include "StandardLibrary.hpp"
#include <iostream>
#include <fstream>

int main(int argc, char* argv[]) {
    if (argc == 1) {
        Repl repl;
        repl.run();
        return 0;
    }

    std::string arg1 = argv[1];
    if (arg1 == "-v" || arg1 == "--version" || arg1 == "version") {
        std::cout << "EasLang v1.0.0\n";
        return 0;
    }
    if (arg1 == "-h" || arg1 == "--help" || arg1 == "help") {
        std::cout << "Usage: eas [script.eas] | build <script.eas> [-o output] | --version\n";
        return 0;
    }

    if (arg1 == "build" || arg1 == "-c" || arg1 == "--compile") {
        if (argc < 3) {
            std::cerr << "Usage: eas build <input.eas> [-o <output.exe>]\n";
            return 1;
        }

        std::string inputFile = argv[2];
        std::string outputFile = "output.exe";

        for (int i = 3; i < argc; ++i) {
            std::string arg = argv[i];
            if ((arg == "-o" || arg == "--output") && i + 1 < argc) {
                outputFile = argv[++i];
            }
        }

        Value content = StandardLibrary::readFile(inputFile);
        if (content.strVal.empty()) {
            std::cerr << "Error: Could not read file " << inputFile << "\n";
            return 1;
        }

        Lexer lexer(content.strVal);
        auto tokens = lexer.tokenize();
        Parser parser(std::move(tokens));
        auto program = parser.parseProgram();

        AotGenerator aot;
        std::string cppCode = aot.generateCpp(program.get());

        std::string tempCpp = "_eas_build_temp.cpp";
        std::ofstream outCpp(tempCpp);
        if (!outCpp.is_open()) {
            std::cerr << "Error: Could not create temporary AOT source file.\n";
            return 1;
        }
        outCpp << cppCode;
        outCpp.close();

        bool ok = aot.buildBinary(tempCpp, outputFile);
        std::remove(tempCpp.c_str());
        if (!ok) {
            std::cerr << "Error: AOT Compilation failed.\n";
            return 1;
        }

        std::cout << "Successfully compiled " << inputFile << " -> " << outputFile << "\n";
        return 0;
    }

    std::string scriptPath = (arg1 == "run") ? (argc > 2 ? argv[2] : "") : arg1;
    if (scriptPath == "--vm" || scriptPath == "-v") {
        scriptPath = (argc > 2 ? argv[2] : "");
    }
    if (scriptPath.empty()) {
        std::cerr << "Usage: eas <script.eas>\n";
        return 1;
    }

    Value content = StandardLibrary::readFile(scriptPath);
    if (content.strVal.empty()) {
        std::ifstream testOpen(scriptPath);
        if (!testOpen.good()) {
            std::cerr << "Error: Cannot open file '" << scriptPath << "'\n";
            return 1;
        }
    }

    try {
        Lexer lexer(content.strVal);
        auto tokens = lexer.tokenize();
        Parser parser(std::move(tokens));
        auto program = parser.parseProgram();

        BytecodeCompiler compiler;
        auto mainChunk = compiler.compile(program.get());

        VM vm;
        vm.registerFunctions(compiler.getFunctions());
        vm.run(mainChunk.get());
    } catch (const std::exception& ex) {
        std::cerr << "Runtime Error: " << ex.what() << "\n";
        return 1;
    }

    return 0;
}
