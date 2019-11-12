TARGETS= test
N = 14
P = 1
CROSS_TOOL = 
CC_CPP = $(CROSS_TOOL)g++
CC_C = $(CROSS_TOOL)gcc

CFLAGS = -Wall -lpthread

all: clean $(TARGETS)

test: test.c threadpool.c
	$(CC_C) $(CFLAGS) $@.c threadpool.c -o $@

run_test: test
	qsub -v n=$(N),p=$(P) run.sh

clean:
	rm -f $(TARGETS)
