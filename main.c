#include <stdio.h>
#include <stdlib.h>
#include "BtreeIndex.h"
#include "bufferNRU.h"

#define VETSIZE 500
int main(){
	
	int cod[VETSIZE];
	int val = 1000;
	for (int i =0; i< VETSIZE; i++)
		cod[i] = val--;
	FILE *file = fopen("testando.dat", "wb");
	int i;
	
	createBtreeIndexFile();
	bufferpool *buffer = createBuffer();
	printBuffer(buffer);
	

	for(i = 0; i<sizeof(cod)/4; i++){
		
		fwrite(cod+i, sizeof(int), 1, file);
	}
	
	fclose(file);

	for(i = 0; i< sizeof(cod)/4; i++){
		insertKeyToIndex(buffer, cod[i], i);
	}
	
	saveAllPages(buffer);
	free(buffer);
	int cont2 = 0;
	for (int j = 0; j < sizeof(cod)/4 ; j++){
		BtreeRemove(cod[j]);
		int cont = 0;
		cont2++;
		for(int i = 0; i< sizeof(cod)/4; i++){
			if( BtreeSearch(cod[i]) < 0){
				printf("-----> NAO ENCONTROU %d\n", cod[i]);
				cont ++;
			}
		}
		printf("\n ----- FIM CICLO %d -----", j+1);
		printf(" %d chaves removidas e %d chaves nao encontradas\n", cont2, cont);
		if( cont != cont2)
			printf("algo deu errado ......\n\n");
		else
			printf("\n");
	}

/*	
	buffer = loadBuffer();
	insertKeyToIndex(buffer, 461, 121);
	saveAllPages(buffer);
	free(buffer);

	for(int i = 0; i< sizeof(cod)/4; i++){
		if( BtreeSearch(cod[i]) < 0)
			printf("-----> NAO ENCONTROU %d\n", cod[i]);
	}
	
	printBtree();
*/
	return EXIT_SUCCESS;
}


