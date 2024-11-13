/* 
SPECIFICATION TO BE IMPLEMENTED:
Scrivere un programma che riceva in input tramite argv[] N coppie di stringhe 
con N maggiore o uguale a 1, in cui la prima stringa della coppia indica il 
nome di un file. 
Per ogni coppia di strighe dovra' essere attivato un processo che dovra' creare 
il file associato alla prima delle stringhe della coppia o poi ogni 5 secondi 
dovra' effettuare il controllo su quante volte la seconda delle stringhe della 
coppia compare nel file, riportando il risultato su standard output.
Il main thread del processo originale dovra' leggere lo stream da standard input in 
modo indefinito, e dovra' scrivere i byte letti in uno dei file identificati 
tramite i nomi passati con argv[]. La scelta del file dove scrivere dovra' 
avvenire in modo circolare a partire dal primo file identificato tramite argv[], 
e il cambio del file destinazione dovra' avvenire qualora venga ricevuto il 
segnale SIGINT (o CTRL_C_EVENT nel caso WinAPI).
Se il segnale SIGINT (o CTRL_C_EVENT nel caso WinAPI) colpira' invece uno degli 
altri processi, questo dovra' riportare il contenuto del file che sta gestendo 
su standard output.
*/

#include<stdio.h> 
#include<stdlib.h> 
#include<string.h> 
#include<stdlib.h> 
#include<errno.h> 
#include<signal.h> 
#include<unistd.h> 
#include<fcntl.h> 
#include<semaphore.h> 
#include<pthread.h> 
#include<sys/ipc.h> 
#include<sys/mman.h> 
#include<sys/types.h> 
#include<sys/sem.h> 


FILE **names; 
char **fnames; 
char **strings;
int turn; 
int num_pro; 
int sem;
int me; 

void cambia(){
	turn=(turn + 1)%num_pro; 
	printf("\nTurn : %d\n",turn);
	fflush(stdout);
}


void print(){
	FILE *f; 
	int ret; 
	char *s; 

	f = fopen(fnames[me*2], "r"); 
	if(f == NULL){
		printf("Errore apertura file print\n"); 
		exit(EXIT_FAILURE);
	}

	printf("\nFILE : %s\n",fnames[me*2]);
	fflush(stdout);
	while(1){

		ret = fscanf(f, "%ms", &s);
		if(ret == EOF){break; }
		printf("%s\n", s); 
		fflush(stdout);
		free(s);

	}

	fclose(f);

}


void run(){

	FILE *file;
	char *s; 
	int count; 
	int ret; 
	struct sigaction set;
	struct sembuf op;

	set.sa_handler = print;
	sigfillset(&set.sa_mask); 
	set.sa_flags = 0; 

	if(sigaction(SIGINT, &set, NULL)){
		printf("Errore sigaction run()\n");
		exit(EXIT_FAILURE);
	}


	while(1){


		op.sem_num = me + num_pro;
		op.sem_op = -1; 
		op.sem_flg = SEM_UNDO;

lock2:
		ret = semop(sem,&op,1);
		if(ret ==-1 && errno == EINTR){goto lock2; }

		sleep(3); 
		count = 0;
	
		file = fopen(fnames[me*2], "r"); 
		
		if(file == NULL){
			printf("Errore apertura file %d\n",me); 
			exit(EXIT_FAILURE);
		}

		while(1){
			ret = fscanf(file, "%ms",&s); 
			if(ret == EOF){break; }
			if(strcmp(s,strings[me*2]) == 0){
				count++;
			}
		}

		printf("\nProcess:%d 		file:%s 	string:%s 		count:%d\n",me,fnames[me*2],strings[me*2],count);

		free(s);
		fclose(file);
		sleep(2);

		op.sem_num = me;
		op.sem_op = 1; 
		op.sem_flg = SEM_UNDO;

unlock2:

		ret = semop(sem,&op,1);
		if(ret == -1 && errno == EINTR){goto unlock2; }


		if(num_pro + me + 1 < num_pro*2){
			op.sem_num = me + num_pro + 1;
			op.sem_op = 1; 
			op.sem_flg = SEM_UNDO;

unlock3:

			ret = semop(sem,&op,1);
			if(ret ==-1 && errno == EINTR){goto unlock3; }
			}
	}
}


int main(int argc, char **argv){

	int key = 1234; 
	int pid;
	struct sembuf op; 
	struct sigaction sa; 
	int ret; 
	char *input;
	int s;

	if( (argc - 1) < 2 && (argc-1)%2 != 0){
		printf("Errore numero argomenti file-string\n"); 
		exit(EXIT_FAILURE);
	}

	num_pro = (argc - 1)/2;
	strings = argv + 2;
	fnames = argv + 1;

	names = (FILE**)malloc(sizeof(FILE*)*num_pro);
	if(names == NULL){
		printf("Errore malloc names\n");
		exit(EXIT_FAILURE);
	}


	sem = semget(key, num_pro*2, IPC_CREAT|0666); 
	if(sem == -1){
		printf("Errore semget\n");
		exit(EXIT_FAILURE);
	}

	for(int i = 0; i<num_pro; i++){
		ret = semctl(sem, i, SETVAL, 1); 
		if(ret == -1){
			printf("Errore semctl1\n");
			exit(EXIT_FAILURE);
		}
	}

	for(int i = num_pro; i<num_pro*2; i++){
		ret = semctl(sem, i, SETVAL, 0); 
		if(ret == -1){
			printf("Errore semctl2\n");
			exit(EXIT_FAILURE);
		}
	}


	sa.sa_handler = cambia; 
	sigfillset(&sa.sa_mask); 
	sa.sa_flags = 0; 

	if(sigaction(SIGINT, &sa, NULL)){
		printf("Errore sigaction\n"); 
		exit(EXIT_FAILURE);
	}

	for(int i=0; i<num_pro; i++){
		me = i; 
		pid = fork(); 

		if(pid < 0){
			printf("Errore nella fork\n"); 
			exit(EXIT_FAILURE);
		}

		else if( pid == 0){	run(); }
	}

	for(int i=0; i<num_pro; i++){

		names[i] = fopen(fnames[i*2], "w+");
		if(names[i] == NULL){
			printf("Errore apertrua file: %d\n",i);
			exit(EXIT_FAILURE);
		}
	}

	turn = 0;

	while(1){

		//semaforo 1 lock
		for(int i=0; i<num_pro; i++){
			op.sem_num = i;
			op.sem_op = -1; 
			op.sem_flg = SEM_UNDO;
lock1:
		ret = semop(sem,&op,1);
		if(ret ==-1 && errno == EINTR){goto lock1; }
		
		}


input: 
		ret = scanf("%ms",&input);
		if(ret == EOF && errno == EINTR){goto input; }

 
		fprintf(names[turn],"%s\n",input);
		fflush(names[turn]);
		

		printf("\nCTRL-C cambio file\n");
		sleep(3);

		//semaforo 2 unlock
			op.sem_num = num_pro;
			op.sem_op = 1; 
			op.sem_flg = SEM_UNDO;
unlock1:
		ret = semop(sem,&op,1);
		if(ret ==-1 && errno == EINTR){goto unlock1; }

		
	}


	return 0;
}