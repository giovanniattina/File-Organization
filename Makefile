all:
	gcc -o prog main.c BtreeIndex.c bufferNRU.c arquivos.c -g -std=gnu99
run:
	./prog
valg:
	valgrind ./prog
