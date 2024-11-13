/*
SPECIFICATION TO BE IMPLEMENTED:
Si sviluppi una applicazione che riceva tramite argv[] la seguente linea di comando

    nome_prog -f file1 [file2] ... [fileN] -s stringa1 [stringa2] ... [stringaN] 
    
indicante N nomi di file (con N > 0) ed N ulteriori stringhe (il numero dei nomi dei 
file specificati deve corrispondere al numero delle stringhe specificate).

L'applicazione dovra' generare N processi figli concorrenti, in cui l'i-esimo di questi 
processi effettuera' la gestione dell'i-esimo dei file identificati tramite argv[].
Tale file dovra' essere rigenerato allo startup dell'applicazione.
Il main thread del processo originale dovra' leggere indefinitamente stringhe da 
standard input e dovra' comparare ogni stringa letta che le N stringhe ricevute in 
input tramite argv[].
Nel caso in cui la stringa letta sia uguale alla i-esima delle N stringhe ricevuta 
in input, questa dovra' essere comunicata all'i-esimo processo figlio in modo che questo 
la possa inserire in una linea del file di cui sta effettuando la gestione. Invece, 
se il main thread legge una stringa non uguale ad alcuna delle N stringhe ricevute 
in input, questa stringa dovra essere comunicata a tutti gli N processi figli
attivi, che la dovranno scrivere sui relativi file in una nuova linea.

L'applicazione dovra' gestire il segnale SIGINT (o CTRL_C_EVENT nel caso
WinAPI) in modo tale che quando uno qualsiasi dei processi figli venga colpito 
dovra' riportare il contenuto del file da esso correntemente gestito in un file
con lo stesso nome ma con suffisso "_backup".  Invece il processo originale non dovra'
terminare o eseguire alcuna attivita' in caso di segnalazione.
*/

#include<stdio.h> 
#include<stdlib.h> 
#include<string.h> 
#include<errno.h> 
#include<unistd.h>
#include<signal.h> 
#include<fcntl.h> 
#include<stdbool.h> 
#include<pthread.h> 
#include<semaphore.h> 
#include<sys/types.h> 
#include<sys/mman.h>
#include<sys/ipc.h> 
#include<sys/sem.h>

#define SIZE 4096 //supponiamo lunghezza strighe <= 4096

int num; 
int me; 
int sem; 
char *string;
char **input; 
char **fnames; 


void print(){

	FILE *f; 
	int ret; 
	char *out; 

	f = fopen(fnames[me], "r");
	if(f == NULL){
		printf("Errore apertura file print\n"); 
		exit(EXIT_FAILURE);
	}

	printf("\nFile: %s\n", fnames[me]);
	while(1){

		ret = fscanf(f, "%ms", &out); 
		if(ret == EOF ){ break; }
		printf("%s\n", out); 
		fflush(stdout);
		free(out); 

	}
}


void run(){

	struct sembuf op; 
	struct sigaction sa;
	FILE *fi;
	int ret; 

	fi = fopen(fnames[me], "w+");
	if(fi == NULL){
		printf("Errore apertura file\n");
		exit(EXIT_FAILURE);
	}

	sa.sa_handler = print; 
	sigfillset(&sa.sa_mask); 
	sa.sa_flags = 0; 

	if(sigaction(SIGINT, &sa, NULL)){
		printf("Errore sigaction run\n");
		exit(EXIT_FAILURE);
	}

	printf("Scrivo nel File: %s\n", fnames[me]);

	while(1){

		if(input[me] != NULL){

			printf("\nProcesso: %d 		scrive: %s\n", me, input[me]);

			fprintf(fi, "%s\n", input[me]); 
			fflush(fi);

			op.sem_num = me;
			op.sem_op = 1; 
			op.sem_flg = SEM_UNDO;

			input[me] = NULL;

unlock4:

			ret = semop(sem, &op, 1); 
			if(ret == 1 && errno == EINTR){goto unlock4;} 
		}

		ret = semctl(sem, me, SETVAL, 1); 
		if(ret == -1){
			printf("Errore semctl1\n");
			exit(EXIT_FAILURE);
		}
	}
}



int main(int argc, char **argv){

	int pid; 
	int ret; 
	int key = 1234; 
	bool t; 
	struct sembuf op;
	sigset_t set; 
	
	if(argc % 2 != 0){
		printf("Numero errato di argomenti\n");
		exit(EXIT_FAILURE);
	}

	num = (argc - 4 )/2;
	fnames = argv + 3;

	input = (char**)mmap(NULL, sizeof(char*)*num, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);

	string = (char*)mmap(NULL, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, 0, 0);
	

	sem = semget(key, num, IPC_CREAT | 0666);
	if(sem == -1){
		printf("Errore semget\n");
		exit(EXIT_FAILURE);
	}

	for(int i=0; i<num; i++){
		ret = semctl(sem, i, SETVAL, 1); 
		if(ret == -1){
			printf("Errore semctl1\n");
			exit(EXIT_FAILURE);
		}
	}

	for(int i=0; i<num; i++){
		me = i; 
		pid = fork();
		if(pid < 0 ){
			printf("Errore fork\n");
			exit(EXIT_FAILURE);
		}
		else if(pid == 0){run(); }
	}

	sigfillset(&set);
	sigprocmask(SIG_UNBLOCK, &set, NULL);

	while(1){

		t = true; 

		for(int i=0; i<num; i++){

			op.sem_num = i;
			op.sem_op = -1; 
			op.sem_flg = SEM_UNDO;

lock1: 
			ret = semop(sem, &op, 1);
			if(ret == -1 && errno == EINTR){goto lock1; }
		}

		while(scanf("%s", string) <= 0 );

		for(int i=0; i<num; i++){
			printf("\nstring: %s 		input: %s\n",argv[num + 4 + i], string);
			sleep(3);

			if(strcmp(argv[num + 4 + i], string) == 0){
				
				input[i] = string;
				t = false; 

			}
		}

		if(t == true){
			for(int i=0; i<num; i++){
					input[i] = string;
			}
		}

	}

	return 0;
}