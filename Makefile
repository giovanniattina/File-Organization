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
func1:
	./programaTrab1 1 turmaB-dadosPBLE.csv
func2:
	./programaTrab1 2

func3:
	./programaTrab1 3 'codINEP' 12345678
func4:
	./programaTrab1 4 1
func5:
	./programaTrab1 5 1
func6:
	./programaTrab1 6 12345678 '29/04/2018' 'SP' 'USP-Sao Carlos' 'Sao Carlos' CTBC
func7:
	./programaTrab1 7 1 12345678 '29/04/2018' '' 'USP' 'Sao Carlos' ''
func8:
	./programaTrab1 8
func9:
	./programaTrab1 9




