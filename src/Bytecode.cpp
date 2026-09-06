#include "Bytecode.hpp"

Chunk::Chunk(std::string chunkName, int fnArity)
    : name(std::move(chunkName)), arity(fnArity), localsCount(0) {}

size_t Chunk::addConstant(Value val) {
    constants.push_back(std::move(val));
    cachedChunks.push_back(nullptr);
    return constants.size() - 1;
}

void Chunk::emit(uint8_t byte, int line) {
    code.push_back(byte);
    lines.push_back(line);
}

void Chunk::emitOp(OpCode op, int line) {
    emit(static_cast<uint8_t>(op), line);
}

void Chunk::emitShort(uint16_t val, int line) {
    emit(static_cast<uint8_t>((val >> 8) & 0xff), line);
    emit(static_cast<uint8_t>(val & 0xff), line);
}

void Chunk::writeConstant(Value val, int line) {
    size_t idx = addConstant(std::move(val));
    emitOp(OpCode::OP_CONSTANT, line);
    emitShort(static_cast<uint16_t>(idx), line);
}
