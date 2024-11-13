/* 
SPECIFICATION TO BE IMPLEMENTED:
Implementare una programma che riceva in input, tramite argv[], il nome
di un file F e un insieme di N stringhe (con N almeno pari ad 1). Il programa dovra' creare 
il file F e popolare il file con le stringhe provenienti da standard-input. 
Ogni stringa dovra' essere inserita su una differente linea del file.
L'applicazione dovra' gestire il segnale SIGINT (o CTRL_C_EVENT nel caso
WinAPI) in modo tale che quando il processo venga colpito si
dovranno  generare N thread concorrenti ciascuno dei quali dovra' analizzare il contenuto
del file F e verificare, per una delle N stringhe di input, quante volte tale stringa 
sia presente nel file. Il risultato del controllo dovra' essere comunicato su standard
output tramite un messaggio. Quando tutti i thread avranno completato questo controllo, 
il contenuto del file F dovra' essere inserito in "append" in un file denominato "backup"
e poi il file F dova' essere troncato.

Qualora non vi sia immissione di input o di segnali, l'applicazione dovra' utilizzare 
non piu' del 5% della capacita' di lavoro della CPU.
*/

#include<stdio.h> 
#include<stdlib.h> 
#include<string.h> 
#include<errno.h> 
#include<signal.h> 
#include<pthread.h> 
#include<fcntl.h>
#include<semaphore.h> 
#include<unistd.h> 
#include<sys/types.h> 
#include<sys/ipc.h> 
#include<sys/mman.h> 
#include<sys/mman.h> 

pthread_mutex_t *done; 
pthread_mutex_t *ready;
int num_thread;
char **strings; 
char *namef;
FILE *backup;



void *fun_thread(void *arg){

	long int me = (long int)arg;
	int cont = 0;
	FILE *f;
	char *p;
	int ret;

	f = fopen(namef, "r");

	if(f == NULL){
	 	printf("Errore apertura file\n");
	 	exit(EXIT_FAILURE);
	 }

	while(1){
		
		ret = fscanf(f,"%ms",&p); 
		if(ret == EOF){ break; }
	 	if(strcmp(p, strings[me+2]) == 0){
	 		cont ++; 
	 	}
	}

	printf("\nThread:%ld   string:%s   numero:%d\n",me,strings[me+2],cont);
	fflush(stdout);

	if(pthread_mutex_unlock(ready + me)){
		printf("Errore unlock rady + me\n");
		exit(EXIT_FAILURE);
	}


}



void gen_thread(){

	pthread_t id;
	char *in;
	int ret;
	FILE *t;

	if(pthread_mutex_lock(done)){
		printf("Errore lock done\n");
		exit(EXIT_FAILURE);
	}

	for(long int i=0; i<num_thread; i++){
		if(pthread_create(&id, NULL, fun_thread, (void *)i)){
			printf("Errore nella creazione del thread\n"); 
			exit(EXIT_FAILURE);
		}
	}


/*
	if(pthread_mutex_unlock(ready)){
		printf("Errore nell'unlock ready\n");
		exit(EXIT_FAILURE);
	}
*/

	t = fopen(namef, "r");

	if(t == NULL){
		printf("Errore apertura file\n");
		exit(EXIT_FAILURE);
	}

	backup = fopen("backup","a");

	if(backup == NULL){
		printf("Errore apertura file backup\n");
		exit(EXIT_FAILURE);
	}

	for(int i=0; i<num_thread; i++){

		if(pthread_mutex_lock(ready + i)){
			printf("Errore nel lock ready + i\n");
			exit(EXIT_FAILURE);
		}
	}

	while(1){

		ret = fscanf(t, "%ms", &in);
		if(ret == EOF) {break; }
		fprintf(backup, "%s\n", in);
		fflush(backup);
	}

	if(pthread_mutex_unlock(done)){
		printf("Errore unlock done\n");
		exit(EXIT_FAILURE);
	}
}



int main(int argc, char **argv){

	FILE *f;
	char *input; 
	int ret; 
	struct sigaction sa; 

	if(argc < 3){ 
		printf("Errore nel numero di argomenti\n");
		exit(EXIT_FAILURE);
	}

	sa.sa_handler = gen_thread; 
	sigfillset(&sa.sa_mask);
	sa.sa_flags = 0;

	sigaction(SIGINT, &sa, NULL);

	strings = argv;
	num_thread = argc - 2;
	namef = argv[1];

	f = fopen(argv[1],"w+");

	if(f == NULL){
		printf("Errore creazione del file\n");
		exit(EXIT_FAILURE);
	}

	done = malloc(sizeof(pthread_mutex_t)); 
	ready = malloc(sizeof(pthread_mutex_t)*num_thread);

	if(done == NULL || ready == NULL){
		printf("Errore nella malloc dei semaforo\n");
		exit(EXIT_FAILURE);
	}

	if(pthread_mutex_init(done, NULL)){
		printf("Errore init mutex");
		exit(EXIT_FAILURE);
	}

	if(pthread_mutex_init(ready, NULL)){
		printf("Errore init mutex");
		exit(EXIT_FAILURE);
	}

	for(int i=0; i<num_thread; i++){
		if(pthread_mutex_lock(ready + i)){
			printf("Errore nel lock del thread ready + i");
			exit(EXIT_FAILURE);
		}
	}

	while(1){

input: 
		ret = scanf("%ms", &input);
		if(errno == EINTR && ret == EOF){ goto input; }

lock1:
		if(pthread_mutex_lock(done)){
			if(errno == EINTR){goto lock1; }
			printf("Errore nel lock\n"); 
			exit(EXIT_FAILURE);
		}

		fprintf(f,"%s\n",input);
		fflush(f);

unlock1:

		if(pthread_mutex_unlock(done)){
			if(errno == EINTR){ goto unlock1; }
			printf("Errore nell'unlock\n");
			exit(EXIT_FAILURE);
		}

	}

	return 0;
}