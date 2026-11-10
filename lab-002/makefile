CC = gcc
RM = rm -rf
STANDARD = -std=c99
OPTIMIZATION = -O0
LIB= 

CFLAGS = $(STANDARD) -c -g $(OPTIMIZATION) -MD -Wall -pedantic -pthread
LDFLAGS = -pthread
SRC = $(wildcard *.c)
OBJ = $(addsuffix .o, $(basename $(SRC)))
TARGET = udp-send

all: $(TARGET)

$(TARGET): $(OBJ)
	@$(CC) $(LDFLAGS) $^ -o $@ $(LIB)
	@echo 'Done!'

%.o: %.c
	@$(CC) $(CFLAGS) $< -o $@

include $(wildcard *.d)

clean:
	@$(RM) *.o *.d *.out *.s *.i
	@$(RM) $(TARGET)
	@echo 'Cleaning ... Done!'
	