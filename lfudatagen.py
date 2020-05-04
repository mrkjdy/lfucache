import random

ops = 10000000

# number of ops
print(str(ops))

# the actual ops
for i in range(ops):
    if random.randint(0,1) == 0:
        print("g " + str(random.randint(0,100)))
    else:
        print("p " + str(random.randint(0,100)) + " " + str(random.randint(0,100)))