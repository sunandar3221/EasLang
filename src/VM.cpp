#include "VM.hpp"
#include "Lexer.hpp"
#include "Parser.hpp"
#include "BytecodeCompiler.hpp"
#include <iostream>
#include <stdexcept>
#include <cmath>
#include <cstdlib>
#include <chrono>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

VM::VM() {
    stack_.resize(STACK_MAX);
    frames_.resize(FRAMES_MAX);
}

void VM::registerFunction(const std::string& name, std::shared_ptr<Chunk> chunk) {
    functions_[name] = std::move(chunk);
}

void VM::registerFunctions(const std::unordered_map<std::string, std::shared_ptr<Chunk>>& fns) {
    for (const auto& kv : fns) {
        functions_[kv.first] = kv.second;
    }
}

void VM::loadLibrary(const std::string& name) {
    loadedLibraries_.insert(name);
    if (name == "io") {
        Value ioObj = Value::makeObject();
        ioObj.setProperty("input", Value(ValueType::FUNCTION, "io.input"));
        ioObj.setProperty("ask", Value(ValueType::FUNCTION, "io.input"));
        ioObj.setProperty("read", Value(ValueType::FUNCTION, "io.read"));
        ioObj.setProperty("write", Value(ValueType::FUNCTION, "io.write"));
        ioObj.setProperty("print", Value(ValueType::FUNCTION, "io.print"));
        globals_["io"] = ioObj;
        globals_["input"] = Value(ValueType::FUNCTION, "io.input");
    } else if (name == "math") {
        Value mathObj = Value::makeObject();
        mathObj.setProperty("sqrt", Value(ValueType::FUNCTION, "math.sqrt"));
        mathObj.setProperty("abs", Value(ValueType::FUNCTION, "math.abs"));
        mathObj.setProperty("pow", Value(ValueType::FUNCTION, "math.pow"));
        mathObj.setProperty("floor", Value(ValueType::FUNCTION, "math.floor"));
        mathObj.setProperty("ceil", Value(ValueType::FUNCTION, "math.ceil"));
        mathObj.setProperty("round", Value(ValueType::FUNCTION, "math.round"));
        mathObj.setProperty("min", Value(ValueType::FUNCTION, "math.min"));
        mathObj.setProperty("max", Value(ValueType::FUNCTION, "math.max"));
        mathObj.setProperty("sin", Value(ValueType::FUNCTION, "math.sin"));
        mathObj.setProperty("cos", Value(ValueType::FUNCTION, "math.cos"));
        mathObj.setProperty("tan", Value(ValueType::FUNCTION, "math.tan"));
        mathObj.setProperty("pi", Value(3.14159265358979323846));
        mathObj.setProperty("e", Value(2.71828182845904523536));
        globals_["math"] = mathObj;
    } else if (name == "time") {
        Value timeObj = Value::makeObject();
        timeObj.setProperty("sleep", Value(ValueType::FUNCTION, "time.sleep"));
        timeObj.setProperty("now", Value(ValueType::FUNCTION, "time.now"));
        globals_["time"] = timeObj;
    } else if (name == "net" || name == "http") {
        Value netObj = Value::makeObject();
        netObj.setProperty("get", Value(ValueType::FUNCTION, "net.get"));
        netObj.setProperty("send", Value(ValueType::FUNCTION, "net.send"));
        globals_[name] = netObj;
    } else if (name == "gui") {
        Value guiObj = Value::makeObject();
        guiObj.setProperty("app", Value(ValueType::FUNCTION, "gui.app"));
        guiObj.setProperty("window", Value(ValueType::FUNCTION, "gui.window"));
        guiObj.setProperty("run", Value(ValueType::FUNCTION, "gui.run"));
        globals_["gui"] = guiObj;
    } else if (name == "str" || name == "string") {
        Value strObj = Value::makeObject();
        strObj.setProperty("lower", Value(ValueType::FUNCTION, "lower"));
        strObj.setProperty("upper", Value(ValueType::FUNCTION, "upper"));
        strObj.setProperty("case_sensitive", Value(ValueType::FUNCTION, "case_sensitive"));
        strObj.setProperty("incase_sensitive", Value(ValueType::FUNCTION, "incase_sensitive"));
        globals_["str"] = strObj;
        globals_["string"] = strObj;
    } else {
        std::string filename = name;
        if (filename.size() < 4 || filename.substr(filename.size() - 4) != ".eas") {
            filename += ".eas";
        }
        Value fileContent = StandardLibrary::readFile(filename);
        if (!fileContent.strVal.empty()) {
            Lexer modLexer(fileContent.strVal);
            auto modTokens = modLexer.tokenize();
            if (!modLexer.hasErrors()) {
                Parser modParser(std::move(modTokens));
                auto modAst = modParser.parseProgram();
                if (!modParser.hasErrors()) {
                    BytecodeCompiler modComp;
                    auto modChunk = modComp.compile(modAst.get());
                    for (const auto& kv : modComp.getFunctions()) {
                        functions_[kv.first] = kv.second;
                    }
                    VM modVM;
                    modVM.globals_ = globals_;
                    modVM.functions_ = functions_;
                    modVM.loadedLibraries_ = loadedLibraries_;
                    modVM.run(modChunk.get());
                    for (const auto& g : modVM.globals_) {
                        globals_[g.first] = g.second;
                    }
                    for (const auto& f : modVM.functions_) {
                        functions_[f.first] = f.second;
                    }
                    for (const auto& l : modVM.loadedLibraries_) {
                        loadedLibraries_.insert(l);
                    }
                }
            }
        }
    }
}

bool VM::isLibraryLoaded(const std::string& name) const {
    return loadedLibraries_.find(name) != loadedLibraries_.end();
}

void VM::runtimeError(const std::string& message, Chunk* chunk, size_t ip, size_t frameCount) {
    std::string err = message;
    if (chunk && !chunk->lines.empty()) {
        size_t lineIdx = (ip > 0 && ip - 1 < chunk->lines.size()) ? ip - 1 : 0;
        int line = chunk->lines[lineIdx];
        err += "\n  [Line " + std::to_string(line) + "] in " + (chunk->name.empty() ? "script" : chunk->name);
    }
    for (size_t i = frameCount > 1 ? frameCount - 1 : 0; i > 0; --i) {
        CallFrame& f = frames_[i - 1];
        if (f.chunk && !f.chunk->lines.empty()) {
            size_t fLineIdx = (f.ip > 0 && f.ip - 1 < f.chunk->lines.size()) ? f.ip - 1 : 0;
            int fLine = f.chunk->lines[fLineIdx];
            err += "\n  [Line " + std::to_string(fLine) + "] in " + (f.chunk->name.empty() ? "script" : f.chunk->name);
        }
    }
    throw std::runtime_error(err);
}

Value VM::run(Chunk* chunk) {
    if (!chunk) return Value();

    auto resolveChunk = [this](Chunk* c) {
        if (!c) return;
        c->cachedChunks.resize(c->constants.size(), nullptr);
        for (size_t i = 0; i < c->constants.size(); ++i) {
            if (c->constants[i].isString()) {
                auto it = functions_.find(c->constants[i].strVal);
                if (it != functions_.end()) {
                    c->cachedChunks[i] = it->second.get();
                }
            }
        }
    };

    resolveChunk(chunk);
    for (auto& kv : functions_) {
        resolveChunk(kv.second.get());
    }

    Value* top = stack_.data();
    size_t frameCount = 0;

    CallFrame* frame = &frames_[frameCount++];
    frame->chunk = chunk;
    frame->ip = 0;
    frame->slots = top;

    for (int i = 0; i < chunk->localsCount; ++i) {
        *top++ = Value();
    }

    Chunk* curChunk = frame->chunk;
    uint8_t* code = curChunk->code.data();
    size_t ip = 0;
    Value* slots = frame->slots;

    while (true) {
        uint8_t instruction = code[ip++];
        switch (static_cast<OpCode>(instruction)) {
            case OpCode::OP_CONSTANT: {
                uint16_t idx = static_cast<uint16_t>((code[ip] << 8) | code[ip + 1]);
                ip += 2;
                *top++ = curChunk->constants[idx];
                break;
            }
            case OpCode::OP_NIL:
                *top++ = Value();
                break;
            case OpCode::OP_TRUE:
                *top++ = Value(true);
                break;
            case OpCode::OP_FALSE:
                *top++ = Value(false);
                break;
            case OpCode::OP_POP:
                if (top > slots) {
                    top--;
                }
                break;
            case OpCode::OP_GET_LOCAL: {
                uint16_t slot = static_cast<uint16_t>((code[ip] << 8) | code[ip + 1]);
                ip += 2;
                *top++ = slots[slot];
                break;
            }
            case OpCode::OP_SET_LOCAL: {
                uint16_t slot = static_cast<uint16_t>((code[ip] << 8) | code[ip + 1]);
                ip += 2;
                slots[slot] = *(top - 1);
                break;
            }
            case OpCode::OP_GET_GLOBAL: {
                uint16_t idx = static_cast<uint16_t>((code[ip] << 8) | code[ip + 1]);
                ip += 2;
                const std::string& name = curChunk->constants[idx].strVal;
                auto it = globals_.find(name);
                if (it != globals_.end()) {
                    *top++ = it->second;
                } else {
                    auto fit = functions_.find(name);
                    if (fit != functions_.end()) {
                        *top++ = Value(ValueType::FUNCTION, name);
                    } else if (name == "true") {
                        *top++ = Value(true);
                    } else if (name == "false") {
                        *top++ = Value(false);
                    } else if (name == "print" || name == "silent_print" || name == "len") {
                        *top++ = Value(ValueType::FUNCTION, name);
                    } else if (name == "io") {
                        runtimeError("Library 'io' is not loaded. Please use 'use io' or 'import io' first.", curChunk, ip, frameCount);
                    } else if (name == "input") {
                        runtimeError("Function 'input' requires library 'io'. Please use 'use io' or 'import io' first.", curChunk, ip, frameCount);
                    } else if (name == "read" || name == "write") {
                        runtimeError("Function '" + name + "' requires library 'io'. Please use 'use io' or 'import io' first.", curChunk, ip, frameCount);
                    } else if (name == "math") {
                        runtimeError("Library 'math' is not loaded. Please use 'use math' or 'import math' first.", curChunk, ip, frameCount);
                    } else if (name == "time") {
                        runtimeError("Library 'time' is not loaded. Please use 'use time' or 'import time' first.", curChunk, ip, frameCount);
                    } else if (name == "net" || name == "http") {
                        runtimeError("Library '" + name + "' is not loaded. Please use 'use " + name + "' first.", curChunk, ip, frameCount);
                    } else if (name == "gui") {
                        runtimeError("Library 'gui' is not loaded. Please use 'use gui' first.", curChunk, ip, frameCount);
                    } else {
                        runtimeError("Undefined variable '" + name + "'", curChunk, ip, frameCount);
                    }
                }
                break;
            }
            case OpCode::OP_SET_GLOBAL: {
                uint16_t idx = static_cast<uint16_t>((code[ip] << 8) | code[ip + 1]);
                ip += 2;
                globals_[curChunk->constants[idx].strVal] = *(top - 1);
                break;
            }
            case OpCode::OP_ADD: {
                Value& a = *(top - 2);
                const Value& b = *(top - 1);
                if (a.type == ValueType::INT && b.type == ValueType::INT) {
                    a.intVal += b.intVal;
                } else {
                    a = a + b;
                }
                top--;
                break;
            }
            case OpCode::OP_SUB: {
                Value& a = *(top - 2);
                const Value& b = *(top - 1);
                if (a.type == ValueType::INT && b.type == ValueType::INT) {
                    a.intVal -= b.intVal;
                } else {
                    a = a - b;
                }
                top--;
                break;
            }
            case OpCode::OP_MUL: {
                Value& a = *(top - 2);
                const Value& b = *(top - 1);
                if (a.type == ValueType::INT && b.type == ValueType::INT) {
                    a.intVal *= b.intVal;
                } else {
                    a = a * b;
                }
                top--;
                break;
            }
            case OpCode::OP_DIV: {
                Value& a = *(top - 2);
                const Value& b = *(top - 1);
                if (b.asFloat() == 0.0) {
                    runtimeError("Division by zero", curChunk, ip, frameCount);
                }
                if (a.type == ValueType::INT && b.type == ValueType::INT && !(a.intVal == INT64_MIN && b.intVal == -1) && (a.intVal % b.intVal == 0)) {
                    a.intVal /= b.intVal;
                } else {
                    a = a / b;
                }
                top--;
                break;
            }
            case OpCode::OP_MOD: {
                Value& a = *(top - 2);
                const Value& b = *(top - 1);
                if (b.asInt() == 0) {
                    runtimeError("Modulo by zero", curChunk, ip, frameCount);
                }
                if (a.type == ValueType::INT && b.type == ValueType::INT) {
                    if (a.intVal == INT64_MIN && b.intVal == -1) {
                        a.intVal = 0;
                    } else {
                        a.intVal %= b.intVal;
                    }
                } else {
                    a = a % b;
                }
                top--;
                break;
            }
            case OpCode::OP_NEGATE: {
                Value& a = *(top - 1);
                if (a.type == ValueType::INT) {
                    a.intVal = -a.intVal;
                } else if (a.type == ValueType::FLOAT) {
                    a.floatVal = -a.floatVal;
                } else {
                    a = Value(0) - a;
                }
                break;
            }
            case OpCode::OP_NOT: {
                Value& a = *(top - 1);
                bool b = !a.isTruthy();
                a.type = ValueType::BOOL;
                a.boolVal = b;
                a.intVal = b ? 1 : 0;
                a.floatVal = b ? 1.0 : 0.0;
                break;
            }
            case OpCode::OP_EQUAL: {
                Value& a = *(top - 2);
                const Value& b = *(top - 1);
                if (a.type == ValueType::INT && b.type == ValueType::INT) {
                    a.type = ValueType::BOOL;
                    a.boolVal = (a.intVal == b.intVal);
                    a.intVal = a.boolVal ? 1 : 0;
                    a.floatVal = a.boolVal ? 1.0 : 0.0;
                } else {
                    a = Value(a == b);
                }
                top--;
                break;
            }
            case OpCode::OP_NOT_EQUAL: {
                Value& a = *(top - 2);
                const Value& b = *(top - 1);
                if (a.type == ValueType::INT && b.type == ValueType::INT) {
                    a.type = ValueType::BOOL;
                    a.boolVal = (a.intVal != b.intVal);
                    a.intVal = a.boolVal ? 1 : 0;
                    a.floatVal = a.boolVal ? 1.0 : 0.0;
                } else {
                    a = Value(a != b);
                }
                top--;
                break;
            }
            case OpCode::OP_LESS: {
                Value& a = *(top - 2);
                const Value& b = *(top - 1);
                if (a.type == ValueType::INT && b.type == ValueType::INT) {
                    a.type = ValueType::BOOL;
                    a.boolVal = (a.intVal < b.intVal);
                    a.intVal = a.boolVal ? 1 : 0;
                    a.floatVal = a.boolVal ? 1.0 : 0.0;
                } else {
                    a = Value(a < b);
                }
                top--;
                break;
            }
            case OpCode::OP_LESS_EQUAL: {
                Value& a = *(top - 2);
                const Value& b = *(top - 1);
                if (a.type == ValueType::INT && b.type == ValueType::INT) {
                    a.type = ValueType::BOOL;
                    a.boolVal = (a.intVal <= b.intVal);
                    a.intVal = a.boolVal ? 1 : 0;
                    a.floatVal = a.boolVal ? 1.0 : 0.0;
                } else {
                    a = Value(a <= b);
                }
                top--;
                break;
            }
            case OpCode::OP_GREATER: {
                Value& a = *(top - 2);
                const Value& b = *(top - 1);
                if (a.type == ValueType::INT && b.type == ValueType::INT) {
                    a.type = ValueType::BOOL;
                    a.boolVal = (a.intVal > b.intVal);
                    a.intVal = a.boolVal ? 1 : 0;
                    a.floatVal = a.boolVal ? 1.0 : 0.0;
                } else {
                    a = Value(a > b);
                }
                top--;
                break;
            }
            case OpCode::OP_GREATER_EQUAL: {
                Value& a = *(top - 2);
                const Value& b = *(top - 1);
                if (a.type == ValueType::INT && b.type == ValueType::INT) {
                    a.type = ValueType::BOOL;
                    a.boolVal = (a.intVal >= b.intVal);
                    a.intVal = a.boolVal ? 1 : 0;
                    a.floatVal = a.boolVal ? 1.0 : 0.0;
                } else {
                    a = Value(a >= b);
                }
                top--;
                break;
            }
            case OpCode::OP_JUMP: {
                uint16_t offset = static_cast<uint16_t>((code[ip] << 8) | code[ip + 1]);
                ip += 2 + offset;
                break;
            }
            case OpCode::OP_JUMP_IF_FALSE: {
                uint16_t offset = static_cast<uint16_t>((code[ip] << 8) | code[ip + 1]);
                ip += 2;
                if (!(*(top - 1)).isTruthy()) {
                    ip += offset;
                }
                break;
            }
            case OpCode::OP_LOOP: {
                uint16_t offset = static_cast<uint16_t>((code[ip] << 8) | code[ip + 1]);
                ip += 2;
                ip -= offset;
                break;
            }
            case OpCode::OP_CALL: {
                uint16_t fnIdx = static_cast<uint16_t>((code[ip] << 8) | code[ip + 1]);
                ip += 2;
                uint8_t argCount = code[ip++];

                Chunk* nextChunk = curChunk->cachedChunks[fnIdx];
                if (nextChunk) {
                    frame->ip = ip;
                    Value* nextSlots = top - argCount;
                    for (int i = argCount; i < nextChunk->localsCount; ++i) {
                        *top++ = Value();
                    }
                    CallFrame* newFrame = &frames_[frameCount++];
                    newFrame->chunk = nextChunk;
                    newFrame->ip = 0;
                    newFrame->slots = nextSlots;

                    frame = newFrame;
                    curChunk = nextChunk;
                    code = curChunk->code.data();
                    ip = 0;
                    slots = nextSlots;
                    break;
                }

                const std::string& name = curChunk->constants[fnIdx].strVal;

                if (name == "len") {
                    Value arg = *(--top);
                    if (arg.isList()) *top++ = Value(static_cast<int64_t>(arg.listVal ? arg.listVal->size() : 0));
                    else if (arg.isString()) *top++ = Value(static_cast<int64_t>(arg.strVal.size()));
                    else if (arg.isObject()) *top++ = Value(static_cast<int64_t>(arg.objVal ? arg.objVal->size() : 0));
                    else *top++ = Value(static_cast<int64_t>(0));
                } else if (name == "push" && argCount >= 2) {
                    Value val = *(--top);
                    Value target = *(--top);
                    top -= (argCount - 2);
                    if (target.isList()) {
                        if (!target.listVal) target.listVal = std::make_shared<std::vector<Value>>();
                        target.listVal->push_back(val);
                    }
                    *top++ = val;
                } else if (name == "pop" && argCount >= 1) {
                    Value target = *(--top);
                    top -= (argCount - 1);
                    if (target.isList() && target.listVal && !target.listVal->empty()) {
                        Value popped = target.listVal->back();
                        target.listVal->pop_back();
                        *top++ = popped;
                    } else {
                        *top++ = Value();
                    }
                } else if (name == "str") {
                    Value arg = argCount > 0 ? *(--top) : Value("");
                    *top++ = Value(arg.toString());
                } else if (name == "int") {
                    Value arg = argCount > 0 ? *(--top) : Value(static_cast<int64_t>(0));
                    *top++ = Value(arg.asInt());
                } else if (name == "float") {
                    Value arg = argCount > 0 ? *(--top) : Value(0.0);
                    *top++ = Value(arg.asFloat());
                } else if (name == "lower" || name == "to_lower" || name == "lowercase" || name == "kecil" || name == "str.lower") {
                    Value arg = argCount > 0 ? *(--top) : Value("");
                    *top++ = StandardLibrary::toLower(arg.toString());
                } else if (name == "upper" || name == "to_upper" || name == "uppercase" || name == "kapital" || name == "str.upper") {
                    Value arg = argCount > 0 ? *(--top) : Value("");
                    *top++ = StandardLibrary::toUpper(arg.toString());
                } else if (name == "incase_sensitive" || name == "incaseSensitive" || name == "incase_sensitif" ||
                           name == "incaseSensitif" || name == "incasesensitive" || name == "incasesensitif" ||
                           name == "incase" || name == "icase" || name == "iequals" || name == "iequal" || name == "str.incase_sensitive") {
                    if (argCount == 1) {
                        Value a = *(--top);
                        *top++ = StandardLibrary::toLower(a.toString());
                    } else if (argCount >= 2) {
                        Value b = *(--top);
                        Value a = *(--top);
                        top -= (argCount - 2);
                        *top++ = StandardLibrary::incaseSensitive(a.toString(), b.toString());
                    } else {
                        *top++ = Value(false);
                    }
                } else if (name == "case_sensitive" || name == "caseSensitive" || name == "case_sensitif" ||
                           name == "caseSensitif" || name == "casesensitive" || name == "casesensitif" ||
                           name == "case" || name == "equals" || name == "equal" || name == "str.case_sensitive") {
                    if (argCount == 1) {
                        Value a = *(--top);
                        *top++ = a;
                    } else if (argCount >= 2) {
                        Value b = *(--top);
                        Value a = *(--top);
                        top -= (argCount - 2);
                        *top++ = StandardLibrary::caseSensitive(a.toString(), b.toString());
                    } else {
                        *top++ = Value(false);
                    }
                } else if (name == "print" || name == "io.print") {
                    std::vector<Value> args;
                    for (size_t i = 0; i < argCount; ++i) {
                        args.push_back(*(top - argCount + i));
                    }
                    top -= argCount;
                    *top++ = StandardLibrary::print(args);
                } else if (name == "silent_print") {
                    std::vector<Value> args;
                    for (size_t i = 0; i < argCount; ++i) {
                        args.push_back(*(top - argCount + i));
                    }
                    top -= argCount;
                    *top++ = StandardLibrary::silentPrint(args);
                } else if (name == "input" || name == "io.input" || name == "io.ask" || name == "ask") {
                    if (!isLibraryLoaded("io")) {
                        runtimeError("Library 'io' is not loaded. Please use 'use io' or 'import io' first.", curChunk, ip, frameCount);
                    }
                    std::string prompt;
                    if (argCount > 0) {
                        prompt = (*(top - argCount)).toString();
                        top -= argCount;
                    }
                    *top++ = StandardLibrary::input(prompt);
                } else if (name == "read" || name == "io.read") {
                    if (!isLibraryLoaded("io")) {
                        runtimeError("Library 'io' is not loaded. Please use 'use io' or 'import io' first.", curChunk, ip, frameCount);
                    }
                    Value path = *(--top);
                    *top++ = StandardLibrary::readFile(path.toString());
                } else if (name == "write" || name == "io.write") {
                    if (!isLibraryLoaded("io")) {
                        runtimeError("Library 'io' is not loaded. Please use 'use io' or 'import io' first.", curChunk, ip, frameCount);
                    }
                    Value c = *(--top);
                    Value p = *(--top);
                    *top++ = StandardLibrary::writeFile(p.toString(), c.toString());
                } else if (name == "math.sqrt") {
                    if (!isLibraryLoaded("math")) {
                        runtimeError("Library 'math' is not loaded. Please use 'use math' or 'import math' first.", curChunk, ip, frameCount);
                    }
                    double val = argCount > 0 ? (*(--top)).asFloat() : 0.0;
                    *top++ = StandardLibrary::mathSqrt(val);
                } else if (name == "math.abs") {
                    if (!isLibraryLoaded("math")) {
                        runtimeError("Library 'math' is not loaded. Please use 'use math' or 'import math' first.", curChunk, ip, frameCount);
                    }
                    double val = argCount > 0 ? (*(--top)).asFloat() : 0.0;
                    *top++ = StandardLibrary::mathAbs(val);
                } else if (name == "math.pow") {
                    if (!isLibraryLoaded("math")) {
                        runtimeError("Library 'math' is not loaded. Please use 'use math' or 'import math' first.", curChunk, ip, frameCount);
                    }
                    double exp = (*(--top)).asFloat();
                    double base = (*(--top)).asFloat();
                    *top++ = StandardLibrary::mathPow(base, exp);
                } else if (name == "math.floor") {
                    if (!isLibraryLoaded("math")) {
                        runtimeError("Library 'math' is not loaded. Please use 'use math' or 'import math' first.", curChunk, ip, frameCount);
                    }
                    double val = argCount > 0 ? (*(--top)).asFloat() : 0.0;
                    *top++ = StandardLibrary::mathFloor(val);
                } else if (name == "math.ceil") {
                    if (!isLibraryLoaded("math")) {
                        runtimeError("Library 'math' is not loaded. Please use 'use math' or 'import math' first.", curChunk, ip, frameCount);
                    }
                    double val = argCount > 0 ? (*(--top)).asFloat() : 0.0;
                    *top++ = StandardLibrary::mathCeil(val);
                } else if (name == "math.round") {
                    if (!isLibraryLoaded("math")) {
                        runtimeError("Library 'math' is not loaded. Please use 'use math' or 'import math' first.", curChunk, ip, frameCount);
                    }
                    double val = argCount > 0 ? (*(--top)).asFloat() : 0.0;
                    *top++ = StandardLibrary::mathRound(val);
                } else if (name == "math.min") {
                    if (!isLibraryLoaded("math")) {
                        runtimeError("Library 'math' is not loaded. Please use 'use math' or 'import math' first.", curChunk, ip, frameCount);
                    }
                    double b = (*(--top)).asFloat();
                    double a = (*(--top)).asFloat();
                    *top++ = StandardLibrary::mathMin(a, b);
                } else if (name == "math.max") {
                    if (!isLibraryLoaded("math")) {
                        runtimeError("Library 'math' is not loaded. Please use 'use math' or 'import math' first.", curChunk, ip, frameCount);
                    }
                    double b = (*(--top)).asFloat();
                    double a = (*(--top)).asFloat();
                    *top++ = StandardLibrary::mathMax(a, b);
                } else if (name == "math.random") {
                    if (!isLibraryLoaded("math")) {
                        runtimeError("Library 'math' is not loaded. Please use 'use math' or 'import math' first.", curChunk, ip, frameCount);
                    }
                    top -= argCount;
                    *top++ = StandardLibrary::mathRandom();
                } else if (name == "math.sin") {
                    if (!isLibraryLoaded("math")) {
                        runtimeError("Library 'math' is not loaded. Please use 'use math' or 'import math' first.", curChunk, ip, frameCount);
                    }
                    double val = argCount > 0 ? (*(--top)).asFloat() : 0.0;
                    *top++ = StandardLibrary::mathSin(val);
                } else if (name == "math.cos") {
                    if (!isLibraryLoaded("math")) {
                        runtimeError("Library 'math' is not loaded. Please use 'use math' or 'import math' first.", curChunk, ip, frameCount);
                    }
                    double val = argCount > 0 ? (*(--top)).asFloat() : 0.0;
                    *top++ = StandardLibrary::mathCos(val);
                } else if (name == "math.tan") {
                    if (!isLibraryLoaded("math")) {
                        runtimeError("Library 'math' is not loaded. Please use 'use math' or 'import math' first.", curChunk, ip, frameCount);
                    }
                    double val = argCount > 0 ? (*(--top)).asFloat() : 0.0;
                    *top++ = StandardLibrary::mathTan(val);
                } else if (name == "time.sleep") {
                    if (!isLibraryLoaded("time")) {
                        runtimeError("Library 'time' is not loaded. Please use 'use time' or 'import time' first.", curChunk, ip, frameCount);
                    }
                    int64_t ms = argCount > 0 ? (*(--top)).asInt() : 0;
                    *top++ = StandardLibrary::timeSleep(ms);
                } else if (name == "time.now") {
                    top -= argCount;
                    *top++ = StandardLibrary::timeNow();
                } else if (name == "get" || name == "net.get" || name == "http.get") {
                    if (argCount == 1) {
                        Value u = *(--top);
                        *top++ = StandardLibrary::httpGet(u.toString());
                    } else if (argCount >= 2) {
                        Value prop = *(--top);
                        Value tgt = *(--top);
                        *top++ = tgt.getProperty(prop.toString());
                    }
                } else if (name == "send" || name == "net.send" || name == "http.send") {
                    Value d = *(--top);
                    Value t = *(--top);
                    *top++ = StandardLibrary::httpSend(t.toString(), d.toString());
                } else if (name == "app" || name == "gui.app") {
                    Value t = *(--top);
                    StandardLibrary::setAppTitle(t.toString());
                    *top++ = Value();
                } else if (name == "window" || name == "gui.window") {
                    Value h = *(--top);
                    Value w = *(--top);
                    StandardLibrary::setWindowSize(static_cast<int>(w.asInt()), static_cast<int>(h.asInt()));
                    *top++ = Value();
                } else if (name == "run" || name == "gui.run") {
                    int timeout = argCount > 0 ? static_cast<int>((*(--top)).asInt()) : -1;
                    *top++ = StandardLibrary::runApp(timeout);
                } else {
                    std::string targetName = name;
                    auto git = globals_.find(name);
                    if (git != globals_.end() && git->second.isFunction()) {
                        targetName = git->second.strVal;
                    }
                    auto it = functions_.find(targetName);
                    if (it != functions_.end()) {
                        frame->ip = ip;
                        Chunk* nChunk = it->second.get();
                        Value* nextSlots = top - argCount;
                        for (int i = argCount; i < nChunk->localsCount; ++i) {
                            *top++ = Value();
                        }
                        CallFrame* newFrame = &frames_[frameCount++];
                        newFrame->chunk = nChunk;
                        newFrame->ip = 0;
                        newFrame->slots = nextSlots;

                        frame = newFrame;
                        curChunk = nChunk;
                        code = curChunk->code.data();
                        ip = 0;
                        slots = nextSlots;
                    } else {
                        runtimeError("Undefined function '" + name + "'", curChunk, ip, frameCount);
                    }
                }
                break;
            }
            case OpCode::OP_RETURN: {
                Value result = (top > slots) ? *(--top) : Value();
                frameCount--;
                if (frameCount == 0) {
                    return result;
                }
                top = slots;
                *top++ = result;
                frame = &frames_[frameCount - 1];
                curChunk = frame->chunk;
                code = curChunk->code.data();
                ip = frame->ip;
                slots = frame->slots;
                break;
            }
            case OpCode::OP_PRINT: {
                uint8_t argCount = code[ip++];
                std::vector<Value> args;
                for (size_t i = 0; i < argCount; ++i) {
                    args.push_back(*(top - argCount + i));
                }
                top -= argCount;
                *top++ = StandardLibrary::print(args);
                break;
            }
            case OpCode::OP_SILENT_PRINT: {
                uint8_t argCount = code[ip++];
                std::vector<Value> args;
                for (size_t i = 0; i < argCount; ++i) {
                    args.push_back(*(top - argCount + i));
                }
                top -= argCount;
                *top++ = StandardLibrary::silentPrint(args);
                break;
            }
            case OpCode::OP_WRITE: {
                if (!isLibraryLoaded("io")) {
                    runtimeError("Library 'io' is not loaded. Please use 'use io' or 'import io' first.", curChunk, ip, frameCount);
                }
                Value content = *(--top);
                Value path = *(--top);
                *top++ = StandardLibrary::writeFile(path.toString(), content.toString());
                break;
            }
            case OpCode::OP_READ: {
                if (!isLibraryLoaded("io")) {
                    runtimeError("Library 'io' is not loaded. Please use 'use io' or 'import io' first.", curChunk, ip, frameCount);
                }
                Value path = *(--top);
                *top++ = StandardLibrary::readFile(path.toString());
                break;
            }
            case OpCode::OP_APP: {
                Value title = *(--top);
                StandardLibrary::setAppTitle(title.toString());
                break;
            }
            case OpCode::OP_WINDOW: {
                Value h = *(--top);
                Value w = *(--top);
                StandardLibrary::setWindowSize(static_cast<int>(w.asInt()), static_cast<int>(h.asInt()));
                break;
            }
            case OpCode::OP_RUN: {
                Value dur = *(--top);
                int timeout = dur.isNil() ? -1 : static_cast<int>(dur.asInt());
                *top++ = StandardLibrary::runApp(timeout);
                break;
            }
            case OpCode::OP_NEW:
                *top++ = Value::makeObject();
                break;
            case OpCode::OP_GET_PROP: {
                Value prop = *(--top);
                Value tgt = *(--top);
                *top++ = tgt.getProperty(prop.toString());
                break;
            }
            case OpCode::OP_SET_PROP: {
                Value val = *(--top);
                Value prop = *(--top);
                Value tgt = *(--top);
                tgt.setProperty(prop.toString(), val);
                *top++ = val;
                break;
            }
            case OpCode::OP_BUILD_LIST: {
                uint16_t count = static_cast<uint16_t>((code[ip] << 8) | code[ip + 1]);
                ip += 2;
                std::vector<Value> list;
                for (size_t i = 0; i < count; ++i) {
                    list.push_back(*(top - count + i));
                }
                top -= count;
                *top++ = Value(list);
                break;
            }
            case OpCode::OP_GET_INDEX: {
                Value idx = *(--top);
                Value tgt = *(--top);
                *top++ = tgt.getIndex(idx);
                break;
            }
            case OpCode::OP_SET_INDEX: {
                Value val = *(--top);
                Value idx = *(--top);
                Value tgt = *(--top);
                tgt.setIndex(idx, val);
                *top++ = val;
                break;
            }
            case OpCode::OP_USE: {
                uint16_t idx = static_cast<uint16_t>((code[ip] << 8) | code[ip + 1]);
                ip += 2;
                const std::string& modName = curChunk->constants[idx].strVal;
                loadLibrary(modName);
                break;
            }
        }
    }
}
