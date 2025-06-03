# Makefile for building cli, gui, lib, test, all

CC := gcc
CFLAGS := -I./include -Wall -Wextra -Werror -Wno-unused-value -Wno-stringop-overflow
LDFLAGS := -Lbuild -lcore
BUILD_DIR := build

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
    SHARED := -fPIC -shared
    LIB_NAME := libcore.so
    RPATH := -Wl,-rpath,$(BUILD_DIR)
else ifeq ($(OS),Windows_NT)
    SHARED := -shared
    LIB_NAME := core.dll
    RPATH :=
endif

SRC_LIB := src/backup.c src/merge.c src/cwalk.c src/cson.c src/flib.c src/cebeq.c
SRC_CLI := src/cli.c
SRC_GUI := src/gui.c

.PHONY: all lib cli gui test help clean

all: lib cli gui

lib: $(BUILD_DIR)/$(LIB_NAME)

$(BUILD_DIR)/$(LIB_NAME): $(SRC_LIB) | $(BUILD_DIR)
	$(CC) $(CFLAGS) $(SRC_LIB) $(SHARED) -o $(BUILD_DIR)/$(LIB_NAME)

cli: $(BUILD_DIR)/cli

$(BUILD_DIR)/cli: $(SRC_CLI) lib
	$(CC) $(CFLAGS) $(SRC_CLI) $(LDFLAGS) $(RPATH) -o $(BUILD_DIR)/cli

gui: $(BUILD_DIR)/gui

$(BUILD_DIR)/gui: $(SRC_GUI) lib
	$(CC) $(CFLAGS) $(SRC_GUI) $(LDFLAGS) $(RPATH) -o $(BUILD_DIR)/gui

test: $(BUILD_DIR)/test

$(BUILD_DIR)/test: $(SRC_LIB)
	$(CC) $(CFLAGS) $(SRC_LIB) -o $(BUILD_DIR)/test

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

help:
	@echo "Usage: make <target>"
	@echo
	@echo "Targets:"
	@echo "  cli     Build the CLI application"
	@echo "  gui     Build the GUI application"
	@echo "  lib     Build the core functionality library"
	@echo "  all     Build lib, cli, and gui"
	@echo "  test    Build the test executable"
	@echo "  help    Show this help message"
	@echo "  clean   Remove build artifacts"

clean:
	rm -rf $(BUILD_DIR)
