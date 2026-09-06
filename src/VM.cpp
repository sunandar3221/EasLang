#include "VM.hpp"
#include <iostream>

VM::VM() {}

void VM::registerFunction(const std::string& name, std::shared_ptr<Chunk> chunk) {
    functions_[name] = std::move(chunk);
}

void VM::registerFunctions(const std::unordered_map<std::string, std::shared_ptr<Chunk>>& fns) {
    for (const auto& kv : fns) {
        functions_[kv.first] = kv.second;
    }
}

Value VM::run(Chunk* chunk) {
    if (!chunk) return Value();

    stack_.resize(STACK_MAX);
    frames_.resize(FRAMES_MAX);

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
                    *top++ = Value();
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
                a = a / b;
                top--;
                break;
            }
            case OpCode::OP_MOD: {
                Value& a = *(top - 2);
                const Value& b = *(top - 1);
                a = a % b;
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
                a = Value(!a.isTruthy());
                break;
            }
            case OpCode::OP_EQUAL: {
                Value& a = *(top - 2);
                const Value& b = *(top - 1);
                a = Value(a == b);
                top--;
                break;
            }
            case OpCode::OP_NOT_EQUAL: {
                Value& a = *(top - 2);
                const Value& b = *(top - 1);
                a = Value(a != b);
                top--;
                break;
            }
            case OpCode::OP_LESS: {
                Value& a = *(top - 2);
                const Value& b = *(top - 1);
                if (a.type == ValueType::INT && b.type == ValueType::INT) {
                    a = Value(a.intVal < b.intVal);
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
                    a = Value(a.intVal <= b.intVal);
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
                    a = Value(a.intVal > b.intVal);
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
                    a = Value(a.intVal >= b.intVal);
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
                } else if (name == "print") {
                    std::vector<Value> args;
                    for (size_t i = 0; i < argCount; ++i) {
                        args.push_back(*(top - argCount + i));
                    }
                    top -= argCount;
                    StandardLibrary::print(args);
                    *top++ = Value();
                } else if (name == "read") {
                    Value path = *(--top);
                    *top++ = StandardLibrary::readFile(path.toString());
                } else if (name == "write") {
                    Value c = *(--top);
                    Value p = *(--top);
                    *top++ = StandardLibrary::writeFile(p.toString(), c.toString());
                } else if (name == "get") {
                    if (argCount == 1) {
                        Value u = *(--top);
                        *top++ = StandardLibrary::httpGet(u.toString());
                    } else if (argCount >= 2) {
                        Value prop = *(--top);
                        Value tgt = *(--top);
                        *top++ = tgt.getProperty(prop.toString());
                    }
                } else if (name == "send") {
                    Value d = *(--top);
                    Value t = *(--top);
                    *top++ = StandardLibrary::httpSend(t.toString(), d.toString());
                } else if (name == "app") {
                    Value t = *(--top);
                    StandardLibrary::setAppTitle(t.toString());
                    *top++ = Value();
                } else if (name == "window") {
                    Value h = *(--top);
                    Value w = *(--top);
                    StandardLibrary::setWindowSize(static_cast<int>(w.asInt()), static_cast<int>(h.asInt()));
                    *top++ = Value();
                } else if (name == "run") {
                    int timeout = argCount > 0 ? static_cast<int>((*(--top)).asInt()) : -1;
                    *top++ = StandardLibrary::runApp(timeout);
                } else {
                    auto it = functions_.find(name);
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
                        top -= argCount;
                        *top++ = Value();
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
                StandardLibrary::print(args);
                break;
            }
            case OpCode::OP_WRITE: {
                Value content = *(--top);
                Value path = *(--top);
                *top++ = StandardLibrary::writeFile(path.toString(), content.toString());
                break;
            }
            case OpCode::OP_READ: {
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
        }
    }
}
