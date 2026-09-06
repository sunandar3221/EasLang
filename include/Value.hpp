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

    explicit operator int64_t() const { return asInt(); }
    explicit operator double() const { return asFloat(); }
    explicit operator bool() const { return isTruthy(); }

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

inline Value operator+(const Value& a, int64_t b) { return a + Value(b); }
inline Value operator+(int64_t a, const Value& b) { return Value(a) + b; }
inline Value operator+(const Value& a, int b) { return a + Value(static_cast<int64_t>(b)); }
inline Value operator+(int a, const Value& b) { return Value(static_cast<int64_t>(a)) + b; }
inline Value operator+(const Value& a, double b) { return a + Value(b); }
inline Value operator+(double a, const Value& b) { return Value(a) + b; }
inline Value operator+(const Value& a, const std::string& b) { return a + Value(b); }
inline Value operator+(const std::string& a, const Value& b) { return Value(a) + b; }
inline Value operator+(const Value& a, const char* b) { return a + Value(b); }
inline Value operator+(const char* a, const Value& b) { return Value(a) + b; }

inline Value operator-(const Value& a, int64_t b) { return a - Value(b); }
inline Value operator-(int64_t a, const Value& b) { return Value(a) - b; }
inline Value operator*(const Value& a, int64_t b) { return a * Value(b); }
inline Value operator*(int64_t a, const Value& b) { return Value(a) * b; }
inline Value operator/(const Value& a, int64_t b) { return a / Value(b); }
inline Value operator/(int64_t a, const Value& b) { return Value(a) / b; }
inline Value operator%(const Value& a, int64_t b) { return a % Value(b); }
inline Value operator%(int64_t a, const Value& b) { return Value(a) % b; }

inline bool operator==(const Value& a, int64_t b) { return a == Value(b); }
inline bool operator==(int64_t a, const Value& b) { return Value(a) == b; }
inline bool operator!=(const Value& a, int64_t b) { return a != Value(b); }
inline bool operator!=(int64_t a, const Value& b) { return Value(a) != b; }
inline bool operator<(const Value& a, int64_t b) { return a < Value(b); }
inline bool operator<(int64_t a, const Value& b) { return Value(a) < b; }
inline bool operator<=(const Value& a, int64_t b) { return a <= Value(b); }
inline bool operator<=(int64_t a, const Value& b) { return Value(a) <= b; }
inline bool operator>(const Value& a, int64_t b) { return a > Value(b); }
inline bool operator>(int64_t a, const Value& b) { return Value(a) > b; }
inline bool operator>=(const Value& a, int64_t b) { return a >= Value(b); }
inline bool operator>=(int64_t a, const Value& b) { return Value(a) >= b; }

std::ostream& operator<<(std::ostream& os, const Value& val);
