CXX ?= g++
CXXFLAGS ?= -std=c++20 -O3 -flto -Iinclude
LDFLAGS ?=

UNAME_S := $(shell uname -s 2>/dev/null)

ifeq ($(findstring MINGW,$(UNAME_S)),MINGW)
    LDFLAGS += -lwininet -lgdi32 -luser32
    TARGET = fasthon.exe
    ALIAS = eas.exe
else ifeq ($(findstring MSYS,$(UNAME_S)),MSYS)
    LDFLAGS += -lwininet -lgdi32 -luser32
    TARGET = fasthon.exe
    ALIAS = eas.exe
else ifeq ($(OS),Windows_NT)
    LDFLAGS += -lwininet -lgdi32 -luser32
    TARGET = fasthon.exe
    ALIAS = eas.exe
else
    TARGET = fasthon
    ALIAS = eas
endif

SRCS = src/AST.cpp src/AotGenerator.cpp src/Bytecode.cpp src/BytecodeCompiler.cpp \
       src/Diagnostic.cpp src/Environment.cpp src/Interpreter.cpp src/Lexer.cpp src/Main.cpp \
       src/Parser.cpp src/Repl.cpp src/StandardLibrary.cpp src/Token.cpp \
       src/VM.cpp src/Value.cpp

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $(SRCS) $(LDFLAGS) -o $(TARGET)
	@cp $(TARGET) $(ALIAS) 2>/dev/null || copy $(TARGET) $(ALIAS) 2>nul || true

clean:
	@rm -f $(TARGET) $(ALIAS) *.o 2>/dev/null || del /Q /F $(TARGET) $(ALIAS) *.o 2>nul || true

.PHONY: all clean
