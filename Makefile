# directories
SRC_DIR := src
BIN_DIR := build

# files

SRC_CPP := $(wildcard $(SRC_DIR)/*.cpp)
SRC_C := $(wildcard $(SRC_DIR)/*.c)

OBJ = $(SRC_CPP:$(SRC_DIR)/%.cpp=$(BIN_DIR)/%.o)
OBJ += $(SRC_C:$(SRC_DIR)/%.c=$(BIN_DIR)/%.o)

## output biinary name
MAIN := main

# compiler / linker
CPP = g++
CC  = gcc
LD  = g++

CFLAGS += $(shell pkgconf --cflags opencv4)
LDLIBS += $(shell pkgconf --libs opencv4 ncurses)

all: $(MAIN)

$(MAIN): $(OBJ) | $(BIN_DIR)
	@echo $(OBJ)
	$(LD) $(CFLAGS) $(LDLIBS) $^ -o $@

$(BIN_DIR)/%.o: $(SRC_DIR)/%.cpp $(BIN_DIR)
	$(CPP) $(CFLAGS) -c $< -o $@

$(BIN_DIR)/%.o: $(SRC_DIR)/%.c $(BIN_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN_DIR):
	mkdir -p $@

clean:
	rm -rv $(BIN_DIR)

.PHONY: clean all
