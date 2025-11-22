CC = gcc
CFLAGS = -Wall

OBJ = blank.o ssu_score.o main.o

all: ssu_score

blank.o: blank.c blank.h
	$(CC) $(CFLAGS) -c blank.c -o blank.o

ssu_score.o: ssu_score.c ssu_score.h
	$(CC) $(CFLAGS) -c ssu_score.c -o ssu_score.o

main.o: main.c
	$(CC) $(CFLAGS) -c main.c -o main.o

ssu_score: $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o ssu_score

clean:
	rm -f $(OBJ) ssu_score