# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -O3
LDFLAGS = -lsensors

# Installation directories
PREFIX = /usr/local
BINDIR = $(PREFIX)/bin

# Executable name
TARGET = yambar-sensors

# Source files
SRC = main.c

# Object files
OBJ = $(SRC:.c=.o)

# Default target
all: $(TARGET)

# Compile source files into object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Link the target
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Install the program
install: $(TARGET)
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 $(TARGET) $(DESTDIR)$(BINDIR)

# Uninstall the program
uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(TARGET)

# Clean up build files
clean:
	rm -f $(TARGET) $(OBJ) *~

# Phony targets
.PHONY: all install uninstall clean
