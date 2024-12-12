/*
SPECIFICATION TO BE IMPLEMENTED:
Scrivere un programma che riceva in input tramite argv[] un numero di parametri
superiore a 1 (incluso il nome del programma). Ogni parametro registrato a partire
da argv[1] deve corrispondere ad una stringa di un unico carattere, ed ognuna di 
queste stringhe dovra' essere diversa dalle altre. 
Per ognuna di queste stringhe l'applicazione dovra' attivare un thread che ne 
effettuera' la gestione.
Dopo aver attivato tali thread, l'applicazione dovra' leggere ogni stringa proveniente 
da standard input e renderla disponibile a tutti i thread attivati in precedenza. 
Ciascuno di essi dovra' controllare se qualche carattre presente nella stringa proveniente
da standard input corrisponde al carattere della stringa che lui stesso sta gestendo, 
ed in tal caso dovra' sostituire quel carattere nella stringa proveniente dallo standard input
sovrascrivendolo col carattere '*'. 
Al termine delle attivita' di controllo e sostituzione di caratteri da parte 
di tutti i thread, l'applicazione dovra' scrivere su un file dal nome "output.txt"
la stringa originale proveniente da standard input e quella ottenuta tramite le 
sostituzioni di carattere (anche se non realmente avvenute), su due linee consecutive del file.

L'applicazione dovra' gestire il segnale  SIGINT (o CTRL_C_EVENT nel caso WinAPI) 
in modo tale che quando il processo venga colpito dovra' riportare su standard output 
le stringhe presenti in output.txt che possono aver subito sostituzione di carattere.

In caso non vi sia immissione di dati sullo standard input e non vi siano segnalazioni, 
l'applicazione dovra' utilizzare non piu' del 5% della capacita' di lavoro della CPU.
*/

#include<stdio.h> 
#include<stdlib.h> 
#include<string.h> 
#include<unistd.h> 
#include<signal.h>
#include<fcntl.h>
#include<pthread.h>
#include<semaphore.h>
#include<errno.h>
#include<sys/mman.h>
#include<sys/ipc.h>
#include<sys/types.h> 
#include<sys/sem.h>

pthread_mutex_t *ready; 
pthread_mutex_t *done;
char *input;
char **nstring;
int num_thread;  
FILE *output; 

struct info{
	int me; 
	char *my_string; 
};


void *fun_thread(void *cur){

	struct info *me = (struct info*)cur; 

	while(1){

		if(pthread_mutex_lock(ready + me->me)){
			printf("Errore lock ready + cur->me\n"); 
			exit(EXIT_FAILURE);
		} 

		for(int i=0; i<strlen(input); i++){
			if(input[i] == *me->my_string){
				input[i]='*';
			}
		}

		if(pthread_mutex_unlock(done + me->me)){
			printf("Errore unlock done + cur->me\n");
			exit(EXIT_FAILURE);
		}
	}

}



int differenza(char **nstring){

	for(int i=0; i<num_thread; i++){
		for(int j=0; j<num_thread; j++){
			if(i!=j && strcmp(nstring[i], nstring[j]) == 0){
				return 1; 
			}
		}
	}

	return 0; 
}

void print(){

	FILE *f; 
	char *out;
	int ret; 

	f = fopen("output.txt", "r+"); 
	if(f == NULL){
		printf("Errore apertura print\n"); 
		exit(EXIT_FAILURE);
	}
	printf("\n");
	while(1){

	//	if(out != NULL){free(out);}
		ret = fscanf(f, "%ms", &out); 
		if(ret  == EOF){break; }
		printf("%s\n", out); 
		fflush(stdout); 

	}


}


int main(int argc, char **argv){

	pthread_t id; 
	int ret; 
	struct sigaction sa;
	struct info *cur; 

	num_thread = argc - 1; 

	if(argc < 2){
		printf("Errore numero argomenti"); 
	}

	for(int i=0; i<num_thread; i++){
		if(strlen(argv[i + 1]) > 1){
			printf("Stringa: %d non è un carattere\n",i);
			exit(EXIT_FAILURE);
		}
	}
	nstring = argv + 1; 

	if(differenza(nstring)){
		printf("Individuate stringhe uguali\n"); 
		exit(EXIT_FAILURE);
	}

	done = malloc(sizeof(pthread_mutex_t)*num_thread); 
	ready = malloc(sizeof(pthread_mutex_t)*num_thread); 

	if(done == NULL || ready == NULL){
		printf("Errore nella malloc dei mutex\n");
		exit(EXIT_FAILURE);
	}

	if(pthread_mutex_init(ready, NULL) || pthread_mutex_init(done, NULL)){
		printf("Errore init mutex ready e done \n");
		exit(EXIT_FAILURE);
	}

	for(int i=0; i<num_thread; i++){
		if(pthread_mutex_lock(ready + i) || pthread_mutex_lock(done + i)){
			printf("Errore lock ready + i o done + i\n");
		}
	}

	output = fopen("output.txt", "w+"); 
	if(output == NULL){
		printf("Errore apertura file output\n"); 
		exit(EXIT_FAILURE);
	}

	for(int i=0; i<num_thread; i++){
		cur = malloc(sizeof(struct info));
		cur->me = i; 
		cur->my_string = nstring[i]; 
		ret = pthread_create(&id, NULL, fun_thread, (void *)cur);
		if(ret){
			printf("Errore pthread create\n"); 
			exit(EXIT_FAILURE);
		}
	}


	sa.sa_handler = print; 
	sigfillset(&sa.sa_mask); 
	sa.sa_flags = 0; 

	if(sigaction(SIGINT, &sa, NULL)){
		printf("Errore nella sigaction\n"); 
		exit(EXIT_FAILURE);
	}


	while(1){

		scanf("%ms",&input);
		fflush(stdin);

		for(int i=0; i<num_thread; i++){
unlock1: 
			if(pthread_mutex_unlock(ready + i)){
				if(errno == EINTR){goto unlock1; }
				printf("Errore uunclock1\n");
				exit(EXIT_FAILURE);
			} 
		}


		for(int j=0; j<num_thread; j++){
lock1: 
			if(pthread_mutex_lock(done + j)){
				if(errno == EINTR){goto lock1; }
				printf("Errore lock done lock1\n");
				exit(EXIT_FAILURE);
			}
		}

rewrite:
		ret = fprintf(output,"%s\n",input);
		fflush(output);
		if(ret < 0 && errno == EINTR){goto rewrite; } 
		
	}

}