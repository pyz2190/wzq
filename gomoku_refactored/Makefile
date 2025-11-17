# Makefile for Gomoku AI - Refactored Version

CC = gcc
CFLAGS = -O2 -Wall -Wextra
TARGET = gomoku_refactored
OBJS = board.o evaluator.o search.o main.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

board.o: board.c board.h types.h
	$(CC) $(CFLAGS) -c board.c

evaluator.o: evaluator.c evaluator.h board.h types.h
	$(CC) $(CFLAGS) -c evaluator.c

search.o: search.c search.h board.h evaluator.h types.h
	$(CC) $(CFLAGS) -c search.c

main.o: main.c types.h board.h evaluator.h search.h
	$(CC) $(CFLAGS) -c main.c

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
