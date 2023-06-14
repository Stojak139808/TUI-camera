# directories
SRC_DIR := src
BIN_DIR := build

# files

SRC_C := $(wildcard $(SRC_DIR)/*.c)

OBJ += $(SRC_C:$(SRC_DIR)/%.c=$(BIN_DIR)/%.o)

## output biinary name
MAIN := main

# compiler / linker
CC  = gcc
LD  = gcc

CFLAGS =
LDLIBS = -lncurses -lturbojpeg

all: $(MAIN)

$(MAIN): $(OBJ) | $(BIN_DIR)
	$(LD) $(CFLAGS) $(LDLIBS) $^ -o $@

$(BIN_DIR)/%.o: $(SRC_DIR)/%.c $(BIN_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR):
	mkdir -p $@

clean:
	rm -rv $(BIN_DIR)

.PHONY: clean all
