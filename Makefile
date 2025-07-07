CC = gcc
TARGET = a.out

SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

CFLAGS = -Wall -Wextra -g -I$(SRC_DIR) 
LDFLAGS =

APP_SOURCES := $(wildcard $(SRC_DIR)/*.c)

APP_OBJECTS := $(APP_SOURCES:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
ALL_OBJECTS := $(APP_OBJECTS) 

EXECUTABLE = $(BIN_DIR)/$(TARGET)


all: $(EXECUTABLE)

$(EXECUTABLE): $(ALL_OBJECTS)
	@echo "Linking..."
	@mkdir -p $(BIN_DIR)
	$(CC) $(LDFLAGS) -o $@ $^

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@echo "Compiling application: $<..."
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	@echo "Cleaning up..."
	@rm -rf $(OBJ_DIR) $(BIN_DIR)

.PHONY: all clean 