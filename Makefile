CC = gcc
TARGET = a.out

SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

CFLAGS = -Wall -Wextra -g -I$(SRC_DIR) 
LDFLAGS =

SOURCES := $(wildcard $(SRC_DIR)/*.c)

OBJECTS := $(SOURCES:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

EXECUTABLE = $(BIN_DIR)/$(TARGET)


all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	@echo "Linking..."
	@mkdir -p $(BIN_DIR)
	$(CC) $(LDFLAGS) -o $@ $^

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@echo "Compiling application: $<..."
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	@echo "Cleaning up..."
	@rm -rf $(OBJ_DIR)/* $(BIN_DIR)/*

cleaner: clean
	@echo "Cleaning up and deleting all assembly files..."
	@rm */*.o */*.s */*.out

.PHONY: all clean 