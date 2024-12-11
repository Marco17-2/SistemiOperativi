/*
SPECIFICATION TO BE IMPLEMENTED:
Implementare una programma che riceva in input, tramite argv[], il nome
di un file F. Il programa dovra' creare il file F e popolare il file
con lo stream proveniente da standard-input. Il programma dovra' generare
anche un ulteriore processo il quale dovra' riversare il contenuto del file F
su un altro file denominato shadow_F, inserendo mano a mano in shadow_F soltanto 
i byte che non erano ancora stati prelevati in precedenza da F. L'operazione 
di riversare i byte di F su shadow_F dovra' avvenire con una periodicita' di 10
secondi.  

L'applicazione dovra' gestire il segnale SIGINT (o CTRL_C_EVENT nel caso
WinAPI) in modo tale che quando un qualsiasi processo (parent o child) venga colpito si
dovra' immediatamente riallineare il contenuto del file shadow_F al contenuto
del file F sempre tramite operazioni che dovra' eseguire il processo child.

Qualora non vi sia immissione di input, l'applicazione dovra' utilizzare 
non piu' del 5% della capacita' di lavoro della CPU.
*/

// Mezzo buggato ma "funziona" al Ctrl-C riscrive due volte l'ultimo input

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/sem.h>
#define SIZE 45

char *fname; 
char *sname; 
int sem; 
int pid; 
int *cont;
int contp; 
FILE *file; 
FILE *shadow;
	
void invio(){
	if(kill(pid, SIGINT)){
		printf("Errore nella kill\n"); 
		exit(EXIT_FAILURE);
	}
}



void allinea(){

	//FILE *file, *shadow;
	char *output; 
	int ret; 

	//file = fopen(fname, "r+");
	//shadow = fopen(sname, "r+"); 

	fflush(stdin);


	if(file == NULL || shadow == NULL){
		printf("Errore apertura file allinea\n"); 
		exit(EXIT_FAILURE);
	}

	for(; contp < *cont; contp++){

			ret = fscanf(file, "%ms", &output);
				if(ret > 0){
					printf("Processo Stringa: %s\n",output);

					fprintf(shadow, "%s\n", output);			
					fflush(shadow);
			}
		
	}
}



void run(){

	//FILE *file; 
	//FILE *shadow;
	struct sembuf op;  
	struct sigaction sa; 
	char *output; 
	int ret; 
	contp = 0; 

	file = fopen(fname, "r+"); 
	shadow = fopen(sname, "w+"); 
	if(file == NULL || shadow == NULL){
		printf("Errore apertura file run \n"); 
		exit(EXIT_FAILURE);
	}

	sa.sa_handler = allinea; 
	sigfillset(&sa.sa_mask); 
	sa.sa_flags = 0; 
	if(sigaction(SIGINT, &sa, NULL)){
		printf("Errore nella sigaction run\n"); 
		exit(EXIT_FAILURE);
	}

	while(1){

start:

		sleep(15); 

		op.sem_num = 0; 
		op.sem_op = -1;
		op.sem_flg = 0;

lock2: 
		ret = semop(sem, &op, 1);
		if(ret == -1 && errno == EINTR){ goto lock2; }

		for(; contp < *cont; contp++){

			ret = fscanf(file, "%ms", &output);
			printf("Processo Stringa: %s\n",output);
			if(ret == EOF && errno == EINTR){goto reset; };

			if(ret > 0){

				ret = fprintf(shadow, "%s\n", output);			
				fflush(shadow);
				if(ret < 0 && errno == EINTR){goto reset; }

			}	
		}

		op.sem_num = 0; 
		op.sem_op = 1;
		op.sem_flg = 0;


unlock2: 
		ret = semop(sem,&op,1);
		if(ret == -1 && errno == EINTR){ goto unlock2; }

	}

	reset: 
		
		/*fclose(file);
		fclose(shadow);
		file = fopen(fname, "r+"); 
		shadow = fopen(sname, "r+"); */
		goto start;


}

int main(int argc, char ** argv){

	int key = 1234; 
	int ret; 
	struct sembuf op; 
	struct sigaction sa; 
	char *input; 
	FILE *f;

	if(argc != 2){
		printf("Errore numero argomenti\n");
		exit(EXIT_FAILURE);
	}

	fname = argv[1]; 
	sname = malloc(strlen("_shadow") + strlen(fname) + 1); 

	cont = (int*)mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
	if(cont == NULL){
		printf("Errore mmap\n");
		exit(EXIT_FAILURE);
	}

	*cont = 0;

	if(sname == NULL){
		printf("Errore malloc\n"); 
		exit(EXIT_FAILURE);
	}

	sprintf(sname, "shadow_%s", fname);
	printf("FILE: %s   %s\n", fname, sname);
	
	sem = semget(key, 1, IPC_CREAT|0666); 
	semctl(sem,0,IPC_RMID);
	sem = semget(key, 1, IPC_CREAT|0666); 
	if(sem == -1 ){
		printf("Errore nella semget\n"); 
		exit(EXIT_FAILURE);
	}	

	if(semctl(sem, 0, SETVAL, 1) == -1){
		printf("Errore semctl\n");
		exit(EXIT_FAILURE);
	}

	f = fopen(fname, "w+"); 
	if(f == NULL){
		printf("Errore apertura file main\n");
		exit(EXIT_FAILURE);
	}

	sa.sa_handler = invio; 
	sigfillset(&sa.sa_mask);
	sa.sa_flags = 0; 

	if(sigaction(SIGINT, &sa, NULL)){
		printf("Errore nella sigaction\n"); 
		exit(EXIT_FAILURE);
	}

	pid = fork(); 
	if(pid < 0 ){
		printf("Errore fork\n");
		exit(EXIT_FAILURE);
	}
	else if(pid == 0){ run(); }

	while(1){ 

		op.sem_num = 0; 
		op.sem_op = -1;
		op.sem_flg = 0;


lock1: 
		ret = semop(sem, &op, 1);
		if(ret == -1 && errno == EINTR){goto lock1; }

		printf("Input>> ");
		scanf("%ms", &input); 
		fflush(stdin);

		ret = fprintf(f,"%s\n",input);
		fflush(f);
		

		printf("\nstringa: %s   caratteri scritti: %d\n",input, ret - 1);
		(*cont) ++;


		op.sem_num = 0; 
		op.sem_op = 1;
		op.sem_flg = 0;


unlock1: 
		ret = semop(sem, &op, 1);
		if(ret == -1 && errno == EINTR){goto unlock1;}


		sleep(3);

	}

	return 0;
}