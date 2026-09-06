#include <iostream>
#include <cstdint>

int64_t computeSum(int64_t limit) {
    int64_t total = 0;
    int64_t i = 0;
    while (i < limit) {
        total += i;
        i++;
    }
    return total;
}

int64_t fib(int64_t n) {
    if (n <= 1) return n;
    return fib(n - 1) + fib(n - 2);
}

int main() {
    std::cout << computeSum(2000000) << "\n";
    std::cout << fib(28) << "\n";
    return 0;
}
