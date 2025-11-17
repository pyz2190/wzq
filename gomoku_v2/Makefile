# Makefile for Gomoku AI v2

CC = gcc
CFLAGS = -O2 -Wall -Wextra -std=c99
TARGET = gomoku_v2

# Source files
SRCS = main.c game_state.c evaluator.c search.c
OBJS = $(SRCS:.c=.o)

# Default target
all: $(TARGET)

# Link
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Compile
%.o: %.c
	$(CC) $(CFLAGS) -c $<

# Clean
clean:
	rm -f $(OBJS) $(TARGET)

# Rebuild
rebuild: clean all

.PHONY: all clean rebuild
