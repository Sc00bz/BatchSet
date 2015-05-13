CC=g++
CFLAGS64=-c -Wall -m64 -O2 -DNDEBUG
CFLAGS32=-c -Wall -m32 -O2 -DNDEBUG
LFLAGS64=-m64
LFLAGS32=-m32

all: batchset-test64 batchset-test32
	

# 64 bit
batchset-test64: test.cpp batchset.h
	echo "Oh shit I'm too drunk for this... fuck Linux... fine I'll fix it in the morning"
	$(CC) $(LFLAGS64) -o batchset-test64 test.cpp

# 32 bit
batchset-test32: test.cpp batchset.h
	echo "Oh shit I'm too drunk for this... fuck Linux... fine I'll fix it in the morning"
	$(CC) $(LFLAGS32) -o batchset-test32 test.cpp

clean:
	-rm batchset-test32 batchset-test64
