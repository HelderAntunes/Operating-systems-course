#include <stdio.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/times.h>
#include <time.h>
#include <string.h>
#include <semaphore.h>

#define NAME_MAX 10
#define RESPONSE_SIZE 10

char SEM_NAME[] = "/sem1";
clock_t tstart;
int tcks_per_sec;
sem_t* mutex;

typedef struct request{
	char gate;
	double time_park;
	int car_id;
}request_t; 

void init_geradorfl();
int calc_time_passed_in_tcks();
void delay(double time_in_sec);
void create_request(request_t* req, int car_id, double u_relogio);
void make_fifo(const char* name_of_fifo);
int open_ctrl_fifo(char gate);
void write_in_parkfl(FILE* fp, request_t* req, const char* observ, int time_live);
void* vehicle(void *arg);
double calc_interval_generation(double u_relogio);

int main(int argc, char *argv[]){

	if(argc != 3){
		fprintf(stderr, "USAGE: %s <T_GERACAO> <U_RELOGIO>\n", argv[0]);
		exit(1);
	}
	
	double t_geracao = (double)atoi(argv[1]);
	tcks_per_sec = sysconf(_SC_CLK_TCK);
	double u_relogio = (double)atoi(argv[2])/tcks_per_sec;
	
	double t = 0;
	double interval;
	int car_id = 1;
	
	mutex = sem_open(SEM_NAME, O_CREAT, 0644, 1);
	if(mutex == SEM_FAILED){
      perror("unable to create semaphore");
      sem_unlink(SEM_NAME);
      exit(-1);
   }
	
	srand(time(NULL));
	
	init_geradorfl();
	
	tstart = times(NULL);
	
	while(t <= t_geracao){
		
		request_t* req = malloc(sizeof(request_t));
		create_request(req, car_id, u_relogio);
	
		pthread_t vehicle_id;
		pthread_create(&vehicle_id, NULL, vehicle, (void*) req);
		
		interval = calc_interval_generation(u_relogio);
		delay(interval);
		t += interval;
		
		car_id++;
	}
	
	sem_close(mutex);
  	sem_unlink(SEM_NAME);
	
	pthread_exit(NULL);
}

void* vehicle(void *arg){
	
	if(pthread_detach(pthread_self()) != 0){
		fprintf(stderr, "impossible detach a thread");
	}
	
	request_t* req = (request_t*) arg;
	
	char private_fifo_name[NAME_MAX];
	sprintf(private_fifo_name, "fifo%d", (*req).car_id);
	make_fifo(private_fifo_name);
	int private_fd = open(private_fifo_name, O_RDWR);
	
	FILE* fp = fopen("gerador.log", "a");
	
	sem_wait(mutex);
	int ctrl_fifo_fd = open_ctrl_fifo((*req).gate);
	
	if(ctrl_fifo_fd != -1){
		
		write(ctrl_fifo_fd, req, sizeof(request_t));
		close(ctrl_fifo_fd);
		sem_post(mutex);
		
		char response[RESPONSE_SIZE];
		read(private_fd, response, RESPONSE_SIZE);
		
		if(strcmp(response, "entrada") == 0){
			
			write_in_parkfl(fp, req, response, -1);

			clock_t time_start = times(NULL);
			read(private_fd, response, RESPONSE_SIZE);
			clock_t time_end = times(NULL);
			
			write_in_parkfl(fp, req, response, (int)time_end-time_start);
		}
		else{
			write_in_parkfl(fp, req, response, -1);
		}
	}
	else{
		sem_post(mutex);
	}
	
	free(req);
	fclose(fp);
	close(private_fd);
	if(unlink(private_fifo_name) != 0){
		perror(private_fifo_name);
	}
	pthread_exit(NULL);
}

int calc_time_passed_in_tcks(){
	clock_t tatual = times(NULL);
	return (int)(tatual-tstart);
}

void delay(double time_in_sec){
	sleep((int)time_in_sec);
	usleep((time_in_sec-(int)time_in_sec)*1000000);
}

void create_request(request_t* req, int car_id, double u_relogio){
	int gate = rand()%4;
	
	switch(gate){
	case 0:
		(*req).gate = 'N';
		break;
	case 1:
		(*req).gate = 'S';
		break;
	case 2:
		(*req).gate = 'E';
		break;
	case 3:
		(*req).gate = 'O';
		break;
	}
	
	(*req).time_park = (rand()%10+1)*u_relogio;
	
	(*req).car_id = car_id;
	
}

void make_fifo(const char* name_of_fifo){
	if(mkfifo(name_of_fifo, 0600) != 0){
		if(errno == EEXIST)
			printf("FIFO %s already exists\n", name_of_fifo);
		else
			printf("Can't create FIFO %s\n", name_of_fifo);
	}
}

int open_ctrl_fifo(char gate){
	int ctrl_fifo_fd;
	
	switch(gate){
	case 'N':
		ctrl_fifo_fd = open("fifoN", O_WRONLY | O_NONBLOCK);
		break;
	case 'S':
		ctrl_fifo_fd = open("fifoS", O_WRONLY | O_NONBLOCK);
		break;
	case 'E':
		ctrl_fifo_fd = open("fifoE", O_WRONLY | O_NONBLOCK);
		break;
	case 'O':
		ctrl_fifo_fd = open("fifoO", O_WRONLY | O_NONBLOCK);
		break;
	default:
		break;	
	}
	
	return ctrl_fifo_fd;
}

void write_in_parkfl(FILE* fp, request_t* req, const char* observ, int time_live){
	if(time_live != -1){
		fprintf(fp, "%7d  ; %4d    ; %4c   ;    %4d    ;   %4d ; %s\n",(int)calc_time_passed_in_tcks(), (*req).car_id, (*req).gate, (int)((*req).time_park* tcks_per_sec), time_live, observ);
	}
	else{
		fprintf(fp, "%7d  ; %4d    ; %4c   ;    %4d    ;      ? ; %s\n",(int)calc_time_passed_in_tcks(), (*req).car_id, (*req).gate, (int)((*req).time_park* tcks_per_sec), observ);
	}
	
}

double calc_interval_generation(double u_relogio){
	int n = rand()%10;
	if(n <= 4)
		return 0;
	else if(n <= 7)
		return u_relogio;
	else
		return 2*u_relogio;
}

void init_geradorfl(){
	int geradorlog_fd = open("gerador.log", O_RDWR | O_CREAT | O_TRUNC | O_APPEND, 0644);
	write(geradorlog_fd, "t(ticks) ; id_viat ; destin ; t_estacion ; t_vida ; observ\n", strlen("t(ticks) ; id_viat ; destin ; t_estacion ; t_vida ; observ\n"));
}

