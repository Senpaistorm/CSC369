import random
import subprocess

def test_cars(n):
    f = open("test_cars.txt", "w")
    for i in range(1, n+1):
        car_id = i
        in_dir = random.randint(0, 3)
        out_dir = random.randint(0, 3)
        s = "{} {} {}\n".format(car_id, in_dir, out_dir)
        f.write(s)
    f.close()

def test_result():
    f = open("output.txt", "r")
    printed = []
    verify = []
    line = f.readline()
    while line != "---\n":
        printed.append(line.strip())
        line = f.readline()
    
    line = f.readline()
    while line != "===\n":
        verify.append(line.strip())
        line = f.readline()
    f.close()
    printed.sort()
    verify.sort()
    return printed == verify

if __name__ == "__main__":
    test_cars(100000)
#    cars = input("Test how many cars? (0 to check result) ")    
#    test_cars(int(cars))
#    p1 = subprocess.Popen(["./traffic", "test_cars.txt"], 
#                          stdout=open("output.txt", "w"))
#    print(test_result())
