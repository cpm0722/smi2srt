#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

#define PATH_LEN 2048
#define FILE_LEN 128
#define CMD_LEN PATH_LEN * 2 + 20
#define TIME_LEN 40

#define LOGFILE "./rename_log.txt"

#define STDOUT_SAVE 100			//임시로 stdout을 저장하는 fd
#define STDERR_SAVE 101			//임시로 stderr을 저장하는 fd

typedef enum {false, true} bool;

char pwd[PATH_LEN];				//현재 실행 경로 저장

char nowRootDir[PATH_LEN];		//명령어 인자로 넘겨받은 경로의 절대 경로 저장

//cmd를 수행하면서 출력 결과를 stdout이 아닌 tmpFile에 저장하는 함수
void redirection(char *cmd, const char *tmpFile)
{
	int tmpFd;
	if((tmpFd = open(tmpFile, O_RDWR | O_CREAT | O_TRUNC, 0644)) < 0){	//tmpFile open
		fprintf(stderr, "open error for %s\n", tmpFile);
		exit(1);
	}
	dup2(STDOUT_FILENO, STDOUT_SAVE);	//stdout 임시 저장
	dup2(tmpFd, STDOUT_FILENO);			//tmpFd를 stdout으로
	system(cmd);						//명령어 실행
	dup2(STDOUT_SAVE, STDOUT_FILENO);	//stdout 복원
	close(tmpFd);
	return;
}

//srt 파일명에 언어 추가하는 함수
bool rename_srt(char src[PATH_LEN])
{
	char *str = src + strlen(src) - strlen(".ko.srt");
	char dst[PATH_LEN];
	if(!strcmp(str, ".ko.srt"))
		return false;
	else{
		strcpy(dst, src);
		if(!strcmp(str, ".ja.srt")){
			str = dst + strlen(src) - strlen(".ja.srt");
			str[0] = '.';
			str[1] = 'k';
			str[2] = 'o';
		}
		else{
			str = dst + strlen(src) - strlen(".srt");
			*str = '\0';
			strcat(dst, ".ko.srt");
		}
		fprintf(stderr, "%s -> %s\n", src, dst);
		if(rename(src, dst) < 0)
			fprintf(stderr, "%s -> %s error!\n", src, dst);
		return true;
	}
}

//재귀적으로 디렉터리 탐색하며 srt 파일 찾아 rename_str 호출하는 함수
void search_directory(char *path)
{
	DIR *dirp;
	if((dirp = opendir(path)) == NULL){			//디렉터리 open
		fprintf(stderr, "opendir error for %s\n", path);
		exit(1);
	}
	struct dirent *dentry;
	while((dentry = readdir(dirp)) != NULL){	//디렉터리 탐색

		if(!strcmp(dentry->d_name, ".") || !strcmp(dentry->d_name, "..") || (dentry->d_name[0] == '@'))	// . .. @로 시작하는 디렉터리 skip
			continue;

		char nowPath[PATH_LEN];					//현재 탐색중인 파일에 대한 절대 경로 완성
		strcpy(nowPath, path);
		strcat(nowPath, "/");
		strcat(nowPath, dentry->d_name);

		struct stat statbuf;
		if(stat(nowPath, &statbuf) < 0){		//현재 파일에 대한 stat 획득
			fprintf(stderr, "stat error for %s\n", nowPath);
			exit(1);
		}

		if(S_ISDIR(statbuf.st_mode))		//현재 파일이 디렉터리인 경우
			search_directory(nowPath);		//해당 디렉터리에 대해 재귀 호출

		else									//현재 파일이 디렉터리가 아닌 일반 파일인 경우
			if(!strcmp(nowPath+strlen(nowPath)-4, ".srt") || !strcmp(nowPath+strlen(nowPath)-4, ".SRT"))	//확장자가 srt또는 SRT인 경우
				rename_srt(nowPath);			//rename_srt 호출
	}
	return;
}

//상대 경로를 절대경로로 변경해 result에 저장하는 함수
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
	getcwd(pwd, PATH_LEN);					//현재 경로 획득해 pwd에 저장

	int logFd;
	if((logFd = open(LOGFILE, O_WRONLY | O_APPEND | O_CREAT, 0644)) < 0){	//LOGFILE write(append) 권한으로 open
		fprintf(stderr, "open error for %s\n", LOGFILE);
		exit(1);
	}
	dup2(STDERR_FILENO, STDERR_SAVE);		//STDERR을 LOGFILE으로 변경
	dup2(logFd, STDERR_FILENO);

	for(int i = 0; i < argc; i++)				//LOGFILE과 ERRFILE에 명령어 write
		fprintf(stderr, "%s ", argv[i]);
	fprintf(stderr, "\n\n");

	for(int i = 1; i < argc; i++){			//인자 탐색하며 search_directory 호출
		char nowPath[PATH_LEN];							//현재 인자의 절대 경로 저장
		get_abs_path(nowPath, argv[i]);
		strcpy(nowRootDir, nowPath);					//현재 인자의 절대 경로 nowRootDir에 저장
		for(int i = strlen(nowRootDir); i >= 0; i--)	//현재 인자의 부모 디렉터리 경로로 변경
			if(nowRootDir[i] == '/'){
				nowRootDir[i] = '\0';
				break;
			}

		search_directory(nowPath);						//search_directory 호출

	}

	close(logFd);

	dup2(STDERR_SAVE, STDERR_FILENO);							//stderr 복원

	exit(0);
}
