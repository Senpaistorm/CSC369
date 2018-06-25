import os

mem_size = [50, 100, 150, 200]
rep_algo = ["rand", "fifo", "lru", "clock", "opt"]
trace_file = ["traceprogs/tr-simpleloop.ref", "traceprogs/tr-matmul.ref", "traceprogs/tr-blocked.ref", "traceprogs/tr-interesting.ref"]
#trace_file = ["traceprogs/tr-simpleloop.ref"]
for k in rep_algo:
	os.system("echo " + k + ">> result.txt")
	for i in trace_file[3:]:
		os.system("echo " + i + ">> result.txt")
		for j in mem_size:
			os.system("echo " + str(j) + ">> result.txt")
			cmd = "sim -f "+ i + " -m " + str(j) + " -s 50000 -a " + k + " >> result.txt"
			#cmd = "sim -f traceprogs/tr-simpleloop.ref -m 50 -s 50000 -a " + rep_algo[j] + " >> result.txt"
			os.system(cmd)

f1 = open("result.txt", "r")
f2 = open("output.txt","w")
for line in f1.readlines():
	if line[0] != '[' and line[0] !='\t' and line[0] !=' ' and line[0] != '\n':
		if(line[0].isdigit() or line[0].islower() or (line[0].isupper() and (line[0]!= 'T' and line[5] != 'r'))):
			f2.write(line)
f1.close()
f2.close()