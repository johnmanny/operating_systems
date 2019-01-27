

TARGET = P1 P2 P3 P4 P5 p1fxn
LIBS = -lm
CC = gcc
CFLAGS = -g -Wall


all: P1 P2 P3 P4 P5 

P1.o: P1.c
	$(CC) $(CFLAGS) -c $< -o $@

P2.o: P2.c
	$(CC) $(CFLAGS) -c $< -o $@

P3.o: P3.c
	$(CC) $(CFLAGS) -c $< -o $@

P4.o: P4.c
	$(CC) $(CFLAGS) -c $< -o $@

P5.o: P5.c
	$(CC) $(CFLAGS) -c $< -o $@

p1fxns.o: p1fxns.c
	$(CC) $(CFLAGS) -c $< -o $@


P1: p1fxns.o P1.o
	$(CC) -g -Wall $(LIBS) $^ -o $@

P2: p1fxns.o P2.o 
	$(CC) -g -Wall $(LIBS) $^ -o $@

P3: p1fxns.o P3.o 
	$(CC) -g -Wall $(LIBS) $^ -o $@

P4: p1fxns.o P4.o 
	$(CC) -Wall $(LIBS) $^ -o $@

P5: P5.o 
	$(CC) -Wall $(LIBS) $^ -o $@


clean:
	rm -f *.o
	rm -f P1 P2 P3 P4 P5
