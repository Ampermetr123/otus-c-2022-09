AS = nasm  # NOTE: yasm тоже подойдёт

all: asm-prog c-prog

c-prog: reverse.c
	$(CC) -Wall -Wextra -Wpedantic -std=c11 $^ -o $@

asm-prog: main.o
	$(CC) -no-pie $^ -o $@

main.o: main.asm
	$(AS) -felf64 -g $^

clean:
	rm -f c-prog asm-prog *.o *.out core

PHONY: clean
