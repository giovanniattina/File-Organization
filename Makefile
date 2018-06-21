all:
	gcc -o programaTrab2 -g ./src/*c -I./include -lm -std=gnu99
io:
	gcc -o io io.c -g ./src/arquivos.c ./src/BtreeIndex.c ./src/bufferNRU.c -std=gnu99 -I./include
run:
	./programaTrab2
valgrind:
	gcc -o programaTrab2 -g ./src/*c -I./include -lm -std=gnu99
	valgrind --tool=memcheck --leak-check=full ./programaTrab2
test1:
	./programaTrab2 1 turmaB-dadosPBLE.csv
	./programaTrab2 1 turmaB-dadosPBLE
test2:
	./programaTrab2 1 debug.csv
	./programaTrab2 2
func1:
	./programaTrab2 1 turmaB-dadosPBLE.csv
func2:
	./programaTrab2 2

func3:
	./programaTrab2 3 'codINEP' 12345678
func4:
	./programaTrab2 4 1
func5:
	./programaTrab2 5 1
func6:
	./programaTrab2 6 12345678 '29/04/2018' 'SP' 'USP-Sao Carlos' 'Sao Carlos' CTBC
func7:
	./programaTrab2 7 1 12345678 '29/04/2018' '' 'USP' 'Sao Carlos' ''
func8:
	./programaTrab2 8
func9:
	./programaTrab2 9
func12:
	./programaTrab2 12 12345678
func13:
	./programaTrab2 13 12345678




