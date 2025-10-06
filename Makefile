# Compiler and flags
CC = gcc
CFLAGS = -Wall -g

# File paths
SRC = src/ls-v1.0.0.c
OBJ = obj/ls-v1.0.0.o
BIN = bin/ls

# Default target: build the ls program
$(BIN): $(OBJ)
	$(CC) $(OBJ) -o $(BIN)

# Rule to compile .c file to .o
$(OBJ): $(SRC)
	@mkdir -p obj
	$(CC) $(CFLAGS) -c $(SRC) -o $(OBJ)

# Clean build files
clean:
	rm -rf obj/*.o $(BIN)

# Run the executable
run:
	./$(BIN)
