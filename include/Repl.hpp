#pragma once

#include "Interpreter.hpp"

class Repl {
public:
    Repl();
    void run();

private:
    Interpreter interpreter_;
};
