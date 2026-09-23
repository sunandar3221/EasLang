#pragma once

#include "Value.hpp"
#include <string>
#include <vector>

class StandardLibrary {
public:
    static Value print(const std::vector<Value>& args);
    static Value silentPrint(const std::vector<Value>& args);
    static Value input(const std::string& prompt = "");
    static Value readFile(const std::string& path);
    static Value writeFile(const std::string& path, const std::string& content);
    static Value appendFile(const std::string& path, const std::string& content);
    static Value writeLines(const std::string& path, const Value& listVal);
    static Value openFile(const std::string& path, const std::string& mode = "w");
    static Value fileWrite(int64_t handleId, const std::string& content);
    static Value fileWriteLine(int64_t handleId, const std::string& line);
    static Value fileFlush(int64_t handleId);
    static Value fileClose(int64_t handleId);
    static Value httpGet(const std::string& url);
    static Value httpSend(const std::string& url, const std::string& data);

    static void setAppTitle(const std::string& title);
    static void setWindowSize(int width, int height);
    static Value runApp(int timeoutMs = -1);

    static Value toLower(const std::string& str);
    static Value toUpper(const std::string& str);
    static Value caseSensitive(const std::string& a, const std::string& b);
    static Value incaseSensitive(const std::string& a, const std::string& b);
    static Value toInt(const Value& val);
    static Value toFloat(const Value& val);

    static Value mathSqrt(double val);
    static Value mathAbs(double val);
    static Value mathPow(double base, double exp);
    static Value mathFloor(double val);
    static Value mathCeil(double val);
    static Value mathRound(double val);
    static Value mathMin(double a, double b);
    static Value mathMax(double a, double b);
    static Value mathRandom();
    static Value mathRandom(double max);
    static Value mathRandom(double min, double max);
    static Value mathRandom(const std::vector<Value>& args);
    static Value mathRandomSeed(int64_t seed);
    static Value mathRandomSeed();
    static Value mathRandomSeed(const std::vector<Value>& args);
    static Value mathSin(double val);
    static Value mathCos(double val);
    static Value mathTan(double val);
    static Value timeSleep(int64_t ms);
    static Value timeNow();

    static std::string getAppTitle();
    static int getWindowWidth();
    static int getWindowHeight();

private:
    static std::string appTitle_;
    static int windowWidth_;
    static int windowHeight_;
};
