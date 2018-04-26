#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arquivos.h>

#define REGSIZE 87
#define REGCABSIZE 5


#define TAM_CAMPOFIXO1 4
#define TAM_CAMPOFIXO2 10
#define TAM_CAMPOFIXO3 2

#define TAM_NOMEESCOLA 0
#define TAM_MUNICIPIO 1
#define TAM_PRESTADORA 2

void writeRegister(FILE *file, registro* reg, int* tamCampos){
	
	char zero = '0';
	long int offset = ftell(file);	//posicao que comecara a escrever
	//escrevendo campos de tamanho fixo
	fwrite(&(reg->codINEP), TAM_CAMPOFIXO1, 1, file);
	fwrite(reg->dataAtiv, TAM_CAMPOFIXO2, 1, file);
	fwrite(reg->uf, TAM_CAMPOFIXO3, 1, file);

	//escrevendo indicador de tamanho e campos de tamanho variavel
	fwrite(tamCampos, 4, 1, file);	
	fwrite(reg->nomeEscola, tamCampos[TAM_NOMEESCOLA], 1, file);

	fwrite(tamCampos +1, 4, 1, file);	
	fwrite(reg->municipio, tamCampos[TAM_MUNICIPIO], 1, file);

	fwrite(tamCampos +2, 4, 1, file);	
	fwrite(reg->prestadora, tamCampos[TAM_PRESTADORA], 1, file);	
	
	offset = ftell(file) - offset;	//apos escrever calcula tamanho final do registro

	for(int i = 0; i<REGSIZE - offset ; i++)
		fwrite(&zero, 1, 1, file); //completa espaco do registro com caracteres '0'

}



/** \brief Funcao que le o arquivo CSV.
  *
  * \param *fileName - Nome do arquivo a ser aberto.
  */
void readCsvFile(char *fileName){
  FILE *csvFile, *binFile;
  registro *reg;
  regCabecalho *cabecalho;
  char linha[100];
  int tamanhoCampos[3]; // Armazenara tamanho dos campos variaveis na ordem: 0-nome escola; 1-municipo; 2-prestadora

  cabecalho = (regCabecalho*)calloc(1, sizeof(cabecalho));
  cabecalho->topoPilha = -1;

  csvFile = fopen(fileName, "r");

  if(!csvFile){
	printf("Falha no carregamento do arquivo.\n");
  } else {
	
	
	binFile = fopen("diskPage.dat", "wb");
	//verificar se abriu

	//escrevendo cabecalho
	fwrite(&(cabecalho->status), 1, 1, binFile);
	fwrite(&(cabecalho->topoPilha), sizeof(cabecalho->topoPilha), 1, binFile);

    while(fgets(linha, 100, csvFile)){

    	reg = (registro*)malloc(sizeof(registro));

		parseLinha(linha, reg, tamanhoCampos);
    //  	printReg(reg, tamanhoCampos);
		
		writeRegister(binFile, reg, tamanhoCampos);	
		
      	if(reg->prestadora != NULL)
			free(reg->prestadora);
		if(reg->municipio != NULL)
			free(reg->municipio);
		if(reg->nomeEscola != NULL)
			free(reg->nomeEscola);
		free(reg);  
    }

    printf("Arquivo carregado.\n");
    fclose(csvFile);

	cabecalho->status = 1;
	rewind(binFile);
	fwrite(&(cabecalho->status), 1, 1, binFile);

	fclose(binFile);
	free(cabecalho);

  }
}



void printReg(registro *reg, int *tamanhoCampos){

  printf("%d ", reg->codINEP);

	//nao tem \0, entao tem que printar um por um
	for(int i = 0; i<10 ; i++){
		printf("%c", reg->dataAtiv[i]);
	}
	printf(" ");
	for(int i = 0; i< 2 ; i++){
		printf("%c", reg->uf[i]);
	}
	printf(" ");
	for(int i = 0; i<tamanhoCampos[TAM_NOMEESCOLA] ; i++){
		printf("%c", reg->nomeEscola[i]);
	}
	printf(" ");
	for(int i = 0; i<tamanhoCampos[TAM_MUNICIPIO] ; i++){
		printf("%c", reg->municipio[i]);
	}
	printf(" ");
	for(int i = 0; i<tamanhoCampos[TAM_PRESTADORA] ; i++){
		printf("%c", reg->prestadora[i]);
	}
	printf("\n");


}

/*
	Separa uma string pelo ';', armazenando cada parte.
	
	char* linha : string que sera percorrida.
	registro* reg : ponteiro para estrutura que armazenara os dados extraidos.
	int *tamVet : vetor que sera preenchido com o tamanho calculado para os campos de tamanho variavel
*/
void parseLinha(char *linha, registro *reg, int *tamVet){
  char c;
  char dado[60];
  int i, j = 0;
  int campo = 0, tamCampo;


  c = linha[j++];
  while (c != '\n') {		//percorre ate o fim da linha
    i = 0;
	tamCampo = 0;	//

	while((c != ';') && (c != '\n')){	//percorre campo ate encontrar ';'
      dado[i++] = c;
      c = linha[j++];
	  tamCampo++;
    }

    campo++;
    dado[i] = '\0';
    if((c != '\n')){
      c = linha[j++];
    }
    
	switch (campo) {
      case 1:			//armazena prestadora na estrutura
	  	
		tamVet[TAM_PRESTADORA] = tamCampo;
        if(strlen(dado) != 0){
          reg->prestadora = malloc(strlen(dado));
          memcpy(reg->prestadora, dado, strlen(dado));
        }
		else
			reg->prestadora = NULL;

        break;

      case 2:		//armazena data da atividade
        if(strlen(dado) == 0)
          memcpy(reg->dataAtiv, "0000000000", 10);
        else
          memcpy(reg->dataAtiv, dado, strlen(dado));
        break;

      case 3:		//armazena codigo do INEP
        reg->codINEP = atoi(dado);
        break;

      case 4:		//armazena nome da escola

		tamVet[TAM_NOMEESCOLA] = tamCampo;
		if(strlen(dado) != 0){
          reg->nomeEscola = malloc(strlen(dado));
          memcpy(reg->nomeEscola, dado, strlen(dado));
        }
		else
			reg->nomeEscola = NULL;

        break;

      case 5:		//armazena municipio

		tamVet[TAM_MUNICIPIO] = tamCampo;
		if(strlen(dado) != 0){
          reg->municipio = malloc(strlen(dado));
          memcpy(reg->municipio, dado, strlen(dado));
        }
		else
			reg->municipio = NULL;
        break;

      case 6:		//armazena uf
        
          memcpy(reg->uf, dado, strlen(dado));
        break;
    }

    if((c == '\n') && (campo == 5)){	// uf nula resulta em c = '\n', terminando o loop antes de chegar ao caso 6
      memcpy(reg->uf, "00", 2);
    }
  }
}
