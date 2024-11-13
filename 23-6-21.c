/*
SPECIFICATION TO BE IMPLEMENTED:
Implementare una programma che riceva in input, tramite argv[], il nome
di un file F. Il programa dovra' creare il file F e popolare il file
con lo stream priveniente da standard-input. Il programma dovra' generare
anche un ulteriore processo il quale dovra' riversare il contenuto  che 
viene inserito in F su un altro file denominato shadow_F, tale inserimento
dovra' essere realizzato in modo concorrente rispetto all'inserimento dei dati su F.

L'applicazione dovra' gestire il segnale SIGINT (o CTRL_C_EVENT nel caso
WinAPI) in modo tale che quando un qualsiasi processo (parent o child) venga colpito si
dovra' immediatamente emettere su standard-output il contenuto del file 
che il processo child sta popolando. 

Qualora non vi sia immissione di input, l'applicazione dovra' utilizzare 
non piu' del 5% della capacita' di lavoro della CPU.
*/

//non so se è quello corretto

#include<stdio.h> 
#include<stdlib.h> 
#include<string.h> 
#include<errno.h> 
#include<signal.h> 
#include<fcntl.h> 
#include<unistd.h> 
#include<pthread.h> 
#include<semaphore.h> 
#include<sys/ipc.h> 
#include<sys/types.h> 
#include<sys/mman.h> 
#include<sys/sem.h>


FILE *f; 
char *fname; 
char *sname;
int pid;
int sem;


void invio(){
	fflush(stdout);
	if(kill(pid, SIGINT)){
		printf("Errore invio del segnale\n");
		exit(EXIT_FAILURE);
	}
}

void printer(){

	FILE *file; 
	char *str;
	int ret;

	file = fopen(sname, "r"); 
	if(file == NULL){
		printf("Errore aprtura file\n");
		exit(EXIT_FAILURE);
	}

	printf("\nFILE : %s \n",sname);

	while(1){
	
		ret = fscanf(file, "%ms", &str);
		if(ret == EOF){ break; }
		printf("%s\n", str);
		fflush(stdout);
		free(str);

	}
}

void run(){

	FILE *shadow;
	FILE *fi;
	struct sigaction set; 
	struct sembuf oper;
	char *str;
	int ret; 

	shadow = fopen(sname, "w+");

	if(shadow == NULL){
		printf("Errore apertura fileeeee\n");
		exit(EXIT_FAILURE);
	}

	fi = fopen(fname, "r");
	if(fi == NULL){
		printf("Errore aprtrua file\n");
		exit(EXIT_FAILURE);
	}

	set.sa_handler = printer; 
	sigfillset(&set.sa_mask);
	set.sa_flags = 0; 

	if(sigaction(SIGINT, &set, NULL)){
		printf("Errore nella sigaction2\n");
		exit(EXIT_FAILURE);
	}

	while(1){

		oper.sem_num = 0;
		oper.sem_op = -1;
		oper.sem_flg = SEM_UNDO;

lock2:
		ret = semop(sem,&oper,1);
		if(ret == -1 && errno == EINTR){goto lock2; }


lettura:
		ret = fscanf(fi, "%ms", &str);
		printf("\nSTRINGA LETTA : %s\n",str);
			
		if(ret < 0 && errno == EINTR){ goto lettura; }

		if(ret == EOF){continue; }

scrittura:
			ret = fprintf(shadow, "%s\n", str);
			fflush(shadow);
			if(errno == EINTR && ret < 0){goto scrittura; }
			free(str);

		oper.sem_num = 1;
		oper.sem_op = 1;
		oper.sem_flg = SEM_UNDO;

unlock2: 

		ret = semop(sem,&oper,1);
		if(ret == -1 && errno == EINTR){goto unlock2; }
		
	}

}



int main(int argc, char **argv){
 
	char *input;
	int ret;
	int key = 1234;		
	struct sembuf oper;
	struct sigaction sa;

	if(argc != 2){
		printf("Errore numero di argomenti\n");
		exit(EXIT_FAILURE);
	}

	fname = argv[1];
	sname = malloc(strlen("shadow_") + strlen(fname)+1);
	
/*	strcpy(sname,"shadow_");
	strcat(sname,fname);*/
	sprintf(sname, "shadow_%s", fname);
	printf("\nSFILE: %s\n", sname);

	f = fopen(fname,"w+");

	if(f == NULL){
		printf("Erore apertura file\n");
		exit(EXIT_FAILURE);
	}

	sem = semget(key,2,IPC_CREAT|0666);
	
	if(sem == -1){
		printf("semget error\n");
		return -1;
	}


	ret = semctl(sem,0,SETVAL,0);
	if(ret == -1){
		printf("semctl error\n");
		exit(-1);
	}

	ret = semctl(sem,1,SETVAL,1);
	if(sem == -1){
		printf("semctl error\n");
		exit(-1);
	}

	pid = fork();

	if(pid < 0 ){
		printf("Errore nella fork\n");
		exit(EXIT_FAILURE);
	}

	if(pid == 0){run(); }

	sa.sa_handler = invio; 
	sigfillset(&sa.sa_mask);
	sa.sa_flags = 0; 

	if(sigaction(SIGINT, &sa, NULL)){
		printf("Errore sigaction\n");
		exit(EXIT_FAILURE);
	}

	while(1){

		oper.sem_num = 1;
		oper.sem_op = -1;
		oper.sem_flg = SEM_UNDO;

lock1:
		ret = semop(sem,&oper,1);
		if(ret ==-1 && errno == EINTR){goto lock1; }

input:
		ret = scanf("%ms", &input);
		if(ret == EOF && errno == EINTR){goto input;}


write:
		ret = fprintf(f, "%s\n", input);
		fflush(f);
		if(ret < 0 && errno == EINTR){goto input; }


		oper.sem_num = 0;
		oper.sem_op = 1;
		oper.sem_flg = SEM_UNDO;

unlock1:
		ret = semop(sem,&oper,1);
		if(ret == -1 && errno == EINTR){ goto unlock1;}

	}



	return 0;
}