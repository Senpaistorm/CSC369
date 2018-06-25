SRCS = simpleloop.c matmul.c blocked.c interesting.c
PROGS = simpleloop matmul blocked interesting

all : $(PROGS)

$(PROGS) : % : %.c
	gcc -Wall -g -o $@ $<


traces: $(PROGS)
	./runit simpleloop
	./runit matmul 100
	./runit blocked 100 25
	./runit interesting


.PHONY: clean
clean : 
	rm -f simpleloop matmul blocked interesting tr-*.ref *.marker *~
