#pragma once

#include "Value.hpp"
#include <string>
#include <unordered_map>
#include <memory>
#include <vector>

class BlockStmt;

struct FunctionDef {
    std::string name;
    std::vector<std::string> params;
    std::shared_ptr<BlockStmt> body;
    std::shared_ptr<class Environment> closure;
};

class Environment : public std::enable_shared_from_this<Environment> {
public:
    explicit Environment(std::shared_ptr<Environment> enclosing = nullptr);

    void define(const std::string& name, Value val);
    bool assign(const std::string& name, const Value& val);
    bool get(const std::string& name, Value& out) const;
    bool contains(const std::string& name) const;

    void defineFunction(const std::string& name, FunctionDef fn);
    bool getFunction(const std::string& name, FunctionDef& out) const;

    std::shared_ptr<Environment> getEnclosing() const;

private:
    std::unordered_map<std::string, Value> values_;
    std::unordered_map<std::string, FunctionDef> functions_;
    std::shared_ptr<Environment> enclosing_;
};
