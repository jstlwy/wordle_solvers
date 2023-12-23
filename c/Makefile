CC := gcc
CFLAGS := -std=c17 -Wall -Werror -Wextra

src := $(wildcard *.c)
obj := $(addsuffix .o, $(basename $(src)))
bin := wordle_solver

.PHONY: all clean

all: $(bin)

$(bin): $(obj)
	$(CC) $^ -o $@

main.o: main.c
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm -f $(obj) $(bin)
