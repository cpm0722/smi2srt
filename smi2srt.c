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

#define CONVERT "/usr/local/bin/smi2srt -n \""	//CONVERT 명령어
#define TMPFILE "tmp.tmp"		//출력 결과를 임시 저장하는 임시 파일의 경로
#define LOGFILE "log.txt"		//로그 저장하는 파일의 경로			
#define ERRFILE "error.txt"		//error 로그 저장하는 파일의 경로
#define TIMEFILE "time.txt"		//최종 수행 시각을 저장하는 파일의 경로

#define STDOUT_SAVE 100			//임시로 stdout을 저장하는 fd
#define STDERR_SAVE 101			//임시로 stderr을 저장하는 fd

typedef enum {false, true} bool;

bool backup = false;			//-b 옵션 여부 저장
bool noTimeSave = false;		//-nts 옵션 여부 저장

char cmd[CMD_LEN] = CONVERT;			//CONVERT 명령어 저장

char pwd[PATH_LEN];				//현재 실행 경로 저장

char nowRootDir[PATH_LEN];		//명령어 인자로 넘겨받은 경로의 절대 경로 저장
char backupDir[PATH_LEN];		//-b 옵션 시 인자로 넘겨받은 backup 디렉터리의 절대 경로 저장

FILE *errorFp;			//error.txt FILE 포인터

time_t startTime;		//시작 시간
time_t lastTime;		//프로그램이 가장 최근에 실행됐던 시간
char strTime[TIME_LEN];	//시작 시간을 문자열로 저장

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

//smi를 srt로 변환하는 함수
void smi2srt(char path[PATH_LEN])
{
	strcpy(cmd + strlen(CONVERT), path);	//CONVERT 명령어 완성
	strcat(cmd, "\"");

	redirection(cmd, TMPFILE);				//CONVERT 수행 후 출력 결과 TMPFILE에 저장

	struct stat statbuf;
	if(stat(TMPFILE, &statbuf) < 0){			//TMPFILE stat 획득
		fprintf(stderr, "stat error for %s\n", TMPFILE);
		exit(1);
	}
	if(statbuf.st_size > 0)						//TMPFILE의 size 0 이상인 경우 (CONVERT 정상 수행된 경우)
		fprintf(stderr, "%s are converted.\n", path + strlen(nowRootDir));	//log.txt에 추가
	else										//TMPFILE의 size 0인 경우 (CONVERT error 발생한 경우)
		fprintf(errorFp, "%s\n", path);			//error.txt에 추가
	
	char jaPath[PATH_LEN];					//.ja.srt의 경로
	strcpy(jaPath, path);
	*(jaPath + strlen(jaPath) - strlen(".smi")) = '\0';	// .ja.srt 경로 생성
	strcat(jaPath, ".ja.srt");
	if(!access(jaPath, F_OK)){			//.ja.srt가 존재할 경우
		char koPath[PATH_LEN];			//.ko.srt 경로 생성
		strcpy(koPath, jaPath);
		char *str = koPath + strlen(koPath) - strlen(".ja.srt"); 
		str[1] = 'k';
		str[2] = 'o';

		rename(jaPath, koPath);
	}

	return;
}

//디렉터리를 재귀적으로 모두 mkdir하는 함수
void mkdir_recursive(char path[PATH_LEN])
{
	int len = strlen(path);
	if(len < strlen(backupDir))
		return;
	int i;
	for(i = len; i >= 0; i--){				//부모 경로가 끝나는 idx를 i에 저장
		if(path[i] == '/'){
			break;
		}
	}
	if(access(path, F_OK) < 0){				//path 디렉터리가 없을 경우
		char parentPath[PATH_LEN];			//parentPath에 부모 디렉터리 경로 저장
		memset(parentPath, '\0', PATH_LEN);
		strncpy(parentPath, path, i);
		if(access(parentPath, F_OK) < 0)	//부모 디렉터리 없을 경우
			mkdir_recursive(parentPath);	//재귀 호출
	}
	if(mkdir(path, 0755) < 0)				//path 디렉터리 mkdir
		fprintf(stderr, "mkdir error for %s\n", path);
	else
		fprintf(stderr, "mkdir %s\n", path);

	return;
}

//재귀적으로 디렉터리 탐색하며 smi 파일 찾아 smi2srt 호출하는 함수
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

		if(S_ISDIR(statbuf.st_mode)){			//현재 파일이 디렉터리인 경우
			if(statbuf.st_mtime > lastTime)		//해당 디렉터리의 최종 수정 시각이 프로그램이 가장 최근에 실행됐던 시각보다 이후일 경우
				search_directory(nowPath);		//해당 디렉터리에 대해 재귀 호출
		}

		else{									//현재 파일이 디렉터리가 아닌 일반 파일인 경우
			if(!strcmp(nowPath+strlen(nowPath)-4, ".smi") || !strcmp(nowPath+strlen(nowPath)-4, ".SMI")){	//확장자가 smi 또는 SMI인 경우

				char srtPath[PATH_LEN];			//srt 경로 생성
				strcpy(srtPath, nowPath);
				strcpy(srtPath + strlen(srtPath)-3, "srt");

				if(access(srtPath, F_OK) < 0)	//srt 파일이 없을 경우에만
					smi2srt(nowPath);				//smi2srt 호출
				else
					fprintf(stderr, "%s already exists.\n", srtPath);

				if(backup){						//-b옵션일 경우

					char backupPath[PATH_LEN];	//backup 경로 완성
					strcpy(backupPath, backupDir);
					strcat(backupPath, nowPath + strlen(nowRootDir));

					int i;
					for(i = strlen(backupPath); i>=0; i--){	//부모 디렉터리 경로가 끝나는 idx 찾아 i에 저장
						if(backupPath[i] == '/')
							break;
					}
					if(i != 0)
						backupPath[i] = '\0';			//부모 디렉터리 경로로 수정

					char fname[FILE_LEN];				//전체 경로가 아닌 실제 파일명 획득
					strcpy(fname, backupPath+i+1);

					if(access(backupPath, F_OK) < 0){	//부모 디렉터리가 없을 경우
						char tmp[PATH_LEN];
						strncpy(tmp, backupPath, i + 1);
						mkdir_recursive(tmp);			//mkdir_recursive 호출
					}

					strcat(backupPath, "/");			//부모 디렉터리 경로가 아닌 전체 경로로 복원
					strcat(backupPath, fname);

					if(rename(nowPath, backupPath)<0){	//backup 디렉터리로 mv
						fprintf(stderr, "rename error for %s\n", backupPath);
						if(access(nowPath, W_OK) < 0)
							fprintf(stderr, "access error for %s\n", nowPath);
						if(access(backupPath, W_OK) < 0)
							fprintf(stderr, "access error for %s\n", backupPath);
					}
				}
			}
		}
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
#ifdef DEBUG
	struct timeval start_tv, end_tv;
	gettimeofday(&start_tv, NULL);			//시작 시간 저장
#endif
	getcwd(pwd, PATH_LEN);					//현재 경로 획득해 pwd에 저장

	for(int i = 1; i < argc; i++){			//옵션 있는지 탐색
		if(!strcmp(argv[i], "-b")){			//-b 옵션일 경우
			backup = true;					//backup = true
			get_abs_path(backupDir, argv[++i]);	//-b 직후 인자의 절대 경로 구해 backupDir에 저장
			fprintf(stderr, "backup: %s\nbackupDir: %s\n", backup?"true":"false", backupDir);
		}
		else if(!strcmp(argv[i], "-nts"))	//-nts 옵션일 경우
			noTimeSave = true;				//backup = true
		else if(!strcmp(argv[i], "-pwd")){
			get_abs_path(pwd, argv[++i]);
			fprintf(stderr, "pwd: %s\n", pwd);
			if(chdir(pwd) < 0)
				fprintf(stderr, "chdir error for %s\n", pwd);
		}
	}

	startTime = time(NULL);			//시작 시간 startTime에 저장
	ctime_r(&startTime, strTime);	//시작 시간 문자열로 strTime에 저장
	strTime[strlen(strTime)-1] = '\0';

	if(access(TIMEFILE, F_OK) < 0)	//TIMEFILE이 없을 경우
		lastTime = 0;				//가장 최근에 프로그램 실행됐던 시간을 0으로 지정
	else{							//TIMEFILE이 있을 경우
		int timeFd;
		if((timeFd = open(TIMEFILE, O_RDONLY)) < 0){	//TIMEFILE read 권한으로 open
			fprintf(stderr, "open error for %s\n", TIMEFILE);
			exit(1);
		}

		if(read(timeFd, &lastTime, sizeof(time)) <= 0){	//TIMEFILE에 저장된 시간 lastTIme에 읽어옴
			fprintf(stderr, "read error for %s\n", TIMEFILE);
			exit(1);
		}
	}

	if((errorFp = fopen(ERRFILE, "a+")) < 0){			//ERRFILE write(append) 권한으로 open
		fprintf(stderr, "fopen error for %s\n", ERRFILE);
		exit(1);
	}

	int logFd;
	if((logFd = open(LOGFILE, O_WRONLY | O_APPEND | O_CREAT, 0644)) < 0){	//LOGFILE write(append) 권한으로 open
		fprintf(stderr, "open error for %s\n", LOGFILE);
		exit(1);
	}
	dup2(STDERR_FILENO, STDERR_SAVE);		//STDERR을 LOGFILE으로 변경
	dup2(logFd, STDERR_FILENO);

	fprintf(stderr, "\n*** [%s] start ***\n", strTime);	//LOGFILE에 현재 시각 write
	fprintf(errorFp, "\n*** [%s] start ***\n", strTime);	//ERRFILE에 현재 시각 write

	for(int i = 0; i < argc; i++){				//LOGFILE과 ERRFILE에 명령어 write
		fprintf(stderr, "%s ", argv[i]);
		fprintf(errorFp, "%s ", argv[i]);
	}
	fprintf(stderr, "\n\n");
	fprintf(errorFp, "\n\n");

	for(int i = 1; i < argc; i++){			//인자 탐색하며 search_directory 호출
		if(!strcmp(argv[i], "-b")){			//-b 옵션일 경우
			i++;							//직후 인자도 skip
			continue;
		}
		else if(!strcmp(argv[i], "-pwd")){	//-pwd 옵션일 경우
			i++;							//직후 인자도 skip
			continue;
		}
		else if(!strcmp(argv[i], "-nts"))	//-nts 옵션일 경우
			continue;
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
	fclose(errorFp);

	if(!noTimeSave){	//nts 옵션 false일 경우에만
		int timeFd;
		if((timeFd = open(TIMEFILE, O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0){	//TIMEFILE write 권한으로 open
			fprintf(stderr, "open error for %s\n", TIMEFILE);
			exit(1);
		}
		time_t now = time(NULL);
		if(write(timeFd, &now, sizeof(time_t)) <= 0){			//현재 시각 TIMEFILE에 write
			fprintf(stderr, "write error for %s\n", TIMEFILE);
		}
		close(timeFd);
	}

	dup2(STDERR_SAVE, STDERR_FILENO);							//stderr 복원

#ifdef DEBUG
	gettimeofday(&end_tv, NULL);			//종료 시간 저장
	fprintf(stderr, "process time: %lld\n", end_tv.tv_sec - start_tv.tv_sec);
#endif

	exit(0);
}
