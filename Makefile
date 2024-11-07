# Compiler and flags
CC = gcc
WARNINGS = -Wall -Wextra
INCLUDE_PATH = -I./raylib-5.0/src
LIB_PATH = -L./raylib-5.0/src
LIBS = -lraylib -lm

# Directories and files
SRC_DIR = src
OBJ_DIR = obj
RAYLIB_SRC_DIR = raylib-5.0/src
OBJS = $(OBJ_DIR)/visualizer_main.o $(OBJ_DIR)/ui.o $(OBJ_DIR)/util.o $(OBJ_DIR)/casm.o $(OBJ_DIR)/preprocess.o $(OBJ_DIR)/lexer.o $(OBJ_DIR)/animation.o $(OBJ_DIR)/complex_animations.o $(OBJ_DIR)/future.o $(OBJ_DIR)/render.o $(OBJ_DIR)/style_overrides.o
TARGET = main

# Platform-specific setup
ifeq ($(shell uname), Darwin)
    PLATFORM = macos
    RAYLIB_LIB_DIR = raylib-5.0/src
    RAYLIB_BUILD_FLAGS = -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo
else
    PLATFORM = linux
    RAYLIB_LIB_DIR = raylib-5.0/src
    RAYLIB_BUILD_FLAGS = -lGL -lm -lpthread -ldl -lrt -lX11
endif

# Default target
all: $(OBJ_DIR) raylib $(TARGET)

# Target build
$(TARGET): $(OBJS)
	$(CC) $(WARNINGS) -o $@ $^ -L$(RAYLIB_LIB_DIR) $(LIBS) $(RAYLIB_BUILD_FLAGS)

# Compile source files into object files in obj directory
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(WARNINGS) $(INCLUDE_PATH) -c $< -o $@

# Create obj directory if it doesn't exist
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# Build Raylib from source
raylib: $(RAYLIB_SRC_DIR)/libraylib.a
$(RAYLIB_SRC_DIR)/libraylib.a:
	cd $(RAYLIB_SRC_DIR) && $(MAKE) PLATFORM=PLATFORM_DESKTOP

# Distribution target for macOS
macos: clean raylib all
	mkdir -p dist/macos
	cp $(TARGET) dist/macos/
	cp -R $(RAYLIB_LIB_DIR) dist/macos/

# Distribution target for Linux
linux: clean raylib all
	mkdir -p dist/linux
	cp $(TARGET) dist/linux/
	cp -R $(RAYLIB_LIB_DIR) dist/linux/

# Windows cross-compilation setup
windows: CC = x86_64-w64-mingw32-gcc
windows: INCLUDE_PATH = -I./raylib-5.0/src
windows: LIB_PATH = -L./raylib-5.0/src
windows: LIBS += -lopengl32 -lgdi32 -lwinmm
windows: raylib_windows
	mkdir -p dist/windows
	$(CC) $(WARNINGS) -o dist/windows/$(TARGET).exe $(OBJS) $(LIB_PATH) $(LIBS)

# Build Raylib for Windows
raylib_windows:
	cd $(RAYLIB_SRC_DIR) && $(MAKE) PLATFORM=PLATFORM_DESKTOP CC=x86_64-w64-mingw32-gcc


# Clean up build files
clean:
	rm -rf $(OBJ_DIR) $(TARGET) dist
	cd $(RAYLIB_SRC_DIR) && $(MAKE) clean

.PHONY: all clean dist-macos dist-linux raylib

