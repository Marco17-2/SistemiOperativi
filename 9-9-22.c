/*
SPECIFICATION TO BE IMPLEMENTED:
Implementare una programma che riceva in input, tramite argv[], i nomi
di N differenti file F1 ed FN, con N maggiore o uguale a 1, che dovranno essere creati.
Per ognuno dei file dovra' essere attivato un nuovo thread che ne gestira' il contenuto 
(indichiamo quindi con T1 ... TN i thread che dovranno essere attivati).
A turno secondo una regola circolare ogni thread Ti dovra' leggere 5 caratteri dallo stream 
di standard input e dovra' scriverli sul file che sta gestendo.
La scrittura dei 5 caratteri su ciascuno dei file deve risultare come una azione atomica,
ovvero i caratteri non possono essere scritti sui file individualmente.

L'applicazione dovra' gestire il segnale SIGINT (o CTRL_C_EVENT nel caso
WinAPI) in modo tale che quando il processo venga colpito esso dovra' 
riportare su standard output i 5 ultimi caratteri correntemente presenti 
su ciascuno degli N file gestiti.

In caso non vi sia immissione di dati sullo standard input, e non vi siano segnalazioni,
l'applicazione dovra' utilizzare non piu' del 5% della capacita' di lavoro della CPU.
*/

// DA SISTEMARE  LSEEK DALLA FINE SISTEMARE SCRITTURA 5 CARATTERI ATOMICI
// FINIRE IMPLEMENTANDO LETTURA ULTIMI 5 CARATTERI

#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <semaphore.h>
#include <pthread.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#define SIZE 5

char **fnames;
int thread_num;
pthread_mutex_t *ready; 
pthread_mutex_t *done; 


void print(){	

	FILE **f; 
	char output[SIZE + 1];
	char check[SIZE + 1]; 
	int ret;

	f = (FILE**)malloc(sizeof(FILE*)*thread_num);
	if(f == NULL){
		printf("Errore malloc\n"); 
		exit(EXIT_FAILURE);
	}

	printf("\n");

	for(int i=0; i<thread_num; i++){

		f[i] = fopen(fnames[i], "r" );

		if(f[i] == NULL){
			printf("Errore f[i]\n"); 
			exit(EXIT_FAILURE);
		}
	}

	for(int i=0; i<thread_num; i++){

		while(1){	

			ret = fscanf(f[i], "%s", check);
			if(ret == EOF || ret == 0){ break; }
			strcpy(output, check); 
		}

		printf("FILE: %d 		stringa: %s\n",i,output);

	}
}	




void *thread_fun(void *arg){
	
	long int me = (long int)arg; 
	char buff[SIZE];
	int f; 
	int ret; 

	f = open(fnames[me], O_CREAT | O_WRONLY, 0660);
	if(f == -1 ){
		printf("Errore aprtura file del thread\n");
		exit(EXIT_FAILURE);

	}

	while(1){

		if(pthread_mutex_lock(ready + me)){
			printf("Errore lock mutex\n");
			exit(EXIT_FAILURE);	
		}

		printf("Thread: %ld\n", me);
		for(int i=0; i<5; i++){
			scanf("%c", &buff[i]);
			while(getchar()!='\n');
		}

		ret = write(f, buff, SIZE);
		if(ret == -1){ 
			printf("Errore scrittura file\n");
			exit(EXIT_FAILURE);
		}

		ret = write(f, "\n", 1);
		if(ret == -1){ 
			printf("Errore scrittura file\n");
			exit(EXIT_FAILURE);
		}

		if(pthread_mutex_unlock(done + me)){
			printf("Errore unlock done + me\n");
			exit(EXIT_FAILURE);
		}

		if(me < thread_num - 1){
			if(pthread_mutex_unlock(ready + me + 1)){
				printf("Errore unlock ready + me + 1\n");
				exit(EXIT_FAILURE);
			}
		}
	}

}




int main(int argc, char** argv){

	pthread_t id;
	FILE **files; 
	struct sigaction sa; 

	if(argc < 2){
		printf("Errore nel numero di argomenti\n");
		exit(EXIT_FAILURE);
	}

	thread_num = argc - 1;
	fnames = argv + 1;

	ready = malloc(sizeof(pthread_mutex_t)*thread_num); 
	done = malloc(sizeof(pthread_mutex_t)*thread_num);
	files = (FILE**)malloc(sizeof(FILE*)*thread_num); 

	if(ready == NULL || done == NULL){
		printf("Errore malloc semafori\n");
		exit(EXIT_FAILURE);
	}

	if(files == NULL){
		printf("Errore malloc files\n");
		exit(EXIT_FAILURE);
	}

	for(int i=0; i<thread_num; i++){
		if(pthread_mutex_lock(ready + i)){
			printf("Errore lock ready + i\n");
			exit(EXIT_FAILURE);
		}
	}

	for(long int i=0; i<thread_num; i++){
		if(pthread_create(&id, NULL, thread_fun, (void*)i)){
			printf("Errore creazione thread\n");
			exit(EXIT_FAILURE);
		}
	}

	for(int i=0; i<thread_num; i++){
		files[i] = fopen(fnames[i], "w+");
		if(files[i] == NULL){
			printf("Errore apertura file %d\n", i);
			exit(EXIT_FAILURE);
		}
	}

	sa.sa_handler = print; 
	sigfillset(&sa.sa_mask);
	sa.sa_flags = 0; 

	if(sigaction(SIGINT, &sa , NULL)){
		printf("Errore sigaction\n"); 
		exit(EXIT_FAILURE);
	}

	while(1){

		for(int i=0; i<thread_num; i++){
lock1:
			if(pthread_mutex_lock(done + i)){
				if(errno == EINTR){goto lock1; }
				printf("Errore lock1\n");
				exit(EXIT_FAILURE);
			}
		}


unlock1: 
		if(pthread_mutex_unlock(ready)){
			if(errno == EINTR){goto unlock1; }
			printf("Errore unlcok1\n");
			exit(EXIT_FAILURE);
		}

	}

	return 0;
}