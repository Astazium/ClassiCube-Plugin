LIB_NAME = mylib

CC = gcc

CFLAGS_BASE = -Wall -Wextra -fPIC -Isrc
LDFLAGS     = -shared

SRC_DIR = src

BUILD_MODE ?= release

ifeq ($(BUILD_MODE), debug)
    CFLAGS    = $(CFLAGS_BASE) -O0 -g3 -DDEBUG
    BUILD_DIR = build/debug
else
    CFLAGS    = $(CFLAGS_BASE) -O2 -DNDEBUG
    BUILD_DIR = build/release
endif

SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRCS))

ifeq ($(OS), Windows_NT)
    TARGET = $(BUILD_DIR)/$(LIB_NAME).dll
else
    TARGET = $(BUILD_DIR)/lib$(LIB_NAME).so
endif

.PHONY: all debug clean

all: $(TARGET)

debug:
	$(MAKE) BUILD_MODE=debug

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $@

clean:
	rm -rf build