/*
SPECIFICATION TO BE IMPLEMENTED:
Implementare una programma che riceva in input, tramite argv[], il nome
di un file F e un insieme di N stringhe (con N almeno pari ad 1). Il programa dovra' creare 
il file F e popolare il file con le stringhe provenienti da standard-input. 
Ogni stringa dovra' essere inserita su una differente linea del file.
L'applicazione dovra' gestire il segnale SIGINT (o CTRL_C_EVENT nel caso
WinAPI) in modo tale che quando il processo venga colpito si
dovranno  generare N processi concorrenti ciascuno dei quali dovra' analizzare il contenuto
del file F e verificare, per una delle N stringhe di input, quante volte la inversa di tale stringa 
sia presente nel file. Il risultato del controllo dovra' essere comunicato su standard
output tramite un messaggio. Quando tutti i processi avranno completato questo controllo, 
il contenuto del file F dovra' essere inserito in "append" in un file denominato "backup"
e poi il file F dova' essere troncato.
*/

// Programma non risolto completamente

#include<stdio.h> 
#include<stdlib.h> 
#include<unistd.h>
#include<errno.h> 
#include<string.h> 
#include<fcntl.h> 
#include<signal.h>
#include<semaphore.h>
#include<pthread.h> 
#include<sys/mman.h> 
#include<sys/sem.h> 
#include<sys/types.h> 
#include<sys/ipc.h> 

char *fname; 
char **strings;
char *rstring;
int num_p;
int turn;

sem_t *ready; 
sem_t *done;


void reverse(){
	rstring = malloc((sizeof(strings[turn])) + 1);

	for(int i=0; i<strlen(strings[turn]); i++){
		rstring[i] = strings[turn][strlen(strings[turn])-i-1];
	}
	rstring[strlen(strings[turn])] = '\0';
}

void run(){

	int cont = 0; 
	char *s;
	int ret;
	FILE *f;


	f = fopen(fname, "r");
	if(f == NULL){
		printf("Errore apertura file\n");
		exit(EXIT_FAILURE);
	}

	while(1){
	
		ret = fscanf(f, "%ms", &s); 
		if (ret == EOF){
			free(s);
			break ;}
		if(strcmp(s, rstring) == 0){ cont ++;}
		free(s);

	}

	printf("\nProcess: %d 	reverse: %s		cont: %d\n", getpid(),rstring,cont);
	fflush(stdout);

	if(sem_post(ready + turn)){
		printf("Errore post ready + turn\n");
		exit(EXIT_FAILURE);
	}
}



void gen_p(){

	int pid;
	char *s;
	int ret;
	FILE *back;
	FILE *f;


	for(int i=0; i<num_p; i++){
		turn = i; 
		reverse();
		pid = fork(); 
		if(pid == 0){run(); }
		if(pid < 0){
			printf("Errore nella fork\n");
			exit(EXIT_FAILURE);
		}
	}

	for(int i=0; i<num_p; i++){
		if(sem_wait(ready + i)){
			printf("Errore nella wait ready + i");
			exit(EXIT_FAILURE);
		}
	}

	back = fopen("append","a");
	if(back == NULL){
		printf("Errore apertura append\n");
		exit(EXIT_FAILURE);
	}

	f = fopen(fname,"r");
	if(f == NULL){
		printf("Errore apertura file\n");
		exit(EXIT_FAILURE);
	}

	while(1){
		ret = fscanf(f, "%ms", &s);

		if(ret == EOF){
			free(s);
			break; 
		}

		fprintf(back, "%s\n", s);
		fflush(back);
		free(s);
	}

}



int main(int argc, char **argv){

	FILE *f; 
	char *input;
	int ret;
	struct sigaction sa;

	if(argc < 2){
		printf("Errore nel numero di argomenti\n");
		exit(EXIT_FAILURE);
	}

	fname  = argv[1];
	strings = argv + 2;
	num_p = argc -2;

	f = fopen(fname, "w+");

	if(f == NULL){ 
		printf("Errore nell'apertura del file\n");
		exit(EXIT_FAILURE);	
	}

	ready = malloc(sizeof(sem_t)*num_p);
	done = malloc(sizeof(sem_t));

	if(ready == NULL || done == NULL){
		printf("Errore nella malloc semafori\n");
		exit(EXIT_FAILURE);
	}

	if(sem_init(done, 1, 1)){
		printf("Errore init done\n"); 
		exit(EXIT_FAILURE);
	}

	for(int i=0; i<num_p; i++){
		if(sem_init(ready + i, 1, 0)){
			printf("Errore nell'init del semaforo");
			exit(EXIT_FAILURE);
		}
	}

	sa.sa_handler = gen_p;
	sigfillset(&sa.sa_mask);
	sa.sa_flags = 0;

	if(sigaction(SIGINT, &sa, NULL)){
		printf("Errore nella sigaction\n");
		exit(EXIT_FAILURE); 
	}

	while(1){


lock1:
		if(sem_wait(done)){
			if(errno == EINTR){goto lock1; }
			printf("Errore lock done + i\n");
			exit(EXIT_FAILURE);
		}


input:
		ret = scanf("%ms", &input);
		if(ret == EOF && errno == EINTR){goto input;}


rewrite:
		ret = fprintf(f,"%s\n",input);
		fflush(f);
		if(ret < 0 && errno == EINTR){goto rewrite; }


unlock1:
		if(sem_post(done)){
			if(errno == EINTR){goto unlock1; }
			printf("Errore nell'unlock done\n");
			exit(EXIT_FAILURE);
		}

	}


	return 0;
}