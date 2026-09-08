CXX ?= g++
CXXFLAGS ?= -std=c++20 -O3 -flto -Iinclude
LDFLAGS ?=

UNAME_S := $(shell uname -s 2>/dev/null)

ifeq ($(findstring MINGW,$(UNAME_S)),MINGW)
    LDFLAGS += -lwininet -lgdi32 -luser32
    TARGET = eas.exe
else ifeq ($(findstring MSYS,$(UNAME_S)),MSYS)
    LDFLAGS += -lwininet -lgdi32 -luser32
    TARGET = eas.exe
else ifeq ($(OS),Windows_NT)
    LDFLAGS += -lwininet -lgdi32 -luser32
    TARGET = eas.exe
else
    TARGET = eas
endif

SRCS = src/AST.cpp src/AotGenerator.cpp src/Bytecode.cpp src/BytecodeCompiler.cpp \
       src/Environment.cpp src/Interpreter.cpp src/Lexer.cpp src/Main.cpp \
       src/Parser.cpp src/Repl.cpp src/StandardLibrary.cpp src/Token.cpp \
       src/VM.cpp src/Value.cpp

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $(SRCS) $(LDFLAGS) -o $(TARGET)

clean:
	rm -f $(TARGET) *.o

.PHONY: all clean
