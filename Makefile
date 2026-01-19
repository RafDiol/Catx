# Compiler
CC = gcc

# Source file
SRC = catx.c

# Target names
TARGET_DEBUG = catx_debug
TARGET_RELEASE = catx

# Compiler flags
CFLAGS_DEBUG = -Wall -Wextra
CFLAGS_RELEASE = -Wall -Wextra -O2

# Default target: build debug
all: debug

# Debug build
debug: $(SRC)
	$(CC) $(CFLAGS_DEBUG) -o $(TARGET_DEBUG) $(SRC)
	@echo "Debug build complete: $(TARGET_DEBUG)"

# Release build
release: $(SRC)
	$(CC) $(CFLAGS_RELEASE) -o $(TARGET_RELEASE) $(SRC)
	@echo "Release build complete: $(TARGET_RELEASE)"

# Clean build artifacts
clean:
	rm -f $(TARGET_DEBUG) $(TARGET_RELEASE) *.o
	@echo "Cleaned up build artifacts"

# Phony targets
.PHONY: all debug release clean