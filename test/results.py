import sys

iter= sys.argv[1]
iter= int(iter)
a=[]

first = "k,n,min,sec,total\n"

with open("time.txt", "r") as f:
    for i in range(iter):
        args = ""
        temp0 = f.readline()
        lk = f.readline().strip()
        ln = f.readline().strip()
        temp = f.readline()
        time = f.readline().strip().split()[1]
        temp1 = f.readline()
        temp2 = f.readline()

        min,sec = time.split("m")
        min = min.strip()
        sec = sec[:-1]
        total = float(min)*60 + float(sec)
        args = lk + "," + ln + "," + min + "," + sec + "," + str(total)+ "\n"
        a.append(args)

with open("output.csv", "w") as f:
    f.write(first)
    f.writelines(a)