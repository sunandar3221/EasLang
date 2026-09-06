#pragma once

#include "Value.hpp"
#include <string>
#include <vector>

class StandardLibrary {
public:
    static Value print(const std::vector<Value>& args);
    static Value silentPrint(const std::vector<Value>& args);
    static Value readFile(const std::string& path);
    static Value writeFile(const std::string& path, const std::string& content);
    static Value httpGet(const std::string& url);
    static Value httpSend(const std::string& url, const std::string& data);

    static void setAppTitle(const std::string& title);
    static void setWindowSize(int width, int height);
    static Value runApp(int timeoutMs = -1);

    static std::string getAppTitle();
    static int getWindowWidth();
    static int getWindowHeight();

private:
    static std::string appTitle_;
    static int windowWidth_;
    static int windowHeight_;
};
