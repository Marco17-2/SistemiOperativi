/*
SPECIFICATION TO BE IMPLEMENTED:
Implementare un programma che riceva in input tramite argv[] i pathname 
associati ad N file, con N maggiore o uguale ad 1. Per ognuno di questi
file generi un thread (quindi in totale saranno generati N nuovi thread 
concorrenti). 
Successivamente il main-thread acquisira' stringhe da standard input in 
un ciclo indefinito, ed ognuno degli N thread figli dovra' scrivere ogni
stringa acquisita dal main-thread nel file ad esso associato.
L'applicazione dovra' gestire il segnale SIGINT (o CTRL_C_EVENT nel caso 
WinAPI) in modo tale che quando uno qualsiasi dei thread dell'applicazione
venga colpito da esso dovra' stampare a terminale tutte le stringhe gia' 
immesse da standard-input e memorizzate nei file destinazione.
*/

#include<stdio.h>
#include<stdlib.h> 
#include<string.h>
#include<unistd.h> 
#include<pthread.h> 
#include<fcntl.h>
#include<signal.h> 
#include<semaphore.h> 
#include<errno.h>
#include<sys/sem.h> 
#include<sys/types.h> 
#include<sys/ipc.h> 
#include<sys/mman.h> 

pthread_mutex_t *ready; 
pthread_mutex_t *done; 
char **fnames; 
char *input;
int num_thread;
 // __thread char* name;
char *output;


void printer(){
	FILE *t;
	char *s;
	char *n;

//	sprintf(n, "%s", name);
	t = fopen(output, "r");
	int ret;

	if(t == NULL){
		printf("Errore apertura fileeeeee");
		exit(EXIT_FAILURE);
	}

	printf("\nFILE: %s\n", output);
	fflush(stdout);

	while(1){
		ret = fscanf(t, "%ms", &s);
		if(ret == EOF){break; }
		printf("%s\n",s);
		fflush(stdout);
	}


}

void *fun_thread(void *arg){

	long int me = (long int)arg;
	int ret;
	FILE *f;
	struct sigaction set;

	//name = fnames[me];
	f = fopen(fnames[me], "w+");

	if(f == NULL){
		printf("Errore apertura file %ld\n", me+1);
		exit(EXIT_FAILURE);
	}

	while(1){

lock2:
		if(pthread_mutex_lock(ready + me)){
			if(errno == EINTR){goto lock2; }
			printf("Errore lock lock2\n");
			exit(EXIT_FAILURE);
		}
		

write:
		ret = fprintf(f, "%s\n", input);
		if(ret < 0 && errno == EINTR){goto write; }
		fflush(f);

		printf("Thread: %ld 		string: %s\n",me,input);
		output = fnames[me];
		fflush(stdout);
		sleep(3);

		if(me < num_thread - 1){
unlock3:
			if(pthread_mutex_unlock(ready + me + 1)){
				if(errno == EINTR){goto unlock3; }
				printf("Errore unlco ready + me +1\n");
				exit(EXIT_FAILURE); 
			}
		}

unlock2:

		if(pthread_mutex_unlock(done + me)){
			if(errno == EINTR){goto unlock2; }
			printf("Errore nell'unlock2\n");
			exit(EXIT_FAILURE);
		}
	}
}


int main(int argc, char **argv){

	pthread_t id; 
	int ret;
	struct sigaction set; 

	set.sa_handler = printer; 
	sigfillset(&set.sa_mask);
	set.sa_flags = 0; 

	sigaction(SIGINT, &set, NULL);

	if(argc < 2){
		printf("Errore nel numero di argomenti\n");
		exit(EXIT_FAILURE);
	}

	fnames = argv + 1;
	num_thread = argc - 1;

	ready = malloc(sizeof(pthread_mutex_t)*num_thread);
	done = malloc(sizeof(pthread_mutex_t)*num_thread);

	if(read == NULL || done == NULL){
		printf("Errore malloc mutex\n");
		exit(EXIT_FAILURE);
	}

	for(int i=0; i<num_thread; i++){
		if(pthread_mutex_init(ready + i, NULL) || pthread_mutex_init(done + i, NULL)){
			printf("Errore nell'init dei mutex\n");
			exit(EXIT_FAILURE);
		}
	}

	for(int i=0; i<num_thread; i++){
		if(pthread_mutex_lock(ready + i)){
			printf("Errore lock ready + i");
		}
	}

	for(int long i=0; i<num_thread; i++){
		if(pthread_create(&id, NULL, fun_thread, (void*)i)){
			printf("Errore creazione thread\n");
			exit(EXIT_FAILURE);
		}	
	}

	while(1){

lock1: 
		for(int i=0; i<num_thread; i++){
			if(pthread_mutex_lock(done + i)){
				if(errno == EINTR){goto lock1; }
				printf("Errore nel lock1\n");
				exit(EXIT_FAILURE);
			}
		}

		//if(input != NULL){free(input);}

input: 
		ret = scanf("%ms", &input);
		if(ret == EOF || errno == EINTR){ goto input; }


unlock1:
		//for(int i=0; i<num_thread; i++){
			if(pthread_mutex_unlock(ready /*+ i*/)){
				if(errno == EINTR){goto unlock1; }
				printf("Errore nell'unlock1\n");
				exit(EXIT_FAILURE);
			}
		//}
	}

	return 0;
}