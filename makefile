# Compiler configuration
CC = gcc

# TODO: fix these paths; think I have SDL3 in homebrew
CFLAGS = -Wall -Wextra -std=c11 -Iinclude -I/opt/homebrew/include/
LDFLAGS = -L/opt/homebrew/lib -lSDL3 -lSDL3_image -lSDL3_ttf
TARGET = build/app
SRC_DIR = src
BUILD_DIR = build

# Source files
SRCS = $(SRC_DIR)/main.c $(SRC_DIR)/gamestate.c $(SRC_DIR)/water_particles.c $(SRC_DIR)/camera.c $(SRC_DIR)/debug.c $(SRC_DIR)/renderer.c $(SRC_DIR)/fire.c $(SRC_DIR)/input.c $(SRC_DIR)/pool.c $(SRC_DIR)/editor.c $(SRC_DIR)/level_io.c

all: $(TARGET)

$(TARGET): $(SRCS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean
