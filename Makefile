CC = gcc
TARGET = a.out

# --- Directories ---
SRC_DIR = src
RUNTIME_DIR = runtime
OBJ_DIR = obj
BIN_DIR = bin

# --- Flags ---
# Add -I$(RUNTIME_DIR) if your main source files need to #include headers from the runtime
CFLAGS = -Wall -Wextra -g -I$(SRC_DIR) -I$(RUNTIME_DIR)
LDFLAGS =

# --- Source Files ---
# Find all .c files in the main source directory
APP_SOURCES := $(wildcard $(SRC_DIR)/*.c)
# Define the runtime source file explicitly
RUNTIME_SRC := $(RUNTIME_DIR)/runtime.c

# --- Object Files ---
# Generate object file names for the application sources
APP_OBJECTS := $(APP_SOURCES:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
# Define the runtime object file name
RUNTIME_OBJ := $(OBJ_DIR)/runtime.o
# Combine all object files needed for the final executable
ALL_OBJECTS := $(APP_OBJECTS) $(RUNTIME_OBJ)

# --- Executable ---
EXECUTABLE = $(BIN_DIR)/$(TARGET)

# --- Targets ---

# Default target
all: $(EXECUTABLE)

# New target to build only the runtime
runtime: $(RUNTIME_OBJ)

# Rule to link the final executable
$(EXECUTABLE): $(ALL_OBJECTS)
	@echo "Linking..."
	@mkdir -p $(BIN_DIR)
	$(CC) $(LDFLAGS) -o $@ $^

# Pattern rule to compile application source files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@echo "Compiling application: $<..."
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

# Explicit rule to compile the runtime source file
$(RUNTIME_OBJ): $(RUNTIME_SRC)
	@echo "Compiling runtime: $<..."
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

# Clean up build artifacts
clean:
	@echo "Cleaning up..."
	@rm -rf $(OBJ_DIR) $(BIN_DIR)

# Phony targets are not files
.PHONY: all clean runtime