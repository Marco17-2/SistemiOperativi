/*
SPECIFICATION TO BE IMPLEMENTED:
Implementare una programma che riceva in input, tramite argv[], 
N differenti stringhe S1 ... SN, con N maggiore o uguale a 1.
Per ognuna delle stringhe dovra' essere attivato un nuovo processo  
che gestira' tale stringa  (indichiamo quindi con P1 ... PN i 
processi che dovranno essere attivati).
Il processo originale dovra' leggere stringhe dallo standard input, e dovra'
comunicare ogni stringa letta a P1. P1 dovra' verificare se la stringa ricevuta
e' uguale alla stringa S1 da lui gestita, e dovra' incrementare un contatore
in caso positivo. Altrimenti, in caso negativo, dovra' comunicare la stringa
ricevuta al processo P2 che fara' lo stesso controllo, e cosi' via fino a PN.

L'applicazione dovra' gestire il segnale SIGINT (o CTRL_C_EVENT nel caso
WinAPI) in modo tale che quando uno qualsiasi dei processi Pi venga colpito
esso dovra' riportare su standard output il valore del contatore che indica
quante volte la stringa Si e' stata trovata uguale alla stringa che 
il processo originale aveva letto da standard input. Il processo originale 
non dovra' invece eseguire alcuna attivita' all'arrivo della segnalazione.

In caso non vi sia immissione di dati sullo standard input, e non vi siano 
segnalazioni, l'applicazione dovra' utilizzare non piu' del 5% della capacita' 
di lavoro della CPU.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/types.h> 

#define SIZE 4096

int cont; 
int num_proc; 
int me; 
int sem1;
int sem2;  
char *string; 
char *my_string; 


void print(){
	printf("\nProcesso: %d 		string: %s 			count: %d\n",me, my_string, cont); 
	munmap(string, SIZE);
}


void run(){

	cont = 0; 
	struct sembuf op; 
	struct sigaction sa; 
	int ret;

	sa.sa_handler = print; 
	sigfillset(&sa.sa_mask);
	sa.sa_flags = 0; 

	if(sigaction(SIGINT, &sa, NULL)){
		printf("Errore sigaction\n");
		exit(EXIT_FAILURE);
	}

	printf("Processo: %d 		my_string: %s\n", me, my_string);
	
	while(1){

		op.sem_num = me; 
		op.sem_op = -1; 
		op.sem_flg = 0; 

lock2: 
		ret = semop(sem1, &op, 1); 
		if(ret == -1 && errno == EINTR){goto lock2; }

		printf("Controllo processo: %d 		string: %s  	ricevuta: %s\n", me, my_string, string);
		fflush(stdout);

		if(strcmp(my_string, string) == 0){
			cont ++; 
		}

		op.sem_num = 0; 
		op.sem_op = 1; 
		op.sem_flg = 0; 

unlock2: 

		ret = semop(sem2, &op, 1); 
		if(ret == -1 && errno == EINTR){goto unlock2; }

		if(me < num_proc -1 ){
			
			op.sem_num = me + 1;  
			op.sem_op = 1; 
			op.sem_flg = 0; 

unlock3: 

		ret = semop(sem1, &op, 1); 
		if(ret == -1 && errno == EINTR){goto unlock3; }

		}

	}

}






int main(int argc, char **argv){

	num_proc = argc - 1; 
	int pid;
	int key1 = 1234;
	int key2 = 1212; 
	int ret; 
	char *input;
	struct sembuf op;

	sigset_t set; 
	sigfillset(&set); 
	sigprocmask(SIG_UNBLOCK, &set , NULL);


	if(argc < 1){
		printf("Errore nel numero di argomenti\n");
		exit(EXIT_FAILURE);
	}

	input = malloc(SIZE);

	string = (char*)mmap(NULL, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
	if(string == NULL){
		printf("Errore nel mmap\n"); 
		exit(EXIT_FAILURE);
	}

	sem1 = semget(key1, num_proc, IPC_CREAT); 
	semctl(sem1,1,IPC_RMID);
	sem1 = semget(key1, num_proc, IPC_CREAT|0666);

	sem2 = semget(key2, 1, IPC_CREAT); 
	semctl(sem2,1,IPC_RMID);		
	sem2 = semget(key2, 1, IPC_CREAT|0666);

	if(sem1 == -1 || sem2 == -1){
		printf("Errore semgeat\n"); 
		exit(EXIT_FAILURE);
	}


	for(int i=0; i < num_proc; i++){

		ret = semctl(sem1, i, SETVAL, 0);
		if(ret == -1 ){
			printf("Errore semctl\n");
			exit(EXIT_FAILURE);
		}
	}

	ret = semctl(sem2, 0, SETVAL, num_proc);
	if(ret == -1 ){
		printf("Errore semctl\n");
		exit(EXIT_FAILURE);
		}


	for(int i=0; i<num_proc; i++){
		my_string = argv[i + 1];
		me = i; 
		pid = fork();

		if(pid < 0){
			printf("Errore nella fork\n"); 
			exit(EXIT_FAILURE);
		}
		else if(pid == 0 ){ run(); }
	}


	while(1){

		op.sem_num = 0; 
		op.sem_op = -num_proc; 
		op.sem_flg = 0; 

lock1: 
		ret = semop(sem2, &op, 1);
		if(ret == -1 && errno == EINTR){goto lock1; } 

		scanf("%s", string);
		fflush(stdin);
	

		printf("\npresa: %s\n", string);

		op.sem_num = 0; 
		op.sem_op = 1; 
		op.sem_flg = 0; 

unlock1: 
			ret = semop(sem1, &op, 1);
			if(ret == -1 && errno == EINTR){goto unlock1; } 

	}

	return 0; 

}
