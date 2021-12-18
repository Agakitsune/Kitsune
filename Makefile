CXX = g++
SHELL := /bin/bash

WFLAGS = -Wall -Werror -Wextra -W
IFLAGS = -I ./include/
LFLAGS = -lX11 -lglog
CXXFLAGS = $(IFLAGS) $(LFLAGS) #$(WFLAGS)

SRC = $(wildcard src/*.cpp)
OBJ = $(SRC:.cpp=.o)
NAME = kitsune

all: build

build: $(OBJ)
	@echo "[---BUILD---]"
	@echo "Building Project..."
	@$(CXX) -o $(NAME) $(OBJ) $(CXXFLAGS)
	@echo "Project was built succesfully"

clean:
	@echo "[---CLEAN---]"
	@echo "Cleaning Project..."
	@rm $(OBJ)
	@rm $(NAME)
	@unset XEPHYR
	@echo "Project was clean succesfully"

run:
	@echo "[---RUN---]"
	@echo "Running Project..."
	@if ! [ -f $(NAME) ]; then\
		echo "Project was not built";\
		make build;\
	fi
	@xinit ./$(NAME) -- "$$(whereis -b Xephyr | cut -f2 -d' ')" :100 -ac -screen 1280x720 -host-cursor
	@echo "Project was ran succesfully"