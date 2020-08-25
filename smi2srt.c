#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>

#define PATH_LEN 2048
#define FILE_LEN 128
#define CMD_LEN PATH_LEN * 2 + 20

#define CONVERT_V1 "./convert_v1 \""
#define CONVERT_V2 "/usr/local/bin/smi2srt -n \""
#define GREP "grep -i \"<body>\" \""
#define GREPFILE "grep.tmp"
#define TMPFILE "tmp.tmp"
#define LOGFILE "log.txt"

#define STDOUT_SAVE 100
#define STDERR_SAVE 101

typedef enum {false, true} bool;

bool backup = false;

char cmd_v1[CMD_LEN] = CONVERT_V1;
char cmd_v2[CMD_LEN] = CONVERT_V2;
char grepCmd[CMD_LEN] = GREP;

char pwd[PATH_LEN];
char nowRootDir[PATH_LEN];
char backupDir[PATH_LEN];

int testFd;


void redirection(char *cmd, const char *tmpFile)
{
	int tmpFd;
	if((tmpFd = open(tmpFile, O_RDWR | O_CREAT | O_TRUNC, 0644)) < 0){
		fprintf(stderr, "open error for %s\n", tmpFile);
		exit(1);
	}
	dup2(STDOUT_FILENO, STDOUT_SAVE);
	dup2(tmpFd, STDOUT_FILENO);
	system(cmd);
	dup2(STDOUT_SAVE, STDOUT_FILENO);
	close(tmpFd);
	return;
}

void smi2srt(char path[PATH_LEN])
{
	//fprintf(stderr, "cmd_v1: %s\n", cmd_v1);
	strcpy(grepCmd + strlen(GREP), path);
	strcat(grepCmd, "\"");
	redirection(grepCmd, GREPFILE);

	struct stat statbuf;
	if(stat(GREPFILE, &statbuf) < 0){
		fprintf(stderr, "stat error for %s\n", GREPFILE);
		exit(1);
	}

	if(statbuf.st_size > 0){
		strcpy(cmd_v1 + strlen(CONVERT_V1), path);
		strcat(cmd_v1, "\" \"");
		strncat(cmd_v1, path, strlen(path)-3);
		strcat(cmd_v1, "srt\"");
		strcat(cmd_v1, " -d1");
		fprintf(stderr, "%s are converted with convert_v1.\n", path + strlen(nowRootDir));
		redirection(cmd_v1, TMPFILE);
	}
	else{
		strcpy(cmd_v2 + strlen(CONVERT_V2), path);
		strcat(cmd_v2, "\"");
		fprintf(stderr, "%s are converted with convert_v2.\n", path + strlen(nowRootDir));

		dup2(STDOUT_FILENO, STDOUT_SAVE);
		dup2(testFd, STDOUT_FILENO);
		system(cmd_v2);
		dup2(STDOUT_SAVE, STDOUT_FILENO);

		//redirection(cmd_v2, TMPFILE);
	}
	return;
}

void mkdir_recursive(char path[PATH_LEN])
{
	int len = strlen(path);
	if(len < strlen(backupDir))
		return;
	int i;
	for(i = len; i >= 0; i--){
		if(path[i] == '/'){
			break;
		}
	}
	if(access(path, F_OK) < 0){
		char parentPath[PATH_LEN];
		strncpy(parentPath, path, i);
		if(access(parentPath, F_OK) < 0)
			mkdir_recursive(parentPath);
	}
	if(mkdir(path, 0755) < 0)
		fprintf(stderr, "mkdir error for %s\n", path);

	return;
}

void search_directory(char *path)
{
	DIR *dirp;
	if((dirp = opendir(path)) == NULL){
		fprintf(stderr, "opendir error for %s\n", path);
		exit(1);
	}
	struct dirent *dentry;
	while((dentry = readdir(dirp)) != NULL){
		if(!strcmp(dentry->d_name, ".") || !strcmp(dentry->d_name, "..") || (dentry->d_name[0] == '@'))
			continue;
		char nowPath[PATH_LEN];
		strcpy(nowPath, path);
		strcat(nowPath, "/");
		strcat(nowPath, dentry->d_name);
		struct stat statbuf;
		if(stat(nowPath, &statbuf) < 0){
			fprintf(stderr, "stat error for %s\n", nowPath);
			exit(1);
		}
		if(S_ISDIR(statbuf.st_mode))
			search_directory(nowPath);
		else{
			if(!strcmp(nowPath+strlen(nowPath)-4, ".smi") || !strcmp(nowPath+strlen(nowPath)-4, ".SMI")){
				smi2srt(nowPath);
				if(backup){
					char backupPath[PATH_LEN];
					strcpy(backupPath, backupDir);
					strcat(backupPath, nowPath + strlen(nowRootDir));
					int i;
					for(i = strlen(backupPath); i>=0; i--){
						if(backupPath[i] == '/')
							break;
					}
					if(i != 0)
						backupPath[i] = '\0';
					char fname[FILE_LEN];
					strcpy(fname, backupPath+i+1);
					if(access(backupPath, F_OK) < 0){
						fprintf(stderr, "mkdir %s\n", backupPath);
						char tmp[PATH_LEN];
						strcpy(tmp, backupPath);
						mkdir_recursive(tmp);
					}
					strcat(backupPath, "/");
					strcat(backupPath, fname);
					if(rename(nowPath, backupPath)<0)
						fprintf(stderr, "rename error for %s\n", backupPath);
				}
			}
		}
	}
	return;
}

void get_abs_path(char result[PATH_LEN], char *path)
{
	if(path[0] == '/' || path[0] == '~'){
		strcpy(result, path);
		return;
	}
	strcpy(result, pwd);
	strcat(result, "/");
	strcat(result, path);
	return;
}

int main(int argc, char *argv[])
{
	if((testFd = open("convert_v2.txt", O_WRONLY | O_APPEND | O_CREAT, 0644)) < 0){
		fprintf(stderr, "open error for %s\n", "convert_v2.txt");
		exit(1);
	}
	int logFd;
	if((logFd = open(LOGFILE, O_WRONLY | O_APPEND | O_CREAT, 0644)) < 0){
		fprintf(stderr, "open error for %s\n", LOGFILE);
		exit(1);
	}
	dup2(STDERR_FILENO, STDERR_SAVE);
	dup2(logFd, STDERR_FILENO);
	getcwd(pwd, PATH_LEN);
	for(int i = 1; i < argc; i++){
		if(!strcmp(argv[i], "-b")){
			backup = true;
			get_abs_path(backupDir, argv[++i]);
			fprintf(stderr, "backup: %s\nbackupDir: %s\n", backup?"true":"false", backupDir);
			break;
		}
	}
	for(int i = 1; i < argc; i++){
		if(!strcmp(argv[i], "-b")){
			i++;
			continue;
		}
		char nowPath[PATH_LEN];
		get_abs_path(nowPath, argv[i]);
		strcpy(nowRootDir, nowPath);
		search_directory(nowPath);
	}
	close(logFd);
	dup2(STDERR_SAVE, STDERR_FILENO);
	close(testFd);
	exit(0);
}
