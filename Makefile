# Compiler and flags
CC = gcc
WARNINGS = -Wall -Wextra
INCLUDE_PATH = -I./raylib-5.0_macos/include
LIB_PATH = -L./raylib-5.0_macos/lib
LIBS = -lraylib -lm

# Directories and files
SRC_DIR = src
OBJ_DIR = obj
OBJS = $(OBJ_DIR)/main.o $(OBJ_DIR)/ui.o $(OBJ_DIR)/util.o $(OBJ_DIR)/casm.o $(OBJ_DIR)/preprocess.o $(OBJ_DIR)/lexer.o $(OBJ_DIR)/animation.o $(OBJ_DIR)/complex_animations.o $(OBJ_DIR)/future.o $(OBJ_DIR)/render.o $(OBJ_DIR)/style_overrides.o
TARGET = main

# Default target
all: $(TARGET)

# Target build
$(TARGET): $(OBJS)
	$(CC) $(WARNINGS) -o $@ $^ $(LIB_PATH) $(LIBS)

# Compile source files into object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(WARNINGS) $(INCLUDE_PATH) -c $< -o $@

# Create object directory if it doesn't exist
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# Clean up build files
clean:
	rm -rf $(OBJ_DIR) $(TARGET)

.PHONY: all clean

