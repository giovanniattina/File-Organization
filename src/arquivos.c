#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "arquivos.h"
#include "BtreeIndex.h"

#define REGSIZE 87
#define HEADERSIZE 5
#define DATAATIVSIZE 10
#define UFSIZE 2

/** \brief Funcao que libera memoria ocupada por uma estrutura de registro e seus campos
  *
  * \param **reg - Ponteiro para o ponteiro do registro a liberar.
  */
void freeRegister(registro **reg){
	free((*reg)->dataAtiv);
	free((*reg)->uf);
	free((*reg)->nomeEscola);
	free((*reg)->municipio);
	free((*reg)->prestadora);
	free(*reg);
	*reg = NULL;
}

/** \brief Funcao que modifica status de um arquivo
  *
  * \param *fileName - Arquivo a ser modificado
  * \param char status - Valor que sera atribuido
	*
  * \return field - Campo lido
  */
void setStatus(FILE *binFile, unsigned char status){
	rewind(binFile);
	fwrite(&status, 1, 1, binFile);
}

/** \brief Funcao que verifica o status de um arquivo, permitindo saber se esta corrompido ou nao.
	*				Ao final, o ponteiro de arquivo passa a apontar o inicio do arquivo.
  *
	*\param FILE *binFile - ponteiro para o arquivo que sera verificado
	*
	*\return 1 - arquivo corrompido ou 0 - arquivo ok
  */
unsigned char isCorruptedFile(FILE *binFile){
	unsigned char status;
	rewind(binFile);
	fread(&status, 1, 1, binFile);
	rewind(binFile);
	return !status;
}

/** \brief Funcao que separa a linha lida do arquivo CSV em campos.
  *
  * \param *fileName - Arquivo a ser aberto.
  *
  * \return field - Campo lido
  */
char *getField(FILE *fileName){
	char c = 0;
	int contador = 0;
	char *field = NULL;

	do{ //percorre toda a linha até achar o delimitador
		c = fgetc(fileName);
		field = (char *)realloc(field, sizeof(char)*(contador+1));
		field[contador++] = c;
	} while(c != 10 && c != ';');

	field[contador-1] = '\0';
	return field; //retorna o campo para ser salvo
}

/** \brief Funcao que le o arquivo CSV.
  *
  * \param *fileName - Arquivo a ser aberto.
  *
  * \return 0 - erro ao abrir arquivo ou numReg - Quantidade de registros.
  */
int readFile(char *fileName){
	int size, numReg = 0, topoPilha = -1;
	char zero = '0', *codINEP;
	FILE *csvFile = fopen(fileName, "r");
	FILE *binFile = fopen("data.dat", "wb");

	if(!csvFile){
		if(binFile)
			fclose(binFile);
		return 0;
	}
	if(!binFile){
		if(csvFile)
			fclose(csvFile);
		return 0;
	}
	//criando arquivo de indice vazio
	if( !createBtreeIndexFile() ){
		fclose(csvFile);
		fclose(binFile);
		return 0;
	}
	fseek(csvFile, 0, SEEK_END); // encontra o tamanho do arquivo CSV
	size = ftell(csvFile);
	rewind(csvFile);

	setStatus(binFile, 0);
	fwrite(&topoPilha, sizeof(int), 1, binFile);

	while(ftell(csvFile) != size) { // percorre todo o arquivo CSV e atribui os campos as variaveis para salvar
		registro *reg = malloc(sizeof(registro));

		reg->prestadora = getField(csvFile);
		reg->tam_prestadora = (int)strlen(reg->prestadora);
		reg->dataAtiv = getField(csvFile);
		codINEP = getField(csvFile);	//precisa guardar referencia para liberar da memoria
		reg->codINEP = atoi(codINEP);
		free(codINEP);
		reg->nomeEscola = getField(csvFile);
		reg->tam_nomeEscola = (int)strlen(reg->nomeEscola);
		reg->municipio = getField(csvFile);
		reg->tam_municipio = (int)strlen(reg->municipio);
		reg->uf = getField(csvFile);

		// Salva todos os campos no arquivo binario
		fwrite(&reg->codINEP, sizeof(int), 1, binFile);
		if (strlen(reg->dataAtiv)) {
			fwrite(reg->dataAtiv, DATAATIVSIZE*sizeof(char), 1, binFile);
		} else {
			fwrite("0000000000", 10*sizeof(char), 1, binFile);
		}
		if (strlen(reg->uf)) {
			fwrite(reg->uf, UFSIZE*sizeof(char), 1, binFile);
		} else {
			fwrite("00", 2*sizeof(char), 1, binFile);
		}
		fwrite(&reg->tam_nomeEscola, sizeof(int), 1, binFile);
		fwrite(reg->nomeEscola, reg->tam_nomeEscola * sizeof(char), 1, binFile);
		fwrite(&reg->tam_municipio, sizeof(int), 1, binFile);
		fwrite(reg->municipio, reg->tam_municipio * sizeof(char), 1, binFile);
		fwrite(&reg->tam_prestadora, sizeof(int), 1, binFile);
		fwrite(reg->prestadora, reg->tam_prestadora * sizeof(char), 1, binFile);

		int tamReg = 28 + reg->tam_prestadora + reg->tam_municipio + reg->tam_nomeEscola;

		for (size_t i = 0; i < REGSIZE - tamReg; i++) { // preenche os bytes que estão sobrando até chegar no tamanho do registro
			fwrite(&zero, sizeof(char), 1, binFile);
		}

		
		//FAZER AQUI INSERCAO NO ARQUIVO DE INDICE USANDO ARVORE B
		insertKeyToIndex( reg->codINEP, numReg);	
		numReg++;
		freeRegister(&reg);
		
	}

	fclose(csvFile);

	setStatus(binFile, 1);
	fclose(binFile);
	

	return numReg;
}

/** \brief Funcao que verifica se determinado registro existe num arquivo
  *
	*\param int RRN - Registro que sera verificado
	*\param FILE *binFile - Arquivo no qual sera procurado o registro
	*
	*\return 0 falha ao abrir aquivo ou -1 nenhum registro encotrado ou 1 registros encontrados
  */
unsigned char isValidReg(int RRN, FILE *binFile){
	int removed;
	int pos, size;
	pos = ftell(binFile);
	fseek(binFile, 0, SEEK_END);
	size = ftell(binFile);

	if(RRN < 0 || HEADERSIZE + RRN*REGSIZE >= size) return 0;//RRN grande demais

	fseek(binFile, HEADERSIZE + RRN*REGSIZE, SEEK_SET);
	fread(&removed, 4, 1, binFile);
	fseek(binFile, pos, SEEK_SET);
	return (removed == -1) ? 0 : 1;
}

/** \brief Funcao que le o arquivo binario e imprime todos os registros.
  *
	*\return 0 falha ao abrir aquivo ou -1 nenhum registro encotrado ou 1 registros encontrados
  */
int showAll(){
	int size, RRN = 0;
	FILE *binFile = fopen("data.dat", "rb+");

	if(!binFile) return 0;

	if(isCorruptedFile(binFile)){
		fclose(binFile);
		return 0;
	}

	setStatus(binFile, 0);

 	fseek(binFile, 0, SEEK_END);
	size = ftell(binFile);
	fseek(binFile, HEADERSIZE, SEEK_SET);

	int tamReg = 0;

 	while(ftell(binFile) != size) {

		if(isValidReg(RRN, binFile)){

			// aloca as memórias para cada campo
			registro *reg = malloc(sizeof(registro));
			reg->nomeEscola = NULL;
			reg->municipio = NULL;
			reg->prestadora = NULL;
			reg->dataAtiv = malloc(10*sizeof(char));
			reg->uf = malloc(2*sizeof(char));

			// Le os dados do arquivo binario e salva nas variaveis
			fread(&reg->codINEP, sizeof(int), 1, binFile);
			fread(reg->dataAtiv, 10 * sizeof(char), 1, binFile);
			fread(reg->uf, 2 * sizeof(char), 1, binFile);
			fread(&reg->tam_nomeEscola, sizeof(int), 1, binFile);
			reg->nomeEscola = realloc(reg->nomeEscola, reg->tam_nomeEscola);
			fread(reg->nomeEscola, reg->tam_nomeEscola * sizeof(char), 1, binFile);
			fread(&reg->tam_municipio, sizeof(int), 1, binFile);
			reg->municipio = realloc(reg->municipio, reg->tam_municipio);
			fread(reg->municipio, reg->tam_municipio * sizeof(char), 1, binFile);
			fread(&reg->tam_prestadora, sizeof(int), 1, binFile);
			reg->prestadora = realloc(reg->prestadora, reg->tam_prestadora);
			fread(reg->prestadora, reg->tam_prestadora * sizeof(char), 1, binFile);

			printReg(reg);

			freeRegister(&reg);
		
  		}
		RRN++;	//sendo valido ou nao, passa para poximo registro
		fseek(binFile, HEADERSIZE + RRN*REGSIZE, SEEK_SET);
	}

	setStatus(binFile, 1);
	fclose(binFile);

	return (RRN > 0) ? 1 : -1;

	// return 1;
}

/** \brief Funcao que busca um determinado registro no arquivo binario pelo RRN e o imprime.
  *
  * \param RRN - O numero RRN a ser procurado.
	*
	*\return 0 - falha ao abrir aquivo ou -1 - nenhum registro encotrado ou 1 - registros encontrados
  */
int findRRN(int RRN){
	int size;

	FILE *binFile = fopen("data.dat", "rb+");

	if(!binFile) return 0;
	if(isCorruptedFile(binFile)){
		fclose(binFile);
		return 0;
	}
	setStatus(binFile, 0);

	// busca o byte offset do registro procurado no arquivo
	fseek(binFile, HEADERSIZE + RRN * REGSIZE, SEEK_SET);

	if(!isValidReg(RRN, binFile)){//RRN removido ou grande demais
		// normaliza status e finaliza
		setStatus(binFile, 1);
		fclose(binFile);
		return -1;
	}

	registro *reg = calloc(sizeof(registro), 1);
	reg->dataAtiv = malloc(DATAATIVSIZE*sizeof(char));
	reg->uf = malloc(UFSIZE*sizeof(char));

	// Le os dados do arquivo binario e salva nas variaveis
	fread(&reg->codINEP, sizeof(int), 1, binFile);
	fread(reg->dataAtiv, DATAATIVSIZE * sizeof(char), 1, binFile);
	fread(reg->uf, UFSIZE * sizeof(char), 1, binFile);
	fread(&reg->tam_nomeEscola, sizeof(int), 1, binFile);
	reg->nomeEscola = realloc(reg->nomeEscola, reg->tam_nomeEscola);
	fread(reg->nomeEscola, reg->tam_nomeEscola * sizeof(char), 1, binFile);
	fread(&reg->tam_municipio, sizeof(int), 1, binFile);
	reg->municipio = realloc(reg->municipio, reg->tam_municipio);
	fread(reg->municipio, reg->tam_municipio * sizeof(char), 1, binFile);
	fread(&reg->tam_prestadora, sizeof(int), 1, binFile);
	reg->prestadora = realloc(reg->prestadora, reg->tam_prestadora);
	fread(reg->prestadora, reg->tam_prestadora * sizeof(char), 1, binFile);

	printReg(reg);

	setStatus(binFile, 1);
	fclose(binFile);
	freeRegister(&reg);

	return 1;
}

/** \brief Funcao que imprime os campos do registro no formato especificado.
  *
  * \param *reg - O registro a ser impresso.
  */
void printReg(registro *reg){
	int i;

	//imprime campos de tamanho fixo
	printf("%d ", reg->codINEP);
	//como nao ha '\0', deve-se imprimir um caracter por vez
	for(i = 0; i<DATAATIVSIZE; i++)
		printf("%c", reg->dataAtiv[i]);
	printf(" ");

	for(i = 0; i<UFSIZE; i++)
		printf("%c", reg->uf[i]);
	printf(" ");

	//imprime campos de tamanho variavel e indicadores de tamanho
	printf("%d ", reg->tam_nomeEscola);
	if (reg->tam_nomeEscola) {
		for(i = 0; i < reg->tam_nomeEscola; i++){
			printf("%c", reg->nomeEscola[i]);
    	}
		printf(" ");
	}

	printf("%d ", reg->tam_municipio);

	if (reg->tam_municipio) {
		for(i = 0; i < reg->tam_municipio; i++){
			printf("%c", reg->municipio[i]);
		}
		printf(" ");
	}

	printf("%d ", reg->tam_prestadora);
	if (reg->tam_prestadora) {
		for(i = 0; i < reg->tam_prestadora; i++){
			printf("%c", reg->prestadora[i]);
		}
	}
	printf("\n");
}

/** \brief Funcao que remove logicamente um registro, utilizando abordagem dinamica
  *
  * \param int RRN - registro a ser removido
  *
	* \return 0 - falha no aquivo ou -1 - registro RRN nao encotrado ou 1 - registros removido
	*/
int removeReg( int RRN){

	int removed = -1, topoPilha;
	FILE *binFile = fopen("data.dat", "rb+");

	if(!binFile) return 0;

	if(isCorruptedFile(binFile)) {
		fclose(binFile);
		return 0;
	}

	setStatus(binFile, 0);

	if(!isValidReg(RRN, binFile)){
		setStatus(binFile, 1);
		fclose(binFile);
		return -1;
	}

	//pegando topo da pilha
	fread(&topoPilha, sizeof(int), 1, binFile);

	//removendo registro logicamente
	fseek(binFile, HEADERSIZE + RRN*REGSIZE, SEEK_SET);
	fwrite(&removed, sizeof(int), 1, binFile);

	//escreve nele o topo da pilha e guarda na pilha o RRN dele
	fwrite(&topoPilha, sizeof(int), 1, binFile);
	topoPilha = RRN;
	fseek(binFile, 1, SEEK_SET);
	fwrite(&topoPilha, sizeof(int), 1, binFile);

	setStatus(binFile, 1);

	rewind(binFile);
	fclose(binFile);
	return 1;
}


void writeReg(FILE *binFile, int codINEP, char *dataAtiv, char *uf, char *nomeEscola, char *municipio, char *prestadora){

	int sizeEsc, sizePrest, sizeMun, tamReg;
	char zero = '0';

	// escrevendo campos fixos passados
	fwrite(&codINEP, sizeof(int), 1, binFile);

	if(strlen(dataAtiv) == 0)
		fwrite("0000000000", DATAATIVSIZE, 1, binFile);
	else
		fwrite(dataAtiv, DATAATIVSIZE, 1, binFile);

	if(strlen(uf) == 0)
		fwrite("00", UFSIZE, 1, binFile);
	else
		fwrite(uf, UFSIZE, 1, binFile);

	//escrevendo campos variaveis e indicadores de tamanho
	sizeEsc = strlen(nomeEscola);
	fwrite(&sizeEsc, sizeof(int), 1, binFile);
	fwrite(nomeEscola, sizeEsc, 1, binFile);

	sizeMun = strlen(municipio);
	fwrite(&sizeMun, sizeof(int), 1, binFile);
	fwrite(municipio, sizeMun, 1, binFile);

	sizePrest = strlen(prestadora);
	fwrite(&sizePrest, sizeof(int), 1, binFile);
	fwrite(prestadora, sizePrest, 1, binFile);

	//preenchendo com 0 o que sobrou de espaco no registro
	tamReg = 28 + sizeEsc + sizeMun + sizePrest;
	for (size_t i = 0; i < REGSIZE - tamReg; i++)
		fwrite(&zero, sizeof(char), 1, binFile);
}

/** \brief Funcao que insere um novo registro reaproveitando espaco das remocoes logicas.
  *
  * \params - dados do registro a ser inserido
  *
	* \return 0 - falha no aquivo ou 1 - registros inserido
	*/
int insertReg(int codINEP, char *dataAtiv, char *uf, char *nomeEscola, char *municipio, char *prestadora){

	int topoPilha, novoTopo;
	FILE *binFile = fopen("data.dat", "rb+");

	if(!binFile) return 0;

	if( isCorruptedFile(binFile) ){
		fclose(binFile);
		return 0;
	}

	setStatus(binFile, 0);
	fread(&topoPilha, sizeof(int), 1, binFile);

	if(topoPilha > -1){
		//atualiza valor da pilha
		fseek(binFile, HEADERSIZE + topoPilha*REGSIZE + sizeof(int), SEEK_SET);
		fread(&novoTopo, sizeof(int), 1, binFile);
		fseek(binFile, 1, SEEK_SET);
		fwrite(&novoTopo, sizeof(int), 1, binFile);

		//posiciona no local de insercao
		fseek(binFile, HEADERSIZE + topoPilha*REGSIZE, SEEK_SET);
	}
	else	//insere no final
		fseek(binFile, 0, SEEK_END);
	
	//estimando rrn no qual sera inserido o registro
	long rrn = (ftell(binFile) - HEADERSIZE)/REGSIZE;

	writeReg(binFile,codINEP, dataAtiv, uf, nomeEscola, municipio, prestadora);

	setStatus(binFile, 1);
	fclose(binFile);

	insertKeyToIndex( codINEP, rrn);	
	
	return 1;
}
/** \brief Funcao que recebe um campo e um valor, e imprime todos os registros que tiverem o valor procurado no campo passado
  *
  * \params - campo e valor procurado
  *
	* \return 0 falha ao abrir aquivo ou -1 nenhum registro encotrado ou 1 registros encontrados
	*/
int search(char  *fieldName, char *value){
	int size, RRN = 0, tamReg = 0, found = -1;
	FILE *binFile = fopen("data.dat", "rb+");

	if(!binFile) return 0;

	if(isCorruptedFile(binFile)){
		fclose(binFile);
		return 0;
	}

	setStatus(binFile, 0);

	fseek(binFile, 0, SEEK_END);
	size = ftell(binFile);
	fseek(binFile, HEADERSIZE, SEEK_SET);


	while(ftell(binFile) != size) {

		if(isValidReg(RRN, binFile)){

			// aloca as memórias para cada campo
			registro *reg = calloc(sizeof(registro), 1);
			reg->dataAtiv = calloc(10*sizeof(char) + 1, 1);
			reg->uf = calloc(2*sizeof(char) +1, 1);

			// Le os dados do arquivo binario e salva nas variaveis
			fread(&reg->codINEP, sizeof(int), 1, binFile);
			fread(reg->dataAtiv, 10 * sizeof(char), 1, binFile);
			fread(reg->uf, 2 * sizeof(char), 1, binFile);
			fread(&reg->tam_nomeEscola, sizeof(int), 1, binFile);
			//ja coloca \0 na posicao extra, permitindo comparar strings
			reg->nomeEscola = calloc(reg->tam_nomeEscola +1, 1);
			fread(reg->nomeEscola, reg->tam_nomeEscola * sizeof(char), 1, binFile);
			fread(&reg->tam_municipio, sizeof(int), 1, binFile);
			reg->municipio = calloc(reg->tam_municipio +1, 1);
			fread(reg->municipio, reg->tam_municipio * sizeof(char), 1, binFile);
			fread(&reg->tam_prestadora, sizeof(int), 1, binFile);
			reg->prestadora = calloc(reg->tam_prestadora +1, 1);
			fread(reg->prestadora, reg->tam_prestadora * sizeof(char), 1, binFile);

			if(checkToPrint(reg, fieldName, value) == 1){
				printReg(reg);
				found = 1;
			}
			freeRegister(&reg);
  		}
		RRN++;	//sendo valido ou nao, passa para poximo registro
		fseek(binFile, HEADERSIZE + RRN*REGSIZE, SEEK_SET);
	}

	setStatus(binFile, 1);
	fclose(binFile);

	return found;
}

/** \brief Funcao que verifica se o registro lido tem o campo e o valor do campo é igual que o usuario esta buscando.
  *
  * \params - registro lido, campo desejado e valor procurado
  *
  * \return 0 se nao tiver o valor procurado no campo, 1 se achar
  */
int checkToPrint(registro *reg, char *field, char *value){

	char dataAtivNull[DATAATIVSIZE +1] = "0000000000";
	char ufNull[UFSIZE+1] = "00";

	if(strcmp(field, "codINEP") == 0)
		//verificando se o campo eh igual
		return reg->codINEP == atoi(value);

	else if(strcmp(field, "dataAtiv") == 0)
		//considerando a possibilidade de serem nulos
		return !strcmp(reg->dataAtiv, value) || (!strlen(value) && !strcmp(dataAtivNull, reg->dataAtiv));

	else if(strcmp(field, "uf") == 0)
		return !strcmp(reg->uf, value) || (!strlen(value) && !strcmp(ufNull, reg->uf));

	else if(strcmp(field, "nomeEscola") == 0)
		return !strcmp(reg->nomeEscola, value) || ( !strlen(value) && !reg->tam_nomeEscola);

	else if(strcmp(field, "municipio") == 0)
		return !strcmp(reg->municipio, value) || ( !strlen(value) && !reg->tam_municipio);

	else if(strcmp(field, "prestadora") == 0)
		return !strcmp(reg->prestadora, value) || ( !strlen(value) && !reg->tam_prestadora);

	return 0;
}

/** \brief Funcao que recebe um RNN para mudar atualizar os dados desse registro.
  *
  * \params - RNN do registro para atualizar e os 6 novos valores para o campo
  *
  * \return 0 falha ao abrir aquivo ou -1 nenhum registro encotrado ou 1 registros encontrados
*/

int updateReg(int RRN, int codINEP, char* dataAtiv, char *uf, char *nomeEscola, char *municipio, char *prestadora){
	int size;
	char zero = '0';
	FILE *binFile = fopen("data.dat", "rb+");

	if(!binFile) return 0;
	if(isCorruptedFile(binFile)){
		fclose(binFile);
		return 0;
	}
	setStatus(binFile, 0);

	// busca o byte offset do registro procurado no arquivo
	fseek(binFile, HEADERSIZE + RRN * REGSIZE, SEEK_SET);

	if(!isValidReg(RRN, binFile)){//RRN removido ou grande demais
		// normaliza status e finaliza
		setStatus(binFile, 1);
		fclose(binFile);
		return -1;
	}

	writeReg(binFile,codINEP, dataAtiv, uf, nomeEscola, municipio, prestadora);

	setStatus(binFile, 1);
	fclose(binFile);
	return 1;
}


/** \brief Funcao que compacta o arquivo de dados de forma eficiente
  *
	* \return 0 falha ao abrir aquivo, 1 se compactou corretamente,
	*/
int compact(){

	int topStack;

	//abre o arquivo principal e cria um outro para fazer a compactacao
	FILE *binFile = fopen("data.dat", "rb+");
	//falha ao abrir o arquivo
	if(!binFile) return 0;

	setStatus(binFile, 0);
	fread(&topStack, sizeof(int), 1, binFile);

	if( topStack == -1){	//arquivo nao possui registros removidos
		setStatus(binFile, 1);
		fclose(binFile);
		return 1;
	}

	//topo do arquivo novo
	topStack = -1;

	FILE *newBinFile = fopen("data2.dat", "wb+");
	if(!newBinFile) return 0;

	//coloca o HEADER no novo arquivo
	setStatus(newBinFile, 0);
	fwrite(&topStack, sizeof(int), 1, newBinFile);

	int RRN = 0;// controla posicao de copia no arquivo

	//guarda em size o tamanho do arquivo
	fseek(binFile, 0, SEEK_END);
	long int size = ftell(binFile);
	fseek(binFile, HEADERSIZE, SEEK_SET);

	//armazenara registro a ser copiado
	unsigned char regInfo[REGSIZE];

	//loop que ocorre do comeco+HEADER ate o fim do arquivo
	while(ftell(binFile) <= size) {

		//verifica se o registro eh valido
		if(isValidReg(RRN, binFile)){
			fread(regInfo, REGSIZE, 1, binFile);
			fwrite(regInfo, REGSIZE, 1, newBinFile);
		}
		RRN++;	//sendo valido ou nao, passa para poximo registro
		fseek(binFile, HEADERSIZE + RRN*REGSIZE, SEEK_SET);
	}

	//remove o arquivo antigo e salva o novo com o nome do antigo
	remove("data.dat");
	rename("data2.dat", "data.dat");

	setStatus(newBinFile, 1);
	fclose(newBinFile);
	fclose(binFile);

	return 1;

}

/** \brief Funcao que retorna a pilha de registros deletados do arquivo
  *
  * \params - sizeStack = tamanho da pilha que vai sendo incrementado a cada desempilhamento
  *
	* \return NULL falha ao abrir aquivo ou o vetor que guarda os RRNs da pilha
	*/
int* showStack(int* sizeStack){

	int topStack, *stack;		//guarda o topo da pilha, vetor que guarda os RRNs dos registros deletados

	FILE *binFile = fopen("data.dat", "rb+");

	//verefica se o arquivo eh valido
	if(!binFile) return NULL;
	setStatus(binFile, '0');

	//guarda o topo da pilha
	fread(&topStack, sizeof(int), 1, binFile);
	stack = (int*)malloc(((*sizeStack) + 1) * sizeof(int));
	stack[*sizeStack] = topStack;
	(*sizeStack) = (*sizeStack) + 1;

	//verifica se a pilha está vazia
	if(topStack == -1){
		fclose(binFile);
		return stack;
	}
	else{
		while(1){
			//busca conforme o topo da pilha encontrado e guarda a continuacao da pilha nessa variavel
			fseek(binFile, HEADERSIZE + topStack * REGSIZE + sizeof(int), SEEK_SET);
			fread(&topStack, sizeof(int), 1, binFile);

			//condição de parada do loop
			if(topStack == -1){
				fclose(binFile);
				return stack;
			}

			//se nao entrou na condicao de parada, ele eh guardado no vetor de retorno
			stack = (int*)realloc(stack, (*sizeStack + 1) * sizeof(int));
			stack[*sizeStack] = topStack;
			(*sizeStack) = (*sizeStack) + 1;
		}
	}

	setStatus(binFile, '0');
	fclose(binFile);
}
