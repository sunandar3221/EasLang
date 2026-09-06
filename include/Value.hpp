#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <iostream>

enum class ValueType {
    NIL,
    BOOL,
    INT,
    FLOAT,
    STRING,
    LIST,
    OBJECT
};

class Value {
public:
    ValueType type;
    bool boolVal;
    int64_t intVal;
    double floatVal;
    std::string strVal;
    std::shared_ptr<std::vector<Value>> listVal;
    std::shared_ptr<std::unordered_map<std::string, Value>> objVal;

    Value();
    Value(bool b);
    Value(int64_t i);
    Value(int i);
    Value(double f);
    Value(std::string s);
    Value(const char* s);
    Value(std::vector<Value> list);
    Value(std::unordered_map<std::string, Value> obj);

    Value(const Value& other);
    Value(Value&& other) noexcept;
    Value& operator=(const Value& other);
    Value& operator=(Value&& other) noexcept;

    operator int64_t() const { return asInt(); }
    operator double() const { return asFloat(); }
    operator bool() const { return isTruthy(); }

    static Value makeList();
    static Value makeObject();

    bool isNil() const;
    bool isBool() const;
    bool isInt() const;
    bool isFloat() const;
    bool isNumber() const;
    bool isString() const;
    bool isList() const;
    bool isObject() const;

    bool isTruthy() const;
    double asFloat() const;
    int64_t asInt() const;
    std::string toString() const;

    bool operator==(const Value& other) const;
    bool operator!=(const Value& other) const;
    Value operator+(const Value& other) const;
    Value operator-(const Value& other) const;
    Value operator*(const Value& other) const;
    Value operator/(const Value& other) const;
    Value operator%(const Value& other) const;
    bool operator<(const Value& other) const;
    bool operator>(const Value& other) const;
    bool operator<=(const Value& other) const;
    bool operator>=(const Value& other) const;

    Value getIndex(const Value& index) const;
    void setIndex(const Value& index, const Value& val);
    Value getProperty(const std::string& key) const;
    void setProperty(const std::string& key, const Value& val);
};

inline int64_t operator+(int64_t i, const Value& v) { return i + v.asInt(); }
inline int64_t operator-(int64_t i, const Value& v) { return i - v.asInt(); }
inline int64_t operator*(int64_t i, const Value& v) { return i * v.asInt(); }
inline int64_t operator/(int64_t i, const Value& v) { int64_t d = v.asInt(); return d ? i / d : 0; }
inline bool operator<(int64_t i, const Value& v) { return i < v.asInt(); }
inline bool operator<=(int64_t i, const Value& v) { return i <= v.asInt(); }
inline bool operator>(int64_t i, const Value& v) { return i > v.asInt(); }
inline bool operator>=(int64_t i, const Value& v) { return i >= v.asInt(); }
inline bool operator==(int64_t i, const Value& v) { return i == v.asInt(); }
inline bool operator!=(int64_t i, const Value& v) { return i != v.asInt(); }

std::ostream& operator<<(std::ostream& os, const Value& val);
