# Compiler and Flags
CC = gcc
CFLAGS = -I./include -std=gnu23 -Wall -Wextra -Werror -Wno-unused-value -Wno-stringop-overflow
LDFLAGS = -Lbuild -lcore
BUILD_DIR = build

# Source Files
LIB_SRCS = src/backup.c src/merge.c src/cwalk.c src/cson.c src/flib.c src/cebeq.c src/threading.c src/message_queue.c
CLI_SRC = src/cli.c
GUI_SRC = src/gui.c
TEST_SRC = testing/thread_test.c

# Platform-specific options
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
    LIB_TARGET = $(BUILD_DIR)/libcore.so
    RPATH = -Wl,-rpath,build
    GUI_LIBS = -lraylib -lm -ldl -lpthread
else ifeq ($(OS),Windows_NT)
    LIB_TARGET = $(BUILD_DIR)/core.dll
    CFLAGS += -DCEBEQ_EXPORT -DCEBEQ_COLOR
    GUI_LIBS = -lraylib -lgdi32 -lwinmm
else
    LIB_TARGET = $(BUILD_DIR)/libcore.so
    RPATH = -Wl,-rpath,build
    GUI_LIBS = -lraylib
endif

# Targets
.PHONY: all lib cli gui test clean help

all: lib cli gui

lib: $(LIB_TARGET)

$(LIB_TARGET): $(LIB_SRCS)
	@mkdir -p $(BUILD_DIR)
ifeq ($(UNAME_S),Linux)
	$(CC) $(CFLAGS) -fPIC -shared $^ -o $@
else
	$(CC) $(CFLAGS) -shared $^ -o $@
endif

cli: lib
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(CLI_SRC) $(LDFLAGS) $(RPATH) -o $(BUILD_DIR)/cli

gui: lib
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(GUI_SRC) $(LDFLAGS) -Llib $(GUI_LIBS) $(RPATH) -o $(BUILD_DIR)/gui

test: lib
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(TEST_SRC) $(LDFLAGS) $(RPATH) -o $(BUILD_DIR)/test

clean:
	rm -rf $(BUILD_DIR)

help:
	@echo "Usage: make <target>"
	@echo ""
	@echo "Targets:"
	@echo "  cli      Build the cli"
	@echo "  gui      Build the gui"
	@echo "  lib      Build the core functionality lib"
	@echo "  all      Build all components (lib, cli, gui)"
	@echo "  test     Build the current test"
	@echo "  clean    Remove build artifacts"
	@echo "  help     Show this message"
