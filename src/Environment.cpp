#include "Environment.hpp"

Environment::Environment(std::shared_ptr<Environment> enclosing)
    : enclosing_(std::move(enclosing)) {}

void Environment::define(const std::string& name, Value val) {
    values_[name] = std::move(val);
}

bool Environment::assign(const std::string& name, const Value& val) {
    auto it = values_.find(name);
    if (it != values_.end()) {
        it->second = val;
        return true;
    }
    if (enclosing_ && enclosing_->assign(name, val)) {
        return true;
    }
    values_[name] = val;
    return true;
}

bool Environment::get(const std::string& name, Value& out) const {
    auto it = values_.find(name);
    if (it != values_.end()) {
        out = it->second;
        return true;
    }
    if (enclosing_) {
        return enclosing_->get(name, out);
    }
    return false;
}

bool Environment::contains(const std::string& name) const {
    if (values_.find(name) != values_.end()) return true;
    if (enclosing_) return enclosing_->contains(name);
    return false;
}

void Environment::defineFunction(const std::string& name, FunctionDef fn) {
    functions_[name] = std::move(fn);
}

bool Environment::getFunction(const std::string& name, FunctionDef& out) const {
    auto it = functions_.find(name);
    if (it != functions_.end()) {
        out = it->second;
        return true;
    }
    if (enclosing_) {
        return enclosing_->getFunction(name, out);
    }
    return false;
}

std::shared_ptr<Environment> Environment::getEnclosing() const {
    return enclosing_;
}
