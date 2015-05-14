CC=g++
CFLAGS64=-Wall -m64 -O2 -DNDEBUG -std=c++11
CFLAGS32=-Wall -m32 -O2 -DNDEBUG -std=c++11

all: batchset-test64 batchset-test32
	

# 64 bit
batchset-test64: test.cpp batchset.h
	$(CC) $(CFLAGS64) -o batchset-test64 test.cpp

# 32 bit
batchset-test32: test.cpp batchset.h
	$(CC) $(CFLAGS32) -o batchset-test32 test.cpp

clean:
	-rm batchset-test32 batchset-test64
