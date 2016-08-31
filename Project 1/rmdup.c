/* REMOVER FICHEIROS DUPLICADOS */
/* USO: rmdup <dirname> */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <linux/limits.h>

#define NAME_MAX 255
#define DATE_MAX 25
#define FILES_MAX 500
#define LINE_MAX (PATH_MAX + 1000)

char path_lsdir[PATH_MAX];

/**
 * Represents the information of a file.
 */
struct file_info
{
	char name[NAME_MAX];
	int size;
	int permissions;
	char date[DATE_MAX];
	char dir_file[PATH_MAX + NAME_MAX];
	char erased;
};

void findAllRegularFiles(char* dir);
void sort_filetxt(int fd);
int readFilesIntoArray(struct file_info record[]);
int cmpfiles(char dir_file1[], char dir_file2[]);
void createHardLinks(char* dir, int files_readed, struct file_info record[]);

int main(int argc, char *argv[])
{
	if (argc != 2) {printf("Usage: %s <dir_path>\n", argv[0]);exit(1);}

	struct file_info record[FILES_MAX];
	int files_readed = 0;
	int fd;

	if(argv[0][0] == '.'){
		if(getcwd(path_lsdir, sizeof(path_lsdir)) == NULL){ perror("getcwd()");exit(1);}
		char temp[PATH_MAX];
		strcpy(temp, argv[0]);
		strcat(path_lsdir, temp+1);
		path_lsdir[strlen(path_lsdir)-6] = '\0';
		strcat(path_lsdir, "/lsdir");
	}
	else{
		strcpy(path_lsdir, argv[0]);
		path_lsdir[strlen(path_lsdir)-6] = '\0';
		strcat(path_lsdir, "/lsdir");
	}

	findAllRegularFiles(argv[1]);

	fd = open("files.txt", O_RDWR);
	if(fd == -1){fprintf(stderr, "open call fail");exit(2);}

	sort_filetxt(fd);

	if(close(fd) == -1){perror("close()");exit(2);}

	files_readed = readFilesIntoArray(record);

	createHardLinks(argv[1], files_readed, record);

	exit(0);
}

/**
 * Call lsdir program to find, recursively, all regular files 
 * in directory writed as argument and save them in file "files.txt".
 * @param dir directory writed as argument
 */
void findAllRegularFiles(char* dir){
	char strfd[4];
	int status;

	int fd = open("files.txt", O_RDWR | O_CREAT | O_TRUNC | O_APPEND, 0644);
	if(fd == -1){fprintf(stderr, "open()");exit(1);}

	int pid = fork();	
	if(pid == 0){
		if(chdir(dir) == -1){perror("chdir()");exit(1);} 
		sprintf(strfd,"%d",fd);
		execl(path_lsdir, path_lsdir, strfd, NULL);
		perror("execl() error2"); exit(1);
	}
	else if (pid > 0) {
		if(wait(&status) == -1){perror("wait() error");exit(1);}
		if(WIFEXITED(status) == 0){
			exit(1);
		}
		else{
			if(WEXITSTATUS(status) != 0){
				exit(1);
			}
		}
	}
	else{ perror("fork()"); exit(1);}

	if(close(fd) == -1){perror("close()");exit(1);}
}

/**
 * Sort the file "file.txt" using the command "sort".
 * @param fd file descriptor of "files.txt"
 */
void sort_filetxt(int fd){
	int pid = fork();
	int status;
	if (pid == 0){ 
		dup2(fd, STDOUT_FILENO);
		execlp("sort", "sort", "files.txt", NULL);
		perror("exclp() error"); exit(1);
	}
	else if(pid > 0){
		if(wait(&status) == -1){perror("wait error");exit(1);}
		if(!WIFEXITED(status)){ fprintf(stderr,"A child terminate abnormally\n");exit(1);}
	}
	else{ perror("fork"); exit(1);}
}

/**
 * Read the sorted files into array.
 * @param fd file descriptor of "file.txt"
 * @param record array where the information of files will stand
 * @return count number of files readed
 */
int readFilesIntoArray(struct file_info record[]){
	int count = 0;
	char line[LINE_MAX];
	FILE * fp;

	fp = fopen("files.txt","r");
	if(fp == NULL){perror("fopen()"); exit(1);}

	while (fgets(line, LINE_MAX, fp)) {

		sscanf(line,"%s %d %d %s %s", record[count].name, &(record[count].size), &(record[count].permissions),
				record[count].date, record[count].dir_file);
		record[count].erased = 0;
		count++;
	}

	fclose(fp);
	return count;
}

/**
 * Compare the content of two files with same size.
 * @param dir_file1 directory of file 1
 * @param dir_file2 directory of file 2
 * @return 0 if they are equals, 1 otherwise
 */
int cmpfiles(char dir_file1[], char dir_file2[]){
	int nr1, nr2;

	char c1[LINE_MAX];
	char c2[LINE_MAX];
	int fd1 = open(dir_file1, O_RDONLY);
	int fd2 = open(dir_file2, O_RDONLY);
	if (fd1 == -1) {
		perror(dir_file1);
		exit(1);
	}
	if(fd2 == -1){
		perror(dir_file2);
		exit(1);
	}
	while (1){
		nr1 = read(fd1, c1, FILES_MAX);
		if(nr1 == -1){
			perror(dir_file1);
			close(fd1);
			close(fd2);
			exit(1);
		}
		if ((nr2 = read(fd2, c2, nr1)) == -1 || nr2 != nr1) {
			perror(dir_file2);
			close(fd1);
			close(fd2);
			exit(1);
		}
		if(nr1 == 0){
			return 0;
		}
		else if(strncmp(c1, c2, nr1) != 0){
			return 1;
		}
	}
}

/**
 * Create hard links, and save them in dir/hlinks.txt
 * @param dir directory called in "rmdup <dir>"
 */
void createHardLinks(char* dir, int files_readed, struct file_info record[]){
	char hlinks_path[PATH_MAX + NAME_MAX];
	char hlinks_name[] = "hlinks.txt";
	int status;
	FILE* fp;
	sprintf(hlinks_path, "%s/%s", dir, hlinks_name);
	fp = fopen(hlinks_path, "w");
	if(fp == NULL){perror("fopen()");exit(1);}
	fprintf(fp, "Hard links created\n");

	int i, j;
	for(i = 0;i < files_readed-1;i++){
		if(record[i].erased != 0)
			continue;
		for(j = i+1;j < files_readed;j++){
			if(record[j].erased != 0)
				continue;
			if(strcmp(record[i].name, record[j].name) == 0 && record[i].size == record[j].size && record[i].permissions == record[j].permissions){

				if(cmpfiles(record[i].dir_file, record[j].dir_file) == 0){

					record[j].erased = 1;
					int pid = fork();
					if(pid == 0){
						execlp("rm", "rm", record[j].dir_file, NULL);
						perror("rm");
						exit(1);
					}
					else if(pid > 0){
						if(wait(&status) == -1){perror("wait()");exit(1);}
						if(!WIFEXITED(status)) exit(1);
						if(link(record[i].dir_file, record[j].dir_file) == -1){perror("link()");exit(1);}
					}
					else{perror("fork()");exit(1);}

					fprintf(fp, "%s ---> %s\n", record[i].dir_file, record[j].dir_file);
				}
			}
		}
	}
	fprintf(fp, "\nNOTE:\n<dirfile1> ---> <dirfile2> means that <dirfile2> was the hard link created\n");
	fclose(fp);
}
