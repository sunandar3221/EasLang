#pragma once

#include <string>
#include <vector>

class Diagnostic {
public:
    static void setSource(const std::string& filename, const std::string& source);
    static const std::string& getFilename();
    static std::string getLine(int line);
    static int getLineCount();

    static int levenshteinDistance(const std::string& s1, const std::string& s2);
    static std::string suggestSimilar(const std::string& word, const std::vector<std::string>& candidates, int maxDist = 2);

    static std::string format(
        const std::string& errorType,
        const std::string& message,
        int line,
        int column,
        int length = 1,
        const std::string& recommendation = "",
        const std::string& hint = ""
    );

    static const std::vector<std::string>& getKeywords();
    static const std::vector<std::string>& getStatementKeywords();
    static const std::vector<std::string>& getBuiltinFunctions();
    static const std::vector<std::string>& getStandardLibraries();
    static std::vector<std::string> getModuleMembers(const std::string& moduleName);

    static bool supportsColor();

private:
    static std::string currentFilename_;
    static std::vector<std::string> sourceLines_;
};
