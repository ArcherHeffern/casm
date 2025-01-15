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
INTERPRETER_OBJS = $(OBJ_DIR)/interpreter_main.o $(OBJ_DIR)/interpreter.o $(OBJ_DIR)/util.o $(OBJ_DIR)/casm.o $(OBJ_DIR)/preprocess.o $(OBJ_DIR)/lexer.o $(OBJ_DIR)/test.o
VISUALISER_OBJS = $(OBJ_DIR)/visualizer_main.o $(OBJ_DIR)/ui.o $(OBJ_DIR)/util.o $(OBJ_DIR)/casm.o $(OBJ_DIR)/preprocess.o $(OBJ_DIR)/lexer.o $(OBJ_DIR)/animation.o $(OBJ_DIR)/complex_animations.o $(OBJ_DIR)/future.o $(OBJ_DIR)/render.o $(OBJ_DIR)/style_overrides.o
VISUALIZER_TARGET = visualizer
INTERPRETER_TARGET = interpreter

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
all: vis interp
vis: $(VISUALISER_OBJS) raylib $(VISUALIZER_TARGET)
interp: $(INTERPRETER_OBJS) raylib $(INTERPRETER_TARGET)

$(INTERPRETER_TARGET): $(INTERPRETER_OBJS)
	mkdir -p $(PLATFORM)
	$(CC) $(WARNINGS) -o $(PLATFORM)/$@ $^ -L$(RAYLIB_LIB_DIR) $(LIBS) $(RAYLIB_BUILD_FLAGS)

$(VISUALIZER_TARGET): $(VISUALISER_OBJS)
	mkdir -p $(PLATFORM)
	$(CC) $(WARNINGS) -o $(PLATFORM)/$@ $^ -L$(RAYLIB_LIB_DIR) $(LIBS) $(RAYLIB_BUILD_FLAGS)

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
	rm -rf $(OBJ_DIR) macos 
	cd $(RAYLIB_SRC_DIR) && $(MAKE) clean

.PHONY: all vis interp clean raylib

