def computeSum(limit):
    total = 0
    i = 0
    while i < limit:
        total = total + i
        i = i + 1
    return total

print(computeSum(2000000))

def fib(n):
    if n <= 1:
        return n
    else:
        return fib(n - 1) + fib(n - 2)

print(fib(28))
