/*
SPECIFICATION TO BE IMPLEMENTED:
Scrivere un programma che riceva in input tramite argv[] i nomi di N file,
con N magiore o uguale a 1. L'applicazione dovrà controllare che i nomi dei 
file siano diversi tra di loro.
I file dovranno essere creati oppure troncati se esistenti.
Per ogniuno dei file dovra' essere attivato un nuovo thread, che indicheremo
con Ti, che gestirà il contenuto del file. 
I thread Ti leggeranno linee di caratteri da standard input a turno secondo 
uno schema circolare, e scriveranno la linea letta all'interno del file 
da loro gestito.

L'applicazione dovrà essere in grado di gestire il segnale SIGINT 
(o CTRL_C_EVENT nel caso WinAPI) in modo tale che quando il processo  
verrà colpito riporti su standard output il contenuto corrente (ovvero le linee 
attualmente presenti) di tutti i file che erano stati specificati in argv[], 
seguendo esattamente l'ordine tramite cui le linee sono state inserite al loro interno.
In ogni caso, nessuno dei thread dovrà terminare la sua esecuzione in caso di arrivo
della segnalazione.
*/

#include <stdio.h> 
#include <stdlib.h>
#include <string.h> 
#include <errno.h> 
#include <pthread.h> 
#include <signal.h> 
#include <unistd.h>
#include <semaphore.h> 
#include <fcntl.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/ipc.h>
#include <sys/sem.h>

char **files; 
int num_thread; 
FILE **buffer;
pthread_mutex_t *ready; 
pthread_mutex_t *done; 



void printer(){

	buffer = (FILE **)malloc(sizeof(FILE*)*num_thread); 
	int ret; 
	int turn = 0; 
	char *string; 


	for(int i=0; i<num_thread; i++){
		buffer[i] = fopen(files[i], "r"); 
		if(buffer[i] == NULL){
			printf("Errore apertura file %d\n",i);
			exit(EXIT_FAILURE); 
		}
	}

	while(1){

			ret = fscanf(buffer[turn], "%ms", &string);
			if(ret == EOF){break; }
			printf("%s\n", string); 
			fflush(stdout); 

			turn = (turn + 1)%num_thread; 
	}

		free(buffer);
}




int controllo(){
	for(int i=0; i<num_thread; i++){
		for(int j=0; j<num_thread; j++){
			if(i != j){
				if(strcmp(files[i], files[j]) == 0){
					return 1;
				}
			}
		}
	}

	return 0;
}


void *fun_thread(void *arg){

	long int me = (long int)arg;
	FILE *my_file;
	char * string; 
	int ret; 
	/*struct sigaction sa; 

	sa.sa_handler = printer; 
	sigfillset(&sa.sa_mask);
	sa.sa_flags = 0; 

	if(sigaction(SIGINT, &sa, NULL)){
		printf("Errore sigaction\n"); 
		exit(EXIT_FAILURE);
	}*/

	my_file = fopen(files[me], "w+");
	if(my_file == NULL){
		printf("Errore apertura thread file\n");
		exit(EXIT_FAILURE);
	}

	while(1){

lock3: 
		if(pthread_mutex_lock(ready + me)){
			if(errno == EINTR){goto lock3;}
			printf("Errore lock3\n");
			exit(EXIT_FAILURE);
		}


		printf("Input per thread: %ld\n", me);

input:
		ret = scanf("%ms", &string); 
		if(ret == EOF && errno == EINTR){goto input; } 



write: 
		fprintf(my_file, "%s\n", string);
		fflush(my_file);
		if(errno == EINTR){goto write; }


		if(me < num_thread - 1){
unlock2:
			if(pthread_mutex_unlock(ready + me + 1)){
				if(errno == EINTR){goto unlock2; }
				printf("Errore unlock2\n"); 
				exit(EXIT_FAILURE);
			}
		}


unlock3: 
		
		if(pthread_mutex_unlock(done + me)){
			if(errno == EINTR){goto unlock3; }
			printf("Errore unlock3\n");
			exit(EXIT_FAILURE);
		}
	}
}


int main(int argc, char **argv){

	pthread_t id; 
	int ret;
	struct sigaction sa; 


	if(argc < 2){

		printf("Errore nel numero di argometni\n");
		exit(EXIT_FAILURE);
	}

	files = argv + 1; 
	num_thread = argc - 1; 

	if (controllo()){
		printf("Nomi non diversi\n"); 
		exit(EXIT_FAILURE);
	}

	ready = malloc(sizeof(pthread_mutex_t)*num_thread);
	done = malloc(sizeof(pthread_mutex_t)*num_thread); 

	if(ready == NULL || done == NULL){
		printf("Errore malloc mutex\n");
		exit(EXIT_FAILURE);
	}

	for (int i = 0; i<num_thread; i++){
		if(pthread_mutex_init(ready + i, NULL) || pthread_mutex_init(done + i, NULL) || pthread_mutex_lock(ready + i)){
			printf("Errore init mutex\n");
			exit(EXIT_FAILURE);
		}
	}

	for(long int i = 0; i<num_thread; i++){
		if(pthread_create(&id, NULL, fun_thread, (void*)i)){
			printf("Errore pthread create\n");
			exit(EXIT_FAILURE);
		}
	}


	sa.sa_handler = printer; 
	sigfillset(&sa.sa_mask);
	sa.sa_flags = 0; 

	if(sigaction(SIGINT, &sa, NULL)){
		printf("Errore sigaction\n"); 
		exit(EXIT_FAILURE);
	}

	while(1){ 

		for(int i=0; i<num_thread; i++){
lock2:
				if(pthread_mutex_lock(done + i)){
					if(errno == EINTR){goto lock2; }
					printf("Errore nel lock done +i\n");
					exit(EXIT_FAILURE);
				}
		}

unlock1: 
		if(pthread_mutex_unlock(ready)){
			if(errno == EINTR){goto unlock1; }
		} 


	}


	return 0;
}