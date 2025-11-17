# Makefile for Low Isomorphism Gomoku AI

CC = gcc
CFLAGS = -O2 -Wall -Wextra -std=c99
LDFLAGS = -lm

TARGET = gomoku_lowiso
SOURCE = gomoku_lowiso.c

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCE) $(LDFLAGS)
	@echo "Build successful: $(TARGET)"

clean:
	rm -f $(TARGET)
	@echo "Cleaned build artifacts"

test: $(TARGET)
	@echo "Testing $(TARGET)..."
	@echo -e "START 1\nTURN\nEND" | ./$(TARGET)
	@echo "Test completed"

help:
	@echo "Available targets:"
	@echo "  all    - Build the program (default)"
	@echo "  clean  - Remove build artifacts"
	@echo "  test   - Build and run a simple test"
	@echo "  help   - Show this help message"
