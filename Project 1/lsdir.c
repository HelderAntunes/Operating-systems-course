/* LISTAR FICHEIROS REGULARES DE UM DIRECTÓRIO E SEUS SUBDIRETÓRIOS NUM FICHEIRO 
 * IDENTIFICADO PELO SEU DESCRITOR */
/* USO: lisdir <file_descriptor> */

#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <linux/limits.h>
#include <stdlib.h>
#include <sys/wait.h>

#define NAME_MAX 255
#define CHILD_MAX 100
#define DATE_MAX 25
#define LINE_MAX (PATH_MAX + 1000)

void wait_for_all_child(int n_child, int pids[]){
	int i, status;
	for(i = 0;i < n_child;i++){ 
		if(waitpid(pids[i], &status, 0) == -1){
			perror("waitpid() error");
			exit(1);
		}
		if(WIFEXITED(status) == 0){
			fprintf(stderr,"A child terminate abnormally\n");
			exit(1);
		}
	}
}

int main(int argc, char *argv[])
{
	if(argc != 2){printf("Usage: %s <fd>\n", argv[0]);return 1;}
	FILE *fp;
	DIR *dir;
	struct dirent *dentry;
	struct stat stat_entry;
	char date[DATE_MAX];
	char cwd[PATH_MAX];  
	char dir_file[PATH_MAX + NAME_MAX];
	int pids[CHILD_MAX]; 
	int pid;
	int countchild = 0;

	fp = fdopen(atoi(argv[1]),"a+");
	if(fp == NULL){perror("fdopen()");}

	if(getcwd(cwd, sizeof(cwd)) == NULL){ perror("getcwd()");exit(1);}

	if ((dir = opendir(cwd)) == NULL) { perror("opendir()");exit(1);}

	while ((dentry = readdir(dir)) != NULL) { 


		lstat(dentry->d_name, &stat_entry);
		sprintf(dir_file, "%s/%s", cwd, dentry->d_name);

		if (S_ISREG(stat_entry.st_mode)) { 

			time_t t = stat_entry.st_mtime;
			struct tm lt;
			localtime_r(&t, &lt);
			int year = lt.tm_year + 1900;
			int month = lt.tm_mon + 1;
			int day = lt.tm_mday;
			int hour = lt.tm_hour;
			int min = lt.tm_min;
			int sec = lt.tm_sec;
			sprintf(date, "%d-%02d-%02d-%02d-%02d-%02d", year, month, day, hour, min, sec);
			fprintf(fp, "%-s %d %o %s %s\n", dentry->d_name, (int)stat_entry.st_size,(int)(stat_entry.st_mode & 0777), date, dir_file);
		}
		else if(S_ISDIR(stat_entry.st_mode)){

			if(strcmp(dentry->d_name, ".") == 0 || strcmp(dentry->d_name, "..") == 0) 
				continue;

			pid = fork();
			if(pid == 0){
				if(chdir(dir_file) == -1){
					if(errno == EACCES){
						mode_t oldMode = stat_entry.st_mode;
						chmod(dir_file, S_IRUSR|S_IRGRP|S_IROTH);
						if(chdir(dir_file) == -1){perror("chdir()");exit(1);}
						chmod(dir_file, oldMode);
					}
					perror("chdir()");exit(1);
				} 
				execl(argv[0], argv[0], argv[1], NULL);
				perror("execl() error");
				exit(1);
			}
			else if(pid > 0){
				pids[countchild++] = pid;
			}
			else{perror("fork");exit(1);}
		}


	}

	wait_for_all_child(countchild, pids);

	fclose(fp);
	exit(0);
}
