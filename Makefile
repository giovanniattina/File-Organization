all:
	gcc -o programaTrab1 -g ./src/*c -I./include -lm -std=gnu99
run:
	./programaTrab1
valgrind:
	gcc -o programaTrab1 -g ./src/*c -I./include -lm -std=gnu99
	valgrind --tool=memcheck --leak-check=full ./programaTrab1
test1:
	./programaTrab1 1 turmaB-dadosPBLE.csv
	./programaTrab1 1 turmaB-dadosPBLE
test2:
	./programaTrab1 1 debug.csv
	./programaTrab1 2
