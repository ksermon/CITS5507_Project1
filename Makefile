# Makefile for Project1.c using OpenMP

# Compiler and flags
CC = gcc
CFLAGS = -fopenmp -Wall -Wextra -pedantic -std=c11 -fsanitize=address -Wshadow -Wfloat-equal -Wunreachable-code -Werror -O2 -g

# Target executable
TARGET = Project1

# Source and object files
SRCS = Project1.c
OBJS = $(SRCS:.c=.o)

# Default rule
all: $(TARGET)

# Linking
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Compilation
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up
clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all clean
