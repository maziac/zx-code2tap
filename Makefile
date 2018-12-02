# Makefile for csv2music.


OBJS=code2tap.o
HEADER=

DEFINES =
INCLUDES = -I.

SRC_DIR = src
BIN_DIR = bin
OBJ_DIR = obj

_OBJS = $(patsubst %,$(OBJ_DIR)/%,$(OBJS))
_HEADERS = $(patsubst %,$(SRC_DIR)/%,$(HEADER))

CC=gcc
CFLAGS=-Wall $(INCLUDES) $(DEFINES) -DDEBUG -g
CXX=g++
CXXFLAGS=-Wall -std=c++11 $(INCLUDES) $(DEFINES) -DDEBUG -g
LD=g++
LDFLAGS=


default:	all


all:	$(BIN_DIR)/zxcode2tap


$(OBJ_DIR)/%.o:	$(SRC_DIR)/%.cpp $(_HEADERS)
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@


$(OBJ_DIR)/%.o:	$(SRC_DIR)/%.c $(_HEADERS)
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@


$(BIN_DIR)/zxcode2tap:	$(_OBJS)
	@mkdir -p $(BIN_DIR)
	$(LD) $^ -o $@ $(LDFLAGS)


clean:
	-rm -f $(OBJ_DIR)/*.o
	-rm -f $(BIN_DIR)/zxcode2tap


