/* 
SPECIFICATION TO BE IMPLEMENTED:
Scrivere un programma che riceva in input tramite argv[] il nome di un file
F e una stringa S contenente un numero arbitrario di caratteri. 
Il main thread dell'applicazione dovra' creare il file F e poi leggere indefinitamente 
stringhe dallo standard input per poi scriverle, una per riga, all'interno del file.
Qualora venga ricevuto il segnale SIGINT (o CTRL_C_EVENT nel caso WinAPI), dovra'
essere lanciato un nuovo thread che riporti il contenuto del file F all'interno di un 
altro file con lo stesso nome, ma con suffisso "_shadow", sostituendo tutte le stringhe
che coincidono con la stringha S ricevuta dall'applicazione tramite argv[] con 
una stringha della stessa lunghezza costituita da una sequenza di caratteri '*'.
Il lavoro di questo thread dovra' essere incrementale, ovvero esso dovra' riportare 
sul file shadow solo le parti del file originale che non erano state riportate in 
prcedenza. Al termine di questa operazione, il thread dovra' indicare su standard 
output il numero di stringhe che sono state sostituite in tutto.
*/


#include<stdio.h> 
#include<errno.h> 
#include<string.h> 
#include<stdlib.h> 
#include<semaphore.h> 
#include<pthread.h> 
#include<unistd.h> 
#include<fcntl.h>
#include<signal.h>  
#include<sys/ipc.h> 
#include<sys/mman.h> 
#include<sys/types.h> 
#include<sys/sem.h> 

char *fname;
char *sname; 
char *string; 
int scritture, scritte;
FILE *sfile;
// FILE *file;


void *thread_fun(){

	int ret; 
	char *out;
	FILE *file;

	sfile = fopen(sname, "a");
	if(sfile == NULL){
		printf("Errore file shadow\n");
		exit(EXIT_FAILURE);
	}

	file = fopen(fname, "r");
	if(file == NULL){
		printf("Errore apertura file thread\n");
		exit(EXIT_FAILURE);
	}

	for(int i=0; i<scritte; i++){
		ret = fscanf(file, "%ms", &out);
	}

	while(1){
		ret = fscanf(file, "%ms", &out);
		printf("string: %s\n", out);
		if(ret == EOF){break; }
		if(strcmp(out, string) == 0){
			strcpy(out, "*");
			for(int i=1; i<strlen(string); i++){
				strcat(out,"*");
			}
		}

		fprintf(sfile,"%s\n", out);
		fflush(sfile);
		scritte++;
	}

}




void genera(){

	pthread_t id; 
	void *status;

	if(pthread_create(&id, NULL, thread_fun, NULL)){
		printf("Errore creazione thread\n");
		exit(EXIT_FAILURE);
	}

	if(pthread_join(id, &status)){
		printf("Errore attesa fine therad genera\n");
		exit(EXIT_FAILURE);
	}

}



int main(int argc, char **argv){
 
	char *input;
	FILE *file;
	int ret;
	struct sigaction sa; 
	scritture = 0; 
	scritte = 0;

	if(argc != 3){
		printf("Errore numero argomenti file-stringa\n");
		exit(EXIT_FAILURE);
	}	

	fname  = argv[1];
	string = argv[2]; 

	sname = malloc(strlen(fname) + strlen("_shadow") + 2);
	sprintf(sname, "%s_shadow", fname);

	file = fopen(fname, "w+");
	if(file == NULL){
		printf("Errore apertura file main\n");
		exit(EXIT_FAILURE);
	}

	sa.sa_handler = genera; 
	sigfillset(&sa.sa_mask);
	sa.sa_flags = 0; 
	if(sigaction(SIGINT, &sa, NULL)){
		printf("Errore sigacrion main\n");
		exit(EXIT_FAILURE);
	}

	while(1){

		scanf("%ms", &input);
print:
		ret = fprintf(file, "%s\n", input);
		fflush(file);
		scritture ++;
		if(ret < 0 && errno == EINTR){goto print;}

	}

	return 0;
} 
