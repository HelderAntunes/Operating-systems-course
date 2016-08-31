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

#define NAME_MAX 255
char SEM_NAME[] = "/sem1";

typedef struct request{
	char gate;
	double time_park;
	int car_id;
}request_t;


clock_t tstart;
int places_used;
int park_capacity;
pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;

int calc_time_passed_in_tcks();
void delay(double time_in_sec);
void make_fifo(const char* name_of_fifo);
void lock_mut();
void unlock_mut();
void init_parquefl();
void write_in_parkfl(FILE* fp, request_t* req, const char* observ);
void* north_ctrl(void *arg);
void* south_ctrl(void *arg);
void* west_ctrl(void *arg);
void* east_ctrl(void *arg);
void* arranger(void *arg);


int main(int argc, char *argv[]){

	if(argc != 3){
		fprintf(stderr, "USAGE: %s <N_LUGARES> <T_ABERTURA>\n", argv[0]);
		exit(1);
	}
	char SEM_NAME[] = "/sem1";
	pthread_t northid, southid, westid, eastid;
	
	init_parquefl();
	
	park_capacity = atoi(argv[1]);
	places_used = 0;
	
	int opening_time = atoi(argv[2]); 
	
	tstart = times(NULL);
	
	sem_t* mutex;
	mutex = sem_open(SEM_NAME, O_CREAT, 0644, 1);
	if(mutex == SEM_FAILED){
      perror("unable to create semaphore");
      sem_unlink(SEM_NAME);
      exit(-1);
   }
	
	make_fifo("fifoN");
	make_fifo("fifoS");
	make_fifo("fifoE");
	make_fifo("fifoO");

	pthread_create(&northid, NULL, north_ctrl, NULL);
	pthread_create(&southid, NULL, south_ctrl, NULL);
	pthread_create(&westid, NULL, west_ctrl, NULL);
	pthread_create(&eastid, NULL, east_ctrl, NULL);
	
	int fd_N = open("fifoN", O_WRONLY); 
	int fd_S = open("fifoS", O_WRONLY); 
	int fd_O = open("fifoO", O_WRONLY); 
	int fd_E = open("fifoE", O_WRONLY); 
	
	sleep(opening_time);
	
	request_t* req_stop = malloc(sizeof(request_t));
	(*req_stop).car_id = -1;
	sem_wait(mutex);
	write(fd_N, req_stop, sizeof(request_t));
	write(fd_S, req_stop, sizeof(request_t));
	write(fd_O, req_stop, sizeof(request_t));
	write(fd_E, req_stop, sizeof(request_t));
	sem_post(mutex);
	free(req_stop);
	
	close(fd_N);
	close(fd_S);
	close(fd_E);
	close(fd_O);
	
	pthread_join(northid, NULL);
	pthread_join(southid, NULL);
	pthread_join(westid, NULL);
	pthread_join(eastid, NULL);
	
	sem_close(mutex);
   sem_unlink(SEM_NAME);
    
	unlink("fifoN");
	unlink("fifoS");
	unlink("fifoE");
	unlink("fifoO");
	
	pthread_exit(NULL);
}

void* north_ctrl(void *arg){
	int closed = 0;
	
	int fd = open("fifoN", O_RDONLY);
	if(fd == -1){fprintf(stderr, "open fifoN fail");return NULL;}

	do{
		request_t* req = malloc(sizeof(request_t));
		
		if(read(fd, req, sizeof(request_t)) == 0){
			free(req);
			break;
		}
		else {
			
			if((*req).car_id == -1){
				closed = 1;
				free(req);
			}
			else if(closed == 1){
			
				FILE* fp = fopen("parque.log", "a");
				write_in_parkfl(fp, req, "encerrado");
				fclose(fp);
				
				char private_fifo_name[NAME_MAX];
				sprintf(private_fifo_name, "fifo%d", (*req).car_id);
				
				int private_fd = open(private_fifo_name, O_WRONLY);
				if(fd == -1){
					perror(private_fifo_name);
				}
				
				int nw = write(private_fd, "encerrado", sizeof("encerrado"));
				if(nw == -1)
					perror(private_fifo_name);
				
				close(private_fd);
				free(req);
			}
			else{
				pthread_t arranger_id;
				pthread_create(&arranger_id, NULL, arranger, (void *)req);
			}
		}
	}while(1);
	
	close(fd);
	pthread_exit(NULL);
}

void* south_ctrl(void *arg){
	int closed = 0;
	
	int fd = open("fifoS", O_RDONLY);
	if(fd == -1){fprintf(stderr, "open fifoN fail");return NULL;}

	do{
		request_t* req = malloc(sizeof(request_t));
		
		if(read(fd, req, sizeof(request_t)) == 0){
			free(req);
			break;
		}
		else {
			
			if((*req).car_id == -1){
				closed = 1;
				free(req);
			}
			else if(closed == 1){
			
				FILE* fp = fopen("parque.log", "a");
				write_in_parkfl(fp, req, "encerrado");
				fclose(fp);
				
				char private_fifo_name[NAME_MAX];
				sprintf(private_fifo_name, "fifo%d", (*req).car_id);
				
				int private_fd = open(private_fifo_name, O_WRONLY);
				if(fd == -1){
					perror(private_fifo_name);
				}
				
				int nw = write(private_fd, "encerrado", sizeof("encerrado"));
				if(nw == -1)
					perror(private_fifo_name);
				
				close(private_fd);
				free(req);
			}
			else{
				pthread_t arranger_id;
				pthread_create(&arranger_id, NULL, arranger, (void *)req);
			}
		}
	}while(1);
	
	close(fd);
	pthread_exit(NULL);
}

void* west_ctrl(void *arg){
	int closed = 0;
	
	int fd = open("fifoO", O_RDONLY);
	if(fd == -1){fprintf(stderr, "open fifoN fail");return NULL;}

	do{
		request_t* req = malloc(sizeof(request_t));
		
		if(read(fd, req, sizeof(request_t)) == 0){
			free(req);
			break;
		}
		else {
			
			if((*req).car_id == -1){
				closed = 1;
				free(req);
			}
			else if(closed == 1){
			
				FILE* fp = fopen("parque.log", "a");
				write_in_parkfl(fp, req, "encerrado");
				fclose(fp);
				
				char private_fifo_name[NAME_MAX];
				sprintf(private_fifo_name, "fifo%d", (*req).car_id);
				
				int private_fd = open(private_fifo_name, O_WRONLY);
				if(fd == -1){
					perror(private_fifo_name);
				}
				
				int nw = write(private_fd, "encerrado", sizeof("encerrado"));
				if(nw == -1)
					perror(private_fifo_name);
				
				close(private_fd);
				free(req);
			}
			else{
				pthread_t arranger_id;
				pthread_create(&arranger_id, NULL, arranger, (void *)req);
			}
		}
	}while(1);
	
	close(fd);
	pthread_exit(NULL);
}

void* east_ctrl(void *arg){
	int closed = 0;
	
	int fd = open("fifoE", O_RDONLY);
	if(fd == -1){fprintf(stderr, "open fifoN fail");return NULL;}

	do{
		request_t* req = malloc(sizeof(request_t));
		
		if(read(fd, req, sizeof(request_t)) == 0){
			free(req);
			break;
		}
		else {
			
			if((*req).car_id == -1){
				closed = 1;
				free(req);
			}
			else if(closed == 1){
			
				FILE* fp = fopen("parque.log", "a");
				write_in_parkfl(fp, req, "encerrado");
				fclose(fp);
				
				char private_fifo_name[NAME_MAX];
				sprintf(private_fifo_name, "fifo%d", (*req).car_id);
				
				int private_fd = open(private_fifo_name, O_WRONLY);
				if(fd == -1){
					perror(private_fifo_name);
				}
				
				int nw = write(private_fd, "encerrado", sizeof("encerrado"));
				if(nw == -1)
					perror(private_fifo_name);
				
				close(private_fd);
				free(req);
			}
			else{
				pthread_t arranger_id;
				pthread_create(&arranger_id, NULL, arranger, (void *)req);
			}
		}
	}while(1);
	
	close(fd);
	pthread_exit(NULL);
}

void* arranger(void *arg){
	
	if(pthread_detach(pthread_self()) != 0){
		fprintf(stderr, "impossible detach a thread");
	}
	
	request_t* req = (request_t*) arg;
	
	FILE* fp = fopen("parque.log", "a");
	
	char private_fifo_name[NAME_MAX];
	sprintf(private_fifo_name, "fifo%d", (*req).car_id);
	int fd = open(private_fifo_name, O_WRONLY);
	if(fd == -1){
		perror(private_fifo_name);
		pthread_exit(NULL);
	}
	
	int enter = 0;
	
	lock_mut();
	if(places_used < park_capacity){
		enter = 1;
		places_used++;
		
		int nw = write(fd, "entrada", sizeof("entrada"));
		if(nw == -1)
			perror(private_fifo_name);
		
		write_in_parkfl(fp, req, "estacionamento");
	}
	else{
		int nw = write(fd, "cheio!", sizeof("cheio!"));
		if(nw == -1)
			perror(private_fifo_name);
		
		write_in_parkfl(fp, req, "cheio");
	}
	unlock_mut();
		
	if(enter){
		delay((double)(*req).time_park);
		
		int nw = write(fd, "saida", sizeof("saida"));
		if(nw == -1)
			perror(private_fifo_name);
		
		write_in_parkfl(fp, req, "saida");
		
		lock_mut();
		places_used--;
		unlock_mut();
	}
	
	free(req);
	fclose(fp);
	close(fd);
	
	pthread_exit(NULL);
}

void init_parquefl(){
	int parklog_fd = open("parque.log", O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0644);
	FILE* fp = fdopen(parklog_fd, "a");
	fprintf(fp, "t(ticks) ; nlug ; id_viat ; observ\n");
	fclose(fp);
	close(parklog_fd);
}

void lock_mut(){
	if(pthread_mutex_lock(&mut) != 0){
		fprintf(stderr, "error of pthread_mutex_lock()");
	} 
}

void unlock_mut(){
	if(pthread_mutex_unlock(&mut) != 0){
		fprintf(stderr, "error of pthread_mutex_unlock()");
	}
}

void write_in_parkfl(FILE* fp, request_t* req, const char* observ){
	fprintf(fp, "%7d  ; %4d ; %5d   ; %s\n", calc_time_passed_in_tcks(), places_used, (*req).car_id, observ);
}

void make_fifo(const char* name_of_fifo){
	if(mkfifo(name_of_fifo, 0600) != 0){
		if(errno == EEXIST)
			printf("FIFO %s already exists\n", name_of_fifo);
		else
			printf("Can't create FIFO %s\n", name_of_fifo);
	}
}

void delay(double time_in_sec){
	sleep((int)time_in_sec);
	usleep((time_in_sec-(int)time_in_sec)*1000000);
}

int calc_time_passed_in_tcks(){
	clock_t tatual = times(NULL);
	return (int)(tatual-tstart);
}


