a = 1
b = 0
i = 0
while i < 10:
    a = b
    b = a + b
    i += 1

print(b)