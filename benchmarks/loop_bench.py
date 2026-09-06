def sumLoop(limit):
    total = 0
    i = 0
    while i < limit:
        total = total + i
        i = i + 1
    return total

print(sumLoop(10000000))
