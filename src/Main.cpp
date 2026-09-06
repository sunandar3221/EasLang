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
#include <filesystem>
#include <sstream>

int main(int argc, char* argv[]) {
    if (argc == 1) {
        Repl repl;
        repl.run();
        return 0;
    }

    std::string arg1 = argv[1];

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

        std::filesystem::create_directories(".eas_cache");
        std::string tempCpp = ".eas_cache/_eas_build_temp.cpp";
        std::ofstream outCpp(tempCpp);
        if (!outCpp.is_open()) {
            std::cerr << "Error: Could not create temporary AOT source file.\n";
            return 1;
        }
        outCpp << cppCode;
        outCpp.close();

        bool ok = aot.buildBinary(tempCpp, outputFile);
        if (!ok) {
            std::cerr << "Error: AOT Compilation failed.\n";
            return 1;
        }

        std::cout << "Successfully compiled " << inputFile << " -> " << outputFile << "\n";
        return 0;
    }

    bool useVm = false;
    std::string scriptPath;
    int scriptArgStart = 2;

    if (arg1 == "--vm" || arg1 == "-v") {
        if (argc < 3) {
            std::cerr << "Usage: eas --vm <script.eas>\n";
            return 1;
        }
        useVm = true;
        scriptPath = argv[2];
        scriptArgStart = 3;
    } else if (arg1 == "run") {
        if (argc < 3) {
            std::cerr << "Usage: eas run <script.eas>\n";
            return 1;
        }
        scriptPath = argv[2];
        scriptArgStart = 3;
    } else {
        scriptPath = arg1;
        scriptArgStart = 2;
    }

    Value content = StandardLibrary::readFile(scriptPath);
    if (content.strVal.empty()) {
        std::ifstream testOpen(scriptPath);
        if (!testOpen.good()) {
            std::cerr << "Error: Cannot open file '" << scriptPath << "'\n";
            return 1;
        }
    }

    if (!useVm) {
        try {
            size_t contentHash = std::hash<std::string>{}(content.strVal);
            std::filesystem::create_directories(".eas_cache");
            std::string cachedExe = ".eas_cache/eas_" + std::to_string(contentHash) + ".exe";

            if (std::filesystem::exists(cachedExe) && std::filesystem::file_size(cachedExe) > 0) {
                std::string cmd = cachedExe;
                for (int i = scriptArgStart; i < argc; ++i) {
                    cmd += " \"";
                    cmd += argv[i];
                    cmd += "\"";
                }
                for (char& c : cmd) {
                    if (c == '/') c = '\\';
                }
                int res = std::system(cmd.c_str());
                return res;
            }

            Lexer lexer(content.strVal);
            auto tokens = lexer.tokenize();
            Parser parser(std::move(tokens));
            auto program = parser.parseProgram();

            AotGenerator aot;
            std::string cppCode = aot.generateCpp(program.get());

            std::string tempCpp = ".eas_cache/temp_" + std::to_string(contentHash) + ".cpp";
            std::ofstream outCpp(tempCpp);
            if (outCpp.is_open()) {
                outCpp << cppCode;
                outCpp.close();

                bool ok = aot.buildBinary(tempCpp, cachedExe);
                if (ok && std::filesystem::exists(cachedExe)) {
                    std::string cmd = cachedExe;
                    for (int i = scriptArgStart; i < argc; ++i) {
                        cmd += " \"";
                        cmd += argv[i];
                        cmd += "\"";
                    }
                    for (char& c : cmd) {
                        if (c == '/') c = '\\';
                    }
                    int res = std::system(cmd.c_str());
                    return res;
                }
            }
        } catch (...) {
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
