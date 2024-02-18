CC = gcc

IDIR = ./include
SRCDIR = ./src
SOURCES = $(SRCDIR)/*.c
EXE_NAME = barbearia_paralela
SIMULATION_TIME = 10

CFLAGS = -Wall -I$(IDIR) -g -lpthread

all: compile

compile: $(SOURCES)
	$(CC) $(SOURCES) $(CFLAGS) -o $(EXE_NAME)

run:
	./$(EXE_NAME) $(SIMULATION_TIME)