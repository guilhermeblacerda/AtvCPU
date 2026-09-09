CC = gcc
CFLAGS = -Wall -Wextra -O2
TARGET = scheduler
SRCS = escalonador.c
OBJS = $(SRCS:.c=.o)

TEMPO_TOTAL ?= 100
TAREFA1 ?= ATT 20 12 8
TAREFA2 ?= NAV 50 30 15
TAREFA3 ?=
TAREFA4 ?=
TAREFA5 ?=

all: $(TARGET) voo.txt

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c escalonador.h
	$(CC) $(CFLAGS) -c $< -o $@

voo.txt:
	@echo "$(TEMPO_TOTAL)" > voo.txt
	@echo "$(TAREFA1)" >> voo.txt
	@echo "$(TAREFA2)" >> voo.txt
	@[ -n "$(TAREFA3)" ] && echo "$(TAREFA3)" >> voo.txt || true
	@[ -n "$(TAREFA4)" ] && echo "$(TAREFA4)" >> voo.txt || true
	@[ -n "$(TAREFA5)" ] && echo "$(TAREFA5)" >> voo.txt || true

clean:
	rm -f $(OBJS) $(TARGET) *.out voo.txt

.PHONY: all clean