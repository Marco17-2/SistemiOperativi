/*
SPECIFICATION TO BE IMPLEMENTED:
Scrivere un programma che riceva in input tramite argv[1] e argv[2] seguenti 
due parametri:
- in argv[1] la taglia del blocco B
- in argv[2] il numero di thread N.
L'applicazione dovra' quindi generare N tread, che indicheremo con T1 ... TN.
Ciascuno di questi thread dovra', secondo una turnazione circolare, acquisire 
un blocco di B bytes dallo standard input e dovra' scriverli in un file il cui 
nome dovra' essere "output_<THREAD_ID>".
Questo file dovra' essere creato dallo stesso thread in carico di gestirlo, in 
particolare al suo stesso startup.
Il main thread, dopo aver creato gli N thread che effettueranno le operazioni sopra
indicate, rimarra' in pausa.

L'applicazione dovra' gestire il segnale  SIGINT (o CTRL_C_EVENT nel caso WinAPI) 
in modo tale che quando il processo venga colpito il suo main thread dovra' 
riportare su standard output il contenuto dei file aggiornati dagli N thread
in modo che sia ricostruita esattamente la stessa sequenza di bytes originariamente 
acquisita tramite standard input.

In caso non vi sia immissione di dati sullo standard input e non vi siano segnalazioni, 
l'applicazione dovra' utilizzare non piu' del 5% della capacita' di lavoro della CPU.
*/

//problema nello stampare l'ultima scrittura e done non utilizzato

#include<stdio.h> 
#include<string.h> 
#include<errno.h> 
#include<stdlib.h> 
#include<unistd.h> 
#include<fcntl.h> 
#include<pthread.h> 
#include<signal.h> 
#include<semaphore.h> 
#include<sys/types.h> 
#include<sys/ipc.h> 
#include<sys/mman.h> 
#include<sys/sem.h> 

int taglia; 
int num_thread; 
int turn; 
int ret; 
char **names; 
pthread_mutex_t *ready; 
pthread_mutex_t *done; 


void print(){

	FILE **files; 
	int time = 0; 
	char *out;  

	files = (FILE**)malloc(sizeof(FILE*)*num_thread); 
	if(files == NULL){
		printf("Errore files\n");
		exit(EXIT_FAILURE);
	}	

	for(int i=0; i<num_thread; i++){
		files[i]=fopen(names[i],"r+");
		if(files[i] == NULL){
			printf("Errore apertura file\n");
			exit(EXIT_FAILURE);
		}
	}

	while(1){

		ret = fscanf(files[time], "%ms", &out); 
		fflush(files[time]); 
		if(ret == EOF){break; }
		printf("\nFile[%d]: %s",time,out); 
		time = (time+1)%num_thread; 
	}

}



void *fun_thread(void *cur){

	long int me = (long int)cur; 
	FILE *my_file; 
	char input[taglia]; 
	char *name;

	name = malloc(strlen("output_2")+3);
	sprintf(name, "output_%d", (int)me);
	names[me] = name; 

	my_file = fopen(name, "w+"); 
	if(my_file == NULL){
		printf("Errore apertura my file %ld", me);
		exit(EXIT_FAILURE);
	}

	if(me == 0){
		if(pthread_mutex_unlock(ready)){
			printf("Errore unlock funt\n");
			exit(EXIT_FAILURE);
		}
	}

	while(1){

		if(pthread_mutex_lock(ready + me)){
			printf("Errore lock fun_thread\n");
			exit(EXIT_FAILURE);
		}

		printf("Thread: %ld\n", me);
		do{
			printf("\ninput %ld>> ",me);
			ret = scanf("%s", input); 
		}while(strlen(input) < taglia);

		fprintf(my_file, "%s\n", input);
		fflush(my_file); 

		turn = (turn + 1)%num_thread; 

		if(pthread_mutex_unlock(ready + turn)){
			printf("Errore unlock mutex ready + turn"); 
			exit(EXIT_FAILURE);
		}

	}

}




int main(int argc, char **argv){

	if(argc != 3 && sizeof(argv[1]) != sizeof(int) && sizeof(argv[2]) != sizeof(int)){
		printf("Errore numero argomenti\n");
		exit(EXIT_FAILURE);
	}

	pthread_t id; 
	char *endptr;
	struct sigaction sa; 
	turn = 0;

	taglia = strtoul(argv[1], &endptr, 10);

	if(endptr == argv[1]){
		printf("Errore convesione\n");
		exit(EXIT_FAILURE);
	}

	num_thread = strtoul(argv[2], &endptr, 10);
	if(endptr == argv[2]){
		printf("Errore convesione\n");
		exit(EXIT_FAILURE);
	}

	sa.sa_handler = print; 
	sigfillset(&sa.sa_mask); 
	sa.sa_flags = 0; 

	if(sigaction(SIGINT, &sa, NULL)){
		printf("Errore sigaction\n"); 
		exit(EXIT_FAILURE);
	}

	ready = malloc(sizeof(pthread_mutex_t)*num_thread); 
	done = malloc(sizeof(pthread_mutex_t)*num_thread); 
	names = (char**)malloc(sizeof(char*)*num_thread); 


	if(ready == NULL || done == NULL || names == NULL){
		printf("Errore nella malloc ready e done\n");
		exit(EXIT_FAILURE);
	}

	if(pthread_mutex_init(ready, NULL) || pthread_mutex_init(done, NULL)){
		printf("Errore init mutex ready e done \n");
		exit(EXIT_FAILURE);
	}

	for(int j=0; j<num_thread; j++){
		if(pthread_mutex_lock(ready + j)){
			printf("Errore lock + j\n");
			exit(EXIT_FAILURE);
		}
	}


	for(long int i=0; i<num_thread; i++){
		ret = pthread_create(&id, NULL, fun_thread, (void *)i); 
		if(ret){
			printf("Errore creazione thread\n");
			exit(EXIT_FAILURE);
		}
	}

	while(1){
		sleep(0);
	}

	return 0; 
}

