import sys, time

num = 8

count = 0
def search(a, s):
    global count
    a[s] = 0
    while a[s] < num:
        ok = True
        x = 0
        while x < s:
            if a[x] == a[s] or abs(a[x] - a[s]) == (s - x):
                ok = False
            x += 1

        if ok:
            if s == num - 1:
                count += 1
            else:
                search(a, s + 1)

        a[s] += 1

a = [0] * 9
t = time.time()
search(a, 0)
print time.time() - t



                    
